/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Main-only navigation. Activity teardown remains audio-owned: never launch
 * another activity until its EXIT command has been consumed. */
enum eTrainingActivity { TRAINING_PITCH, TRAINING_METRONOME, TRAINING_RHYTHM, TRAINING_ACTIVITIES_COUNT };
static uint8_t training_choice,training_session;
static int training_ui_start(void)
{
    if(training_menu_open)return 1;
    if(ear_enabled || metro_enabled || song.playing || transport_busy() || boot_work.state)return 0;
    fm1_irq_off();
    training_menu_open=1;training_session=1;
    panic_req=(uint8_t)((1u<<NTRK)-1u);audition_request=0;
    if(audition_active)audition_stop();
    for(uint32_t k=0;k<27u;k++)kb_note[k]=KB_SILENT;
    fm1_irq_on();ui.force=1;return 1;
}
static int training_ui_launch(void)
{
    if(!training_menu_open || ear_enabled || metro_enabled || ear_request || metro_request)return 0;
    int started;
    if(training_choice==TRAINING_PITCH)started=ear_ui_start();
    else {
        fm1_irq_off();metro_settings.mode=(uint8_t)(training_choice==TRAINING_RHYTHM?METRO_GAME:METRO_PRACTICE);fm1_irq_on();
        started=metro_ui_start();
        if(started)settings_save();
    }
    if(started) {fm1_irq_off();training_menu_open=0;fm1_irq_on();ui.force=1;}
    return started;
}
static int training_ui_input(uint32_t pressed)
{
    uint32_t home=1u<<panel.btn[B_HOME],play=1u<<panel.btn[B_PLAY];
    if(training_session && !training_menu_open && (pressed&home) && (ear_enabled || metro_enabled)) {
        /* Reuse normal exits so ratings, saved settings and track restoration
         * keep their existing semantics. The chooser then waits for audio. */
        if(ear_enabled)ear_ui_input(home);else metro_ui_exit();
        fm1_irq_off();training_menu_open=1;fm1_irq_on();ui.force=1;return 1;
    }
    if(!training_menu_open) {
        if((pressed&play) && (fm1_in.buttons&home) && !ear_enabled && !metro_enabled)return training_ui_start();
        return 0;
    }
    int32_t d=panel_enc(EN_SELECT)+panel_enc(EN_K1);
    for(uint32_t k=0;k<NE;k++)(void)panel_enc(k);
    if(d) {training_choice=(uint8_t)clamp((int32_t)training_choice+d,0,TRAINING_ACTIVITIES_COUNT-1);ui.force=1;}
    if(pressed&home) {
        fm1_irq_off();training_menu_open=0;training_session=0;fm1_irq_on();ui.home_t0=0;ui.force=1;
    } else if(pressed&(play|(1u<<panel.btn[B_OCTUP]))) (void)training_ui_launch();
    return 1;
}
static void training_ui_draw(void)
{
    if(!ui.force)return;ui.force=0;
    static const char *names[TRAINING_ACTIVITIES_COUNT]={"Pitch training","Metronome","Rhythm game"};
    static const char *hints[TRAINING_ACTIVITIES_COUNT]={"Find hidden notes","Meters and pulse grids","Full / Odds / Evens"};
    for(uint32_t band=0;band<5u;band++) {
        cv_begin(240,48,T_BG);
        if(!band) {
            cv_text_c(120,0,TY_TX,"EAR TRAINER",T_THEME,T_BG);
            cv_text_c(120,27,TY_LB,"SELECT  PLAY  HOME",T_DIM,T_BG);
        } else if(band<=TRAINING_ACTIVITIES_COUNT) {
            uint32_t at=band-1u;uint16_t border=at==training_choice?T_THEME:T_PANEL;
            cv_rrect(5,2,230,44,5,border,T_BG);cv_rrect(7,4,226,40,4,T_PANEL,border);
            cv_text_c(120,4,TY_TX,names[at],at==training_choice?T_THEME:T_TEXT,T_PANEL);
            cv_text_c(120,26,TY_LB,hints[at],T_DIM,T_PANEL);
        } else cv_text_c(120,16,TY_LB,"HOME: LEAVE TRAINING",T_DIM,T_BG);
        cv_blit(0,band*48u);
    }
}
