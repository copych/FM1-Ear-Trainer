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
/* Factory loading stays in main; the existing engine generation/panic publication
 * stops old voices. Publish poly mode and zero sends before audio resumes. */
static void ear_ui_load_preset(uint32_t cmd)
{
    fm1_irq_off();ear_loading=1;ear_request=EAR_CMD_NONE;fm1_irq_on();
    engine_load_of(&trk[0],ear_preset_engine(ear_settings.preset),ear_preset_index(ear_settings.preset));
    fm1_irq_off();
    trk[0].p[P_VOICE]=V_POLY;
    for(uint32_t k=0;k<FX_SLOTS;k++) trk[0].p[P_FXSEND+k]=0;
    ear_request=(uint8_t)cmd;ear_loading=0;
    fm1_irq_on();
}
static int ear_ui_start(void)
{
    if(ear_enabled) { ear_publish(EAR_CMD_RETRY); return 1; }
    if(song.playing || transport_busy() || boot_work.state) return 0;
    ear_saved_sel=song.sel;
    ear_saved_rec=song.rec; ear_saved_solo=(uint8_t)track_solo;
    for(uint32_t k=0;k<NTRK;k++) ear_saved_mute[k]=(uint8_t)trk[k].p[P_MUTE];
    ear_ui_load_preset(EAR_CMD_NONE); /* Default TINE EP; retain selected sound on re-entry. */
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
    /* The printed PRESETS knob owns sound selection; K3 remains available. */
    int32_t delta[4];
    for(uint32_t k=0;k<4u;k++) delta[k]=panel_enc(EN_K1+k);
    delta[2]=panel_enc(EN_PRESET);
    for(uint32_t k=0;k<NE;k++) (void)panel_enc(k);
    if(delta[0] || delta[1] || delta[2] || delta[3]) {
        ear_settings_t before=ear_settings;
        uint32_t sound_changed, cmd;
        fm1_irq_off();
        int32_t value=(int32_t)ear_settings.scale+delta[0];
        ear_settings.scale=(uint8_t)(value<0?0:value>=EAR_SCALES_COUNT?EAR_SCALES_COUNT-1:value);
        /* Ten pairs per range: octave first, then full keyboard. */
        value=(int32_t)(10u*ear_settings.range+ear_settings.count*(ear_settings.count-1u)/2u+ear_settings.guess-1u)+delta[1];
        uint32_t complexity=(uint32_t)(value<0?0:value>=10*EAR_RANGES_COUNT?10*EAR_RANGES_COUNT-1:value), count=1;
        ear_settings.range=(uint8_t)(complexity/10u);complexity%=10u;
        for(;count<4u && complexity>=count;count++) complexity-=count;
        ear_settings.count=(uint8_t)count;
        ear_settings.guess=(uint8_t)(complexity+1u);
        value=(int32_t)ear_settings.gap+delta[3]*25;
        ear_settings.gap=(uint16_t)(value<0?0:value>1000?1000:value);
        value=(int32_t)ear_settings.preset+delta[2];
        uint32_t preset=(uint32_t)(value<0?0:value>=EAR_PRESETS_COUNT?EAR_PRESETS_COUNT-1:value);
        sound_changed=preset!=ear_settings.preset;ear_settings.preset=(uint8_t)preset;
        cmd=delta[0] || delta[1] || delta[3] || ear.state==EAR_CORRECT?EAR_CMD_NEXT:EAR_CMD_REPEAT;
        if(sound_changed) { ear_loading=1;ear_request=EAR_CMD_NONE; }
        else if(delta[0] || delta[1] || delta[3]) ear_request=(uint8_t)cmd;
        fm1_irq_on();ui.force=1;
        if(sound_changed) ear_ui_load_preset(cmd);
        if(before.scale!=ear_settings.scale || before.count!=ear_settings.count ||
           before.guess!=ear_settings.guess || before.range!=ear_settings.range ||
           before.gap!=ear_settings.gap || before.preset!=ear_settings.preset) {
            ear_settings_ms=fm1_ms;settings_save();
        }
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
/* Dedicated game screen: four knob zones, musical cards/result, score, patch.
 * Five 48-row tiles retain the existing bounded canvas instead of a framebuffer. */
static void ear_ui_draw(void)
{
    uint32_t state,attempts,armed,score,streak,award,count,guess,entered,index,scale,gap,sounding,range;
    uint8_t notes[4];
    static uint32_t last_state=EAR_STATES_COUNT,last_attempts,last_armed,last_entered,last_index;
    fm1_irq_off();
    state=ear.state;attempts=ear.attempts;armed=ear.armed;score=ear.score;streak=ear.streak;award=ear.last_award;
    count=ear.count;guess=ear.guess;entered=ear.entered;index=ear.index;scale=ear.scale;gap=ear.gap;
    sounding=ear.sounding;range=ear.range;
    for(uint32_t k=0;k<4u;k++)notes[k]=ear.notes[k];
    fm1_irq_on();
    if(!ui.force && state==last_state && attempts==last_attempts && armed==last_armed &&
        entered==last_entered && index==last_index)return;
    last_state=state;last_attempts=attempts;last_armed=armed;last_entered=entered;last_index=index;ui.force=0;
    uint16_t bg=state==EAR_CORRECT?RGB(16,104,55):state==EAR_WRONG?RGB(148,32,42):T_BG;
    uint16_t ink=(state==EAR_CORRECT || state==EAR_WRONG)?RGB(255,255,255):T_TEXT;
    uint16_t dim=(state==EAR_CORRECT || state==EAR_WRONG)?RGB(230,238,230):T_DIM;
    char label[40];
    for(uint32_t band=0;band<5u;band++) {
        cv_begin(240,48,bg);
        if(band==0) {
            if(ear_saving) {
                cv_text_c(120,4,TY_TX,"SAVING SETTINGS",ink,bg);
                cv_text_c(120,28,TY_LB,"PLEASE WAIT",dim,bg);
            } else for(uint32_t knob=0;knob<4u;knob++) {
                int32_t x=(int32_t)knob*60;
                cv_rrect(x+2,2,56,44,4,T_PANEL,bg);
                str_cpy(label,"K",2);fmt_int(label+1,(int32_t)knob+1);
                cv_text_c(x+30,2,TY_LB,label,T_DIM,T_PANEL);
                if(knob==0)str_cpy(label,scale==EAR_MINOR?"A MIN":"C MAJ",sizeof label);
                else if(knob==1) {
                    fmt_int(label,(int32_t)guess);uint32_t at=str_len(label);
                    str_cpy(label+at," OF ",5);fmt_int(label+at+4,(int32_t)count);
                } else if(knob==2)str_cpy(label,"--",sizeof label);
                else fmt_int(label,(int32_t)gap);
                if(text_w(TY_LB,label)>54 && knob==0)
                    str_cpy(label,scale==EAR_MINOR?"AMIN":"CMAJ",sizeof label);
                if(text_w(TY_LB,label)>54 && knob==1) {
                    fmt_int(label,(int32_t)guess);uint32_t at=str_len(label);
                    label[at]='/';fmt_int(label+at+1,(int32_t)count);
                }
                const aafont_t *f=ty_fit(TY_LB,label,54);
                cv_text_c(x+30,17,f,label,T_TEXT,T_PANEL);
                const char *caption=knob==0?(text_w(TY_LB,"SCALE")>54?"KEY":"SCALE"):knob==1?(range==EAR_RANGE_FULL?"FULL":"OCT"):
                    knob==2?"FREE":"ms";
                cv_text_c(x+30,32,TY_LB,caption,T_DIM,T_PANEL);
            }
        } else if(band==1 && count) {
            static const char *names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            uint32_t width=(224u-(count-1u)*4u)/count,known=count-guess;
            for(uint32_t k=0;k<count;k++) {
                int32_t x=8+(int32_t)(k*(width+4u));
                uint32_t lit=k<known && (sounding&(1u<<k));
                uint32_t accepted=k>=known && k<known+entered;
                uint32_t failed=state==EAR_WRONG && k==known+entered;
                uint32_t good=state==EAR_CORRECT || accepted || lit;
                uint16_t fill=good?RGB(22,76,52):failed?RGB(92,28,35):T_PANEL;
                uint16_t border=good?RGB(90,255,150):failed?RGB(255,180,180):T_DIM;
                if((state==EAR_FIRST || state==EAR_SECOND) && k==index && !lit)border=T_THEME;
                cv_rrect(x,4,(int32_t)width,40,5,border,bg);
                cv_rrect(x+2,6,(int32_t)width-4,36,4,fill,border);
                str_cpy(label,"?",sizeof label);
                if(k<known || state==EAR_CORRECT) {
                    str_cpy(label,names[notes[k]%12u],sizeof label);
                    fmt_int(label+str_len(label),(int32_t)(notes[k]/12u)-1);
                }
                const aafont_t *f=ty_fit(TY_VL,label,(int32_t)width-8);
                cv_text_c(x+(int32_t)width/2,ty_y(24,f),f,label,good || failed?RGB(255,255,255):T_TEXT,fill);
                if(good)cv_rect(x+5,39,(int32_t)width-10,3,RGB(90,255,150));
            }
        } else if(band==2) {
            const char *status=state==EAR_CORRECT?"CORRECT":state==EAR_WRONG?"WRONG - REPLAY":
                state==EAR_ANSWER?(armed?"YOUR TURN":"RELEASE KEYS"):
                state==EAR_PREPARE?"GET READY":"LISTEN";
            str_cpy(label,status,sizeof label);
            if(state==EAR_CORRECT) {uint32_t at=str_len(label);str_cpy(label+at," +",3);fmt_int(label+at+2,(int32_t)award);}
            cv_text_c(120,2,ty_fit(TY_TX,label,224),label,ink,bg);
            str_cpy(label,"TRY ",5);fmt_int(label+4,(int32_t)attempts);
            cv_text(8,29,TY_LB,label,dim);
            str_cpy(label,"NOTES ",7);fmt_int(label+6,(int32_t)(entered>guess?guess:entered));
            uint32_t at=str_len(label);label[at]='/';fmt_int(label+at+1,(int32_t)guess);
            cv_text_r(232,29,TY_LB,label,dim,bg);
        } else if(band==3) {
            cv_text(8,0,TY_LB,"SCORE",dim);cv_text(158,0,TY_LB,"STREAK",dim);
            fmt_int(label,(int32_t)score);cv_text(8,17,ty_fit(TY_VL,label,140),label,ink);
            fmt_int(label,(int32_t)streak);cv_text(158,17,TY_VL,label,ink);
        } else if(band==4) {
            uint32_t engine=ear_preset_engine(ear_settings.preset),preset=ear_preset_index(ear_settings.preset);
            const preset_t *patch=eng_preset(ENGINES[engine],preset);
            const aafont_t *font=TY_VL;
            if(text_w(font,patch->name)>224)font=ty_fit(TY_TX,patch->name,224);
            cv_free_text(8,ty_y(24,font),font,patch->name,ink,bg,224);
        }
        cv_blit(0,band*48u);
    }
}
