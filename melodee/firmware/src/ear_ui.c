/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Main-loop adapter: existing engine loading/drawing; commands cross to audio
 * with IRQs masked. Game state and generated-note scheduling stay in audio. */
static uint8_t ear_saved_mute[NTRK], ear_saved_sel, ear_saved_rec, ear_saved_solo;
/* Only exercise references may drive keyboard lights, never hidden pitches. */
static uint32_t ear_ui_key_lights(void)
{
    uint32_t mask=0;
    fm1_irq_off();
    for(uint32_t k=0;k<4u && k+ear.guess<ear.count;k++)
        if((ear.sounding&(1u<<k)) && ear.notes[k]>=53u && ear.notes[k]<=79u)
            mask|=1u<<(ear.notes[k]-53u);
    fm1_irq_on();
    return mask;
}
static void ear_publish(uint32_t cmd)
{
    fm1_irq_off();
    ear_request=(uint8_t)cmd;
    fm1_irq_on();
}
static int ear_ui_start(void)
{
    if(ear_enabled) { ear_publish(EAR_CMD_RETRY); return 1; }
    if(song.playing || transport_busy() || boot_work.state) return 0;
    ear_saved_sel=song.sel;
    ear_saved_rec=song.rec; ear_saved_solo=(uint8_t)track_solo;
    for(uint32_t k=0;k<NTRK;k++) ear_saved_mute[k]=(uint8_t)trk[k].p[P_MUTE];
    engine_load_of(&trk[0],ENGI_FM6,0); /* Original factory TINE EP, explicitly polyphonic below. */
    fm1_irq_off();
    transport_req=2; song.rec=0; song.sel=0; track_solo=0;
    trk[0].p[P_VOICE]=V_POLY;
    panic_req=(uint8_t)((1u<<NTRK)-1u);
    audition_request=0;
    if(audition_active) audition_stop();
    for(uint32_t k=0;k<NTRK;k++) trk[k].p[P_MUTE]=k!=0u;
    for(uint32_t k=0;k<FX_SLOTS;k++) trk[0].p[P_FXSEND+k]=0;
    for(uint32_t k=0;k<27u;k++) kb_note[k]=KB_SILENT;
    ear_enabled=1; ear_request=EAR_CMD_NEXT;
    fm1_irq_on();
    ui.force=1;
    return 1;
}
static int ear_ui_input(uint32_t pressed)
{
    uint32_t home=1u<<panel.btn[B_HOME], play=1u<<panel.btn[B_PLAY];
    if((pressed&play) && (fm1_in.buttons&home)) return ear_ui_start();
    if(!ear_enabled) return 0;
    /* Consume controls while active: no transport, save or preset changes. */
    int32_t delta[4];
    for(uint32_t k=0;k<4u;k++) delta[k]=panel_enc(EN_K1+k);
    for(uint32_t k=0;k<NE;k++) (void)panel_enc(k);
    if(delta[0] || delta[1] || delta[2] || delta[3]) {
        fm1_irq_off();
        int32_t value=(int32_t)ear_settings.scale+delta[0];
        ear_settings.scale=(uint8_t)(value<0?0:value>=EAR_SCALES_COUNT?EAR_SCALES_COUNT-1:value);
        value=(int32_t)ear_settings.count+delta[1];
        ear_settings.count=(uint8_t)(value<1?1:value>4?4:value);
        value=(int32_t)ear_settings.guess+delta[2];
        ear_settings.guess=(uint8_t)(value<1?1:value>ear_settings.count?ear_settings.count:value);
        value=(int32_t)ear_settings.gap+delta[3]*25;
        ear_settings.gap=(uint16_t)(value<0?0:value>1000?1000:value);
        ear_request=EAR_CMD_NEXT;
        fm1_irq_on();ui.force=1;
    }
    if(pressed&home) {
        ear_publish(EAR_CMD_EXIT);
        fm1_irq_off();
        for(uint32_t k=0;k<NTRK;k++) trk[k].p[P_MUTE]=ear_saved_mute[k];
        song.sel=ear_saved_sel;
        song.rec=ear_saved_rec; track_solo=ear_saved_solo;
        fm1_irq_on();
        ui.home_t0=0;
        ui.force=1;
    } else if(pressed&(1u<<panel.btn[B_REC])) ear_publish(EAR_CMD_NEXT);
    else if(pressed&play) ear_publish(EAR_CMD_RETRY);
    return 1;
}
static void ear_ui_draw(void)
{
    uint32_t state, attempts, armed, score, streak, award, count, guess, entered, index, scale, gap, sounding;
    uint8_t notes[4];
    static uint32_t last_state=EAR_STATES_COUNT, last_attempts, last_armed, last_entered,last_index;
    fm1_irq_off();
    state=ear.state; attempts=ear.attempts; armed=ear.armed;
    score=ear.score;streak=ear.streak;award=ear.last_award;
    count=ear.count;guess=ear.guess;entered=ear.entered;index=ear.index;scale=ear.scale;gap=ear.gap;
    sounding=ear.sounding;
    for(uint32_t k=0;k<4u;k++) notes[k]=ear.notes[k];
    fm1_irq_on();
    if(!ui.force && state==last_state && attempts==last_attempts && armed==last_armed && entered==last_entered && index==last_index) return;
    last_entered=entered;last_index=index;
    last_state=state; last_attempts=attempts; last_armed=armed; ui.force=0;
    const char *status=state==EAR_CORRECT?"CORRECT!":state==EAR_WRONG?"WRONG":
        state==EAR_ANSWER?(armed?"PLAY HIDDEN NOTES":"RELEASE KEYS"):
        state==EAR_FIRST || state==EAR_SECOND?"LISTEN":
        state==EAR_PREPARE?"GET READY":"LISTEN";
    /* Independent header shows persistent state; never expose target pitches. */
    uint16_t bg=state==EAR_CORRECT?RGB(16,104,55):state==EAR_WRONG?RGB(148,32,42):T_BG;
    uint16_t text=(state==EAR_CORRECT || state==EAR_WRONG)?RGB(255,255,255):T_TEXT;
    uint16_t dim=(state==EAR_CORRECT || state==EAR_WRONG)?RGB(230,238,230):T_DIM;
    uint16_t accent=(state==EAR_CORRECT || state==EAR_WRONG)?text:T_THEME;
    for(uint32_t band=0;band<5u;band++) {
        cv_begin(240,48,bg);
        if(band==0) {
            cv_text(10,0,TY_LB,"EAR TRAINER",accent);
            char stats[32]="PTS ";fmt_int(stats+4,(int32_t)score);
            uint32_t at=str_len(stats);str_cpy(stats+at," S",3);fmt_int(stats+at+2,(int32_t)streak);
            cv_text(10,25,TY_LB,stats,dim);
        }
        else if(band==1) {
            static const char *names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            for(uint32_t k=0;k<count;k++) {
                char label[8]="?";
                if(k<count-guess) {
                    str_cpy(label,names[notes[k]%12u],4);
                    fmt_int(label+str_len(label),(int32_t)(notes[k]/12u)-1);
                }
                /* Only known pitches get a held-note highlight; hidden notes stay '?'. */
                uint32_t lit=k<count-guess && (sounding&(1u<<k));
                cv_text(10+k*58u,4,TY_TX,label,lit?RGB(90,255,150):text);
                if(lit) cv_rect(10+k*58u,31,42,3,RGB(90,255,150));
            }
        } else if(band==2) {
            cv_text(10,2,TY_TX,status,text);
            char progress[32]="TRIES ";fmt_int(progress+6,(int32_t)attempts);
            uint32_t at=str_len(progress);str_cpy(progress+at,"  ",3);
            fmt_int(progress+at+2,(int32_t)(entered>guess?guess:entered));
            at=str_len(progress);progress[at]='/';fmt_int(progress+at+1,(int32_t)guess);
            cv_text(10,28,TY_LB,progress,dim);
        } else if(band==3) {
            if(state==EAR_CORRECT) {
                char points[24]="+";fmt_int(points+1,(int32_t)award);
                cv_text(10,0,TY_LB,points,text);
                cv_text(70,0,TY_LB,"NEXT AUTO",text);
            } else cv_text(10,0,TY_LB,state==EAR_WRONG?"REPEAT AUTOMATIC":"PLAY: REPLAY",text);
            cv_text(10,24,TY_LB,"REC: NEXT  HOME: EXIT",dim);
        }
        else {
            cv_text(10,0,TY_LB,scale==EAR_MINOR?"K1 A MINOR":"K1 C MAJOR",text);
            char label[24]="K2 ";fmt_int(label+3,(int32_t)count);cv_text(150,0,TY_LB,label,text);
            str_cpy(label,"K3 GUESS ",10);fmt_int(label+9,(int32_t)guess);cv_text(10,24,TY_LB,label,text);
            str_cpy(label,"K4 ",4);fmt_int(label+3,(int32_t)gap);uint32_t at=str_len(label);str_cpy(label+at,"ms",3);
            cv_text(140,24,TY_LB,label,text);
        }
        cv_blit(0,band*48u);
    }
}
