/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <assert.h>
static void advance_to(uint32_t at)
{ while((int32_t)(at-metro.now)>0)(void)metro_sample(); }
static void begin(uint32_t mode)
{
    metro_settings=metro_default;metro_settings.mode=(uint8_t)mode;
    metro_enabled=1;metro_restart();
}
static void finish(void)
{ advance_to(metro.end+metro.window+2u);assert(metro.state==METRO_RESULT); }
static void shot(const char *name)
{
    ui.force=1;metro_ui_draw();char path[160];
    snprintf(path,sizeof path,"build/ear-baseline/%s.ppm",name);
    FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 240\n255\n");
    for(uint32_t i=0;i<240u*240u;i++) {
        uint16_t c=(uint16_t)((host_screen[i]>>8)|(host_screen[i]<<8));
        uint8_t p[3]={(uint8_t)(((c>>11)&31u)*255u/31u),(uint8_t)(((c>>5)&63u)*255u/63u),(uint8_t)((c&31u)*255u/31u)};
        fwrite(p,1,3,f);
    }
    fclose(f);
}
int main(void)
{
    ui_power_on();song.master_q12=4096;settings.palette=0;palette_set(0);
    /* Real gesture and transport isolation; held keys don't become answers. */
    fm1_in.buttons=1u<<panel.btn[B_HOME];
    assert(metro_ui_input(1u<<panel.btn[B_REC]));fm1_in.buttons=0;
    assert(metro_enabled && !ear_ui_start());metro_commands();assert(metro.state==METRO_COUNTIN);
    shot("metro-countin");
    metro_ui_input(1u<<panel.btn[B_PLAY]);metro_commands();assert(metro.state==METRO_STOPPED);
    metro_ui_input(1u<<panel.btn[B_PLAY]);metro_commands();assert(metro.state==METRO_COUNTIN);
    metro_settings.offset=-100;metro_settings.meter=17;metro_settings.bpm=300;metro_page=1;shot("metro-options");
    metro_settings=metro_default;metro_page=0;metro_restart();
    /* Quarter-note tap tempo: one tap does nothing, four intervals average. */
    metro.tap_valid=0;metro_settings.bpm=90;metro_tempo_tap();assert(metro_settings.bpm==90);
    for(uint32_t k=0;k<4u;k++) {advance_to(metro.now+FS/2u);metro_tempo_tap();assert(metro_settings.bpm==120);}
    advance_to(metro.now+FS*3u/5u);metro_tempo_tap();assert(metro_settings.bpm==114u);
    advance_to(metro.now+FS*3u);metro_tempo_tap();assert(!metro.tap_count);
    /* Fractional scheduling stays within a sample, across all meters, rates,
     * patterns and subdivisions, including a wrapping 32-bit clock. */
    for(uint32_t m=0;m<METRO_METERS_COUNT;m++)for(uint32_t p=0;p<METRO_PATTERNS_COUNT;p++)
    for(uint32_t sub=1;sub<=4u;sub++) {
        metro_settings=metro_default;metro_settings.bpm=137;metro_settings.meter=(uint8_t)m;
        metro_settings.pattern=(uint8_t)p;metro_settings.sub=(uint8_t)sub;
        metro.now=0xFFFFF000u;metro_restart();
        for(uint32_t k=0;k<30u;k++) {
            uint32_t expected=metro.start+(uint32_t)(((uint64_t)k*FS*240u)/metro.den);
            assert(metro.next==expected);advance_to(metro.next);(void)metro_sample();
            assert(metro.pulse==k+1u);
        }
    }
    begin(METRO_PRACTICE);advance_to(metro.start);assert(metro_sample()!=0);
    metro_settings.volume=0;metro_restart();advance_to(metro.start);
    for(uint32_t k=0;k<1000u;k++)assert(metro_sample()==0);
    /* Exact taps yield no misses/bias/jitter. Targets are unique and count
     * only silent beats, not heard guide beats or count-in. */
    for(uint32_t mode=METRO_KEEP;mode<=METRO_MISSING;mode++) {
        begin(mode);assert(metro.targets==16u);
        metro_tap();assert(!metro.hits && !metro.extra);
        for(uint32_t k=0;k<metro.targets;k++) {advance_to(metro_targets[k].at);metro_tap();}
        finish();assert(metro.hits==16u && !metro.misses && !metro.extra && !metro.abs_ms && !metro.bias_ms);
    }
    /* Alternate 20 ms early/late; duplicate taps can't inflate a score. */
    begin(METRO_MISSING);
    for(uint32_t k=0;k<metro.targets;k++) {
        advance_to(metro_targets[k].at+(k&1u?FS/50u:0u-FS/50u));metro_tap();
        metro_tap();
    }
    finish();assert(metro.hits==16u && metro.extra==16u && !metro.misses);
    assert(metro.abs_ms==320u && metro.square_ms==6400u && !metro.bias_ms);
    assert(metro_sqrt(metro.square_ms/metro.hits)==20u);
    assert(metro_deviation(2,119,59u*59u+60u*60u)==0u);
    assert(metro_deviation(2,-119,59u*59u+60u*60u)==0u);
    assert(metro_deviation(2,0,800)==20u);
    assert(metro_grade(16,16,0,0,FS*60u/1000u)==100u);
    assert(metro_grade(16,16,0,320,FS*60u/1000u)==66u);
    assert(metro_grade(16,16,16,320,FS*60u/1000u)==33u);
    shot("metro-result");
    /* Positive latency correction extends judgment deadlines; a delayed tap
     * must not already have been counted as missed. Negative correction is
     * supported, including the early first answer across the count-in. */
    for(int32_t offset=-100;offset<=100;offset+=200) {
        metro_settings=metro_default;metro_settings.mode=METRO_MISSING;
        metro_settings.meter=18;metro_settings.offset=(int8_t)offset;metro_restart();
        for(uint32_t k=0;k<metro.targets;k++) {
            advance_to(metro_targets[k].at+(offset*(int32_t)FS/1000));metro_tap();
        }
        advance_to(metro.end+metro.window+FS/5u);assert(metro.state==METRO_RESULT);
        assert(metro.hits==metro.targets && !metro.misses && !metro.abs_ms);
    }
    begin(METRO_KEEP);finish();assert(metro.misses==metro.targets && !metro.hits);
    /* Worst meter and strict nearest-beat window, late misses, odd missing
     * patterns and the single beat meter all have finite target counts. */
    for(uint32_t m=0;m<METRO_METERS_COUNT;m++)for(uint32_t mode=METRO_KEEP;mode<=METRO_MISSING;mode++) {
        metro_settings=metro_default;metro_settings.mode=(uint8_t)mode;
        metro_settings.bpm=300;metro_settings.meter=(uint8_t)m;metro_settings.tolerance=150;
        metro_settings.silent=4;metro_restart();assert(metro.targets && metro.targets<=METRO_TARGETS_MAX);
        for(uint32_t k=0;k<metro.targets;k++) {advance_to(metro_targets[k].at);metro_tap();}
        finish();assert(metro.hits==metro.targets && !metro.misses);
    }
    /* Local keyboard and MIDI edge ownership: holds/repeated note-ons aren't
     * more taps; same pitch on a separate source is an independent tap. */
    begin(METRO_MISSING);advance_to(metro_targets[0].at);
    fm1_in.notes=1;keyboard_block();assert(metro.hits==1u);keyboard_block();assert(!metro.extra);
    fm1_in.notes=0;keyboard_block();fm1_in.notes=1;keyboard_block();assert(metro.extra==1u);
    ear_midi(0x90,0,60,100,1);ear_midi(0x90,0,60,100,1);assert(metro.extra==2u);
    ear_midi(0x90,0,60,100,2);assert(metro.extra==3u);
    ear_source_lost(1);ear_source_lost(2);fm1_in.notes=0;keyboard_block();
    /* Real controls select settings page, save a personal preset and load it. */
    metro_page=0;host_enc[panel.enc[EN_SELECT]]=1;metro_ui_input(0);assert(metro_page==1u);
    host_enc[panel.enc[EN_K1]]=2;metro_ui_input(0);assert(metro_settings.sub==3u);
    metro_preset=METRO_FACTORY_COUNT;metro_ui_input(1u<<panel.btn[B_OCTUP]);assert(metro_user[0].sub==3u);
    metro_ui_exit();metro_commands();assert(!metro_enabled && metro.state==METRO_STOPPED && !metro_sample());
    assert(ear_ui_start());ear_publish(EAR_CMD_EXIT);ear_audio_tick(CTL);assert(!ear_enabled);
    puts("metronome: gestures, fractional clock/wrap, tap tempo, all meters/patterns/subdivisions, gain, scoring, misses/extras and input ownership PASS");
    return 0;
}
