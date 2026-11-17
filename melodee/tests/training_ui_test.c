/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <assert.h>
/* Populate the UI harness's tentative matrix with the real panel layout. */
static const int8_t FM1_KEYMAP[6][FM1_NCOL]={
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
    {5,11,4,10,3,9,2,8,-1,-1,-1},
    {34,35,36,37,38,40,39,13,7,6,12},
    {23,24,25,26,27,28,29,30,31,32,33},
    {0,1,15,14,17,16,19,18,20,21,22},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}
};
static void training_press(uint32_t button)
{host_pressed=1u<<panel.btn[button];ui_input();}
static void screenshot(void)
{
    ui.force=1;ui_draw();FILE *f=fopen("build/ear-baseline/training-menu.ppm","wb");assert(f);
    fprintf(f,"P6\n240 240\n255\n");
    for(uint32_t k=0;k<240u*240u;k++) {
        uint16_t c=(uint16_t)((host_screen[k]>>8)|(host_screen[k]<<8));
        uint8_t p[3]={(uint8_t)(((c>>11)&31u)*255u/31u),(uint8_t)(((c>>5)&63u)*255u/63u),(uint8_t)((c&31u)*255u/31u)};
        fwrite(p,1,3,f);
    }
    fclose(f);
}
int main(void)
{
    ui_power_on();song.master_q12=4096;settings.palette=0;palette_set(0);
    uint32_t home=1u<<panel.btn[B_HOME];
    fm1_in.buttons=home;training_press(B_PLAY);fm1_in.buttons=0;
    assert(training_menu_open && training_session && !ear_enabled && !metro_enabled);
    settings_lights=LIGHTS_MID;ui_leds();
    for(uint32_t k=0;k<27u;k++)assert(led_get(fm1_led_dim[0],14u+k) && !led_get(fm1_led,14u+k));
    settings_lights=LIGHTS_OFF;ui_leds();
    for(uint32_t k=0;k<27u;k++)assert(!led_get(fm1_led_dim[0],14u+k));
    settings_lights=LIGHTS_MID;
    screenshot();
    /* Opening the chooser silences the instrument, but raw input ownership
     * still follows held keys and MIDI sources across activity boundaries. */
    events_block(CTL);uint32_t age=vage;
    fm1_in.notes=1u;keyboard_block();assert(ear_keys==1u && vage==age);
    assert(ear_midi(0x90,0,60,100,1));assert(ear_down[1]==1u && vage==age);
    assert(ear_midi(0x80,0,60,0,1));assert(!ear_down[1]);
    fm1_in.notes=0;keyboard_block();
    for(uint32_t choice=0;choice<TRAINING_ACTIVITIES_COUNT;choice++) {
        host_enc[panel.enc[EN_SELECT]]=(int32_t)choice-training_choice;ui_input();
        assert(training_choice==choice);training_press(B_PLAY);
        assert(!training_menu_open && training_session);
        assert(ear_enabled==(choice==TRAINING_PITCH));assert(metro_enabled==(choice!=TRAINING_PITCH));
        events_block(CTL);
        if(choice!=TRAINING_PITCH)assert(metro.cfg.mode==(choice==TRAINING_METRONOME?METRO_PRACTICE:METRO_GAME));
        ui_leds();for(uint32_t k=0;k<27u;k++)assert(led_get(fm1_led_dim[0],14u+k));
        if(choice!=TRAINING_PITCH) {
            fm1_in.notes=1u;ui_leds();assert(led_get(fm1_led,14u));fm1_in.notes=0;
        }
        training_press(B_HOME);assert(training_menu_open);
        uint32_t exiting=choice==TRAINING_PITCH?ear_request:metro_request;
        assert(exiting==(choice==TRAINING_PITCH?EAR_CMD_EXIT:METRO_CMD_EXIT));
        /* PLAY before audio teardown cannot overwrite EXIT or re-save the
         * temporarily muted track state as the next activity's baseline. */
        training_press(B_PLAY);
        assert((choice==TRAINING_PITCH?ear_request:metro_request)==exiting);
        events_block(CTL);assert(!ear_enabled && !metro_enabled);
        assert(training_menu_open && training_session);
    }
    host_enc[panel.enc[EN_SELECT]]=100;ui_input();assert(training_choice==TRAINING_ACTIVITIES_COUNT-1u);
    host_enc[panel.enc[EN_SELECT]]=-100;ui_input();assert(training_choice==TRAINING_PITCH);
    training_press(B_HOME);assert(!training_menu_open && !training_session);
    song.playing=1;assert(!training_ui_start());song.playing=0;
    /* Existing direct rhythm shortcut still works outside the shared menu. */
    fm1_in.buttons=home;training_press(B_REC);fm1_in.buttons=0;
    assert(metro_enabled && !training_session);events_block(CTL);
    training_press(B_HOME);events_block(CTL);assert(!metro_enabled && !training_menu_open);
    puts("training menu: pitch/metronome/unified rhythm game, silent input, audio exit handshake, HOME/back, bounds and shortcut PASS");
    return 0;
}
