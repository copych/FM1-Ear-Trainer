/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Main-loop adapter, included after ear_ui.c. Reuses its track isolation and
 * restoration slots; the two activities cannot run simultaneously. */
static uint8_t metro_preset,metro_custom;
static uint16_t metro_seen_bpm=120;
static void metro_publish(uint32_t cmd)
{ fm1_irq_off();metro_request=(uint8_t)cmd;fm1_irq_on();ui.force=1; }
static int metro_ui_start(void)
{
    if(metro_enabled)return 1;
    if(ear_enabled || song.playing || transport_busy() || boot_work.state)return 0;
    ear_saved_sel=song.sel;ear_saved_rec=song.rec;ear_saved_solo=(uint8_t)track_solo;
    fm1_irq_off();
    metro_normalize(&metro_settings);
    for(uint32_t k=0;k<NTRK;k++) {ear_saved_mute[k]=(uint8_t)trk[k].p[P_MUTE];trk[k].p[P_MUTE]=1;}
    transport_req=2;song.rec=0;track_solo=0;panic_req=(uint8_t)((1u<<NTRK)-1u);
    audition_request=0;if(audition_active)audition_stop();
    for(uint32_t k=0;k<27u;k++)kb_note[k]=KB_SILENT;
    metro_enabled=1;metro_request=METRO_CMD_RESTART;
    fm1_irq_on();metro_seen_bpm=metro_settings.bpm;metro_custom=1;ui.force=1;return 1;
}
static void metro_ui_exit(void)
{
    fm1_irq_off();metro_request=METRO_CMD_EXIT;
    for(uint32_t k=0;k<NTRK;k++)trk[k].p[P_MUTE]=ear_saved_mute[k];
    song.sel=ear_saved_sel;song.rec=ear_saved_rec;track_solo=ear_saved_solo;
    fm1_irq_on();ui.home_t0=0;ui.force=1;
    settings_save(); /* settings_poll waits until audio has consumed EXIT */
}
static void metro_ui_changed(void)
{
    metro_custom=1;
    metro_seen_bpm=metro_settings.bpm;
    fm1_irq_off();
    if(metro.state!=METRO_STOPPED)metro_request=METRO_CMD_RESTART;
    fm1_irq_on();ui.force=1;settings_save();
}
static int metro_ui_input(uint32_t pressed)
{
    uint32_t home=1u<<panel.btn[B_HOME];
    if((pressed&(1u<<panel.btn[B_REC])) && (fm1_in.buttons&home))return metro_ui_start();
    if(!metro_enabled)return 0;
    int32_t d[4];for(uint32_t k=0;k<4u;k++)d[k]=panel_enc(EN_K1+k);
    int32_t preset=panel_enc(EN_PRESET),offset=panel_enc(EN_ALGO);
    for(uint32_t k=0;k<NE;k++)(void)panel_enc(k);
    if(metro_seen_bpm!=metro_settings.bpm)metro_ui_changed();
    if(offset) {
        fm1_irq_off();metro_settings.offset=(int8_t)clamp((int32_t)metro_settings.offset+offset,-100,100);fm1_irq_on();
        metro_ui_changed();
    }
    if(preset) {
        metro_preset=(uint8_t)clamp((int32_t)metro_preset+preset,0,METRO_FACTORY_COUNT+METRO_USER_COUNT-1u);
        fm1_irq_off();
        uint8_t activity=metro_settings.mode;
        if(metro_preset<METRO_FACTORY_COUNT) {
            int8_t correction=metro_settings.offset;metro_settings=metro_factory[metro_preset].settings;metro_settings.offset=correction;
        }
        else if(metro_user[metro_preset-METRO_FACTORY_COUNT].bpm)metro_settings=metro_user[metro_preset-METRO_FACTORY_COUNT];
        metro_settings.mode=activity;metro_normalize(&metro_settings);
        fm1_irq_on();
        metro_ui_changed();
        metro_custom=0;
    }
    if(d[0] || d[1] || d[2] || d[3]) {
        fm1_irq_off();
        metro_settings.bpm=(uint16_t)clamp((int32_t)metro_settings.bpm+d[0],30,300);
        metro_settings.meter=(uint8_t)clamp((int32_t)metro_settings.meter+d[1],0,METRO_METERS_COUNT-1);
        metro_settings.game=(uint8_t)clamp((int32_t)metro_settings.game+d[2],0,METRO_GAMES_COUNT-1);
        metro_settings.cycle=(uint8_t)clamp((int32_t)metro_settings.cycle+d[3],0,METRO_CYCLES_COUNT-1);
        metro_normalize(&metro_settings);
        fm1_irq_on();metro_ui_changed();
    }
    if(pressed&home)metro_ui_exit();
    else if(pressed&(1u<<panel.btn[B_PLAY]))metro_publish(METRO_CMD_TOGGLE);
    else if(pressed&(1u<<panel.btn[B_REC])) {metro_publish(METRO_CMD_TAP);settings_save();}
    else if(pressed&(1u<<panel.btn[B_OCTUP])) {
        if(metro_preset>=METRO_FACTORY_COUNT) {
            metro_user[metro_preset-METRO_FACTORY_COUNT]=metro_settings;settings_save();ui.force=1;
            metro_custom=0;
        }
    } else if(pressed&(1u<<panel.btn[B_OCTDN]))metro_publish(METRO_CMD_RESTART);
    return 1;
}
/* Exact integer sqrt for display-only standard deviation, bounded 16 steps. */
static uint32_t metro_sqrt(uint32_t x)
{
    uint32_t r=0,bit=1u<<30;
    for(uint32_t i=0;i<16u;i++,bit>>=2) {
        if(x>=r+bit) {x-=r+bit;r=(r>>1)+bit;}else r>>=1;
    }
    return r;
}
static uint32_t metro_deviation(uint32_t hits,int32_t sum,uint32_t squares)
{
    /* Multiply before dividing: truncating the mean first would turn tiny
     * jitter around a large fixed delay into a falsely large deviation.
     * 120 taps x 150 ms bounds both products below 324,000,000. */
    return hits?metro_sqrt((squares*hits-(uint32_t)(sum*sum))/(hits*hits)):0;
}
static void metro_ui_draw(void)
{
    static uint32_t version=~0u;
    uint32_t state,beat,bar,hits,misses,extra,targets,abs_ms,square_ms,v,window;
    int32_t bias,last;metro_settings_t s;
    fm1_irq_off();
    s=metro_settings;v=metro.version;state=metro.state;beat=metro.beat;bar=metro.bar;
    hits=metro.hits;misses=metro.misses;extra=metro.extra;targets=metro.targets;
    abs_ms=metro.abs_ms;square_ms=metro.square_ms;bias=metro.bias_ms;last=metro.last_ms;window=metro.window;
    fm1_irq_on();
    if(!ui.force && version==v)return;version=v;ui.force=0;
    const metro_meter_t *m=&metro_meters[s.meter];char label[48];
    for(uint32_t band=0;band<5u;band++) {
        cv_begin(240,48,T_BG);
        if(!band) {
            cv_text(8,0,TY_LB,s.mode==METRO_PRACTICE?"METRO":"RHYTHM",T_THEME);
            const char *status=state==METRO_STOPPED?"STOPPED":state==METRO_COUNTIN?"COUNT IN":
                state==METRO_RESULT?"RESULT":s.mode==METRO_PRACTICE?"PLAYING":"TAP SILENCE";
            cv_text_r(232,0,TY_LB,status,T_TEXT,T_BG);
            str_cpy(label,"REC TAP  OFS ",sizeof label);
            fmt_int(label+str_len(label),s.offset);str_cpy(label+str_len(label),"ms",3);
            cv_text_c(120,24,TY_LB,label,T_DIM,T_BG);
        } else if(band==1) {
            for(uint32_t k=0;k<4u;k++) {
                int32_t x=(int32_t)k*60;
                const char *caption=k==0?"TEMPO":k==1?"METER":k==2?"GAME":"A/H";
                cv_text_c(x+30,0,ty_fit(TY_LB,caption,58),caption,T_DIM,T_BG);
                if(!k)fmt_int(label,s.bpm);
                else if(k==1)str_cpy(label,m->name,sizeof label);
                else if(k==2)str_cpy(label,s.game==METRO_FULL_BARS?"FULL":s.game==METRO_ODDS?"ODDS":"EVENS",sizeof label);
                else str_cpy(label,s.cycle==METRO_CYCLE_1_1?"1/1":"2/2",sizeof label);
                cv_text_c(x+30,23,TY_LB,label,T_TEXT,T_BG);
            }
        } else if(band==2) {
            if(s.mode==METRO_PRACTICE || state==METRO_COUNTIN || state==METRO_STOPPED) {
                cv_text_c(120,0,TY_TX,m->groups,T_TEXT,T_BG);
                for(uint32_t k=0;k<m->beats;k++) {
                    int32_t w=224/(int32_t)m->beats,x=8+(int32_t)k*w;
                    uint16_t c=(state==METRO_RUNNING || state==METRO_COUNTIN) && k==beat?T_THEME:T_PANEL;
                    cv_rrect(x,29,w-2,12,2,c,T_BG);
                }
            } else if(state==METRO_RESULT) {
                uint32_t score=metro_grade(hits,targets,extra,abs_ms,window);
                str_cpy(label,"SCORE ",sizeof label);fmt_int(label+6,(int32_t)score);
                str_cpy(label+str_len(label),"%",2);cv_text_c(120,0,TY_TX,label,T_THEME,T_BG);
                str_cpy(label,"HIT ",sizeof label);fmt_int(label+4,(int32_t)hits);uint32_t at=str_len(label);
                label[at]='/';fmt_int(label+at+1,(int32_t)targets);at=str_len(label);
                str_cpy(label+at," BIAS ",7);fmt_int(label+at+6,hits?bias/(int32_t)hits:0);
                cv_text_c(120,25,ty_fit(TY_LB,label,232),label,T_TEXT,T_BG);
            } else {
                /* No phase/bar/playhead hints during the scored section. */
                cv_text_c(120,0,TY_TX,s.game==METRO_FULL_BARS?"FULL BARS":s.game==METRO_ODDS?"ODD PULSES":"EVEN PULSES",T_TEXT,T_BG);
                cv_text_c(120,26,TY_LB,"KEY / MIDI: TAP",T_DIM,T_BG);
            }
        } else if(band==3) {
            if(s.mode==METRO_PRACTICE) {
                cv_text_c(120,0,TY_LB,"REC: TAP TEMPO",T_TEXT,T_BG);
                cv_text_c(120,26,TY_LB,"QUARTER-NOTE BPM",T_DIM,T_BG);
            } else {
                str_cpy(label,"ERROR ",sizeof label);fmt_int(label+6,hits?(int32_t)(abs_ms/hits):0);
                uint32_t at=str_len(label);str_cpy(label+at,"ms  SD ",8);fmt_int(label+at+7,(int32_t)metro_deviation(hits,bias,square_ms));
                cv_text_c(120,0,ty_fit(TY_LB,label,232),label,T_TEXT,T_BG);
                if(state==METRO_RESULT) {
                    str_cpy(label,"MISS ",sizeof label);fmt_int(label+5,(int32_t)misses);at=str_len(label);
                    str_cpy(label+at,"  EXTRA ",9);fmt_int(label+at+8,(int32_t)extra);
                } else {str_cpy(label,last<0?"EARLY ":"LATE ",sizeof label);fmt_int(label+(last<0?6:5),last<0?-last:last);}
                cv_text_c(120,25,TY_LB,label,T_DIM,T_BG);
            }
        } else {
            if(metro_preset<METRO_FACTORY_COUNT)str_cpy(label,metro_custom?"CUSTOM SETUP":metro_factory[metro_preset].name,sizeof label);
            else {
                str_cpy(label,"USER ",sizeof label);fmt_int(label+5,metro_preset-METRO_FACTORY_COUNT+1);
                str_cpy(label+str_len(label),metro_user[metro_preset-METRO_FACTORY_COUNT].bpm?"  OCT+: SAVE":" EMPTY / OCT+: SAVE",28);
            }
            cv_text_c(120,0,ty_fit(TY_LB,label,232),label,T_THEME,T_BG);
            cv_text_c(120,26,TY_LB,"PLAY: GO  HOME: EXIT",T_DIM,T_BG);
        }
        cv_blit(0,band*48u);
    }
    (void)bar;
}
