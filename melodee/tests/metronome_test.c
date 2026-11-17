/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <assert.h>
static void advance_to(uint32_t at)
{ while((int32_t)(at-metro.now)>0)(void)metro_sample(); }
static void begin(uint32_t game)
{
    metro_settings=metro_default;metro_settings.mode=METRO_GAME;metro_settings.game=(uint8_t)game;
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
    /* A round starts on its first audible guide pulse, without a prepare
     * delay or an extra precount bar before the A/H cycle. */
    begin(METRO_ODDS);
    assert(metro.start==metro.now);
    assert(metro_targets[0].at==metro.start+2u*FS);
    assert(metro_sample()!=0 && metro.state==METRO_AUDIBLE);shot("metro-guide");
    advance_to(metro_targets[0].at);assert(!metro_sample() && metro.state==METRO_RUNNING);shot("metro-hidden");
    /* At 300 BPM on sixteenths, alternate targets are 100 ms apart:
     * their non-overlap cap is just under 50 ms, not under 25 ms. */
    metro_settings.bpm=300;metro_settings.meter=METRO_METER_4_4_SIXTEENTHS;
    metro_restart();assert(metro.window>FS*49u/1000u);
    metro_enabled=0;metro_settings=metro_default;
    /* Real gesture and transport isolation; held keys don't become answers. */
    fm1_in.buttons=1u<<panel.btn[B_HOME];
    assert(metro_ui_input(1u<<panel.btn[B_REC]));fm1_in.buttons=0;
    assert(metro_enabled && !ear_ui_start());metro_commands();assert(metro.state==METRO_RUNNING);
    shot("metro-audible");
    metro_ui_input(1u<<panel.btn[B_PLAY]);metro_commands();assert(metro.state==METRO_STOPPED);
    metro_ui_input(1u<<panel.btn[B_PLAY]);metro_commands();assert(metro.state==METRO_RUNNING);
    metro_settings.offset=-100;metro_settings.meter=METRO_METER_4_4_SIXTEENTHS;
    metro_settings.bpm=300;metro_settings.game=METRO_EVENS;metro_settings.cycle=METRO_CYCLE_2_2;shot("metro-options");
    metro_settings=metro_default;metro_restart();
    /* Quarter-note tap tempo: one tap does nothing, four intervals average. */
    metro.tap_valid=0;metro_settings.bpm=90;metro_tempo_tap();assert(metro_settings.bpm==90);
    for(uint32_t k=0;k<4u;k++) {advance_to(metro.now+FS/2u);metro_tempo_tap();assert(metro_settings.bpm==120);}
    advance_to(metro.now+FS*3u/5u);metro_tempo_tap();assert(metro_settings.bpm==114u);
    advance_to(metro.now+FS*3u);metro_tempo_tap();assert(!metro.tap_count);
    /* Fractional scheduling stays within a sample, across all meters, rates,
     * pulse grids, including a wrapping 32-bit clock. */
    for(uint32_t m=0;m<METRO_METERS_COUNT;m++) {
        metro_settings=metro_default;metro_settings.bpm=137;metro_settings.meter=(uint8_t)m;
        metro.now=0xFFFFF000u;metro_restart();
        for(uint32_t k=0;k<30u;k++) {
            uint32_t expected=metro.start+(uint32_t)(((uint64_t)k*FS*240u)/metro.den);
            assert(metro.next==expected);advance_to(metro.next);(void)metro_sample();
            assert(metro.pulse==k+1u);
        }
    }
    metro_settings=metro_default;metro_restart();advance_to(metro.start);assert(metro_sample()!=0);
    /* Check rendered clicks, including the remaining audible odd-meter
     * pulses, against the rules without using the target predicate. */
    for(uint32_t game=0;game<METRO_GAMES_COUNT;game++)for(uint32_t cycle=0;cycle<METRO_CYCLES_COUNT;cycle++) {
        begin(game);metro_settings.meter=METRO_METER_7_8_223;
        metro_settings.cycle=(uint8_t)cycle;metro_restart();uint32_t span=cycle?2u:1u;
        for(uint32_t bar=0;bar<8u;bar++)for(uint32_t beat=0;beat<7u;beat++) {
            int hidden=bar%(span*2u)>=span;
            int silent=hidden && (game==METRO_FULL_BARS || (game==METRO_ODDS?!(beat&1u):(beat&1u)));
            advance_to(metro.start+(bar*7u+beat)*FS/4u);
            assert((metro_sample()!=0)==!silent);
            assert(metro.state==(hidden?METRO_RUNNING:METRO_AUDIBLE));
        }
    }
    /* Exact taps yield no misses/bias/jitter. Targets are unique and count
     * only silent beats, not heard guide beats. */
    for(uint32_t game=0;game<METRO_GAMES_COUNT;game++)for(uint32_t cycle=0;cycle<METRO_CYCLES_COUNT;cycle++) {
        begin(game);metro_settings.cycle=(uint8_t)cycle;metro_restart();
        uint32_t expected=game==METRO_FULL_BARS?16u:8u;assert(metro.targets==expected);
        /* Independent schedule: immediately 1/1 or 2/2 guide/hidden bars. */
        uint32_t target=0,span=cycle?2u:1u;
        for(uint32_t bar=0;bar<8u;bar++)for(uint32_t beat=0;beat<4u;beat++) {
            int wanted=(bar%(span*2u)>=span) &&
                (game==METRO_FULL_BARS || (game==METRO_ODDS?!(beat&1u):(beat&1u)));
            assert(metro_target(bar+1u,beat,&metro_settings)==wanted);
            if(wanted)assert(metro_targets[target++].at==metro.start+(bar*4u+beat)*FS/2u);
        }
        assert(target==expected);
        metro_tap();assert(!metro.hits && metro.extra==1u);metro_restart();
        for(uint32_t k=0;k<metro.targets;k++) {advance_to(metro_targets[k].at);metro_tap();}
        finish();assert(metro.hits==expected && !metro.misses && !metro.extra && !metro.abs_ms && !metro.bias_ms);
    }
    /* Captured browser times remain accurate despite delivery after the
     * audible event; the grace changes expiration, never the match window. */
    begin(METRO_FULL_BARS);metro.input_grace=FS/2u;
    uint32_t captured=metro_targets[0].at;
    advance_to(captured+FS/4u);metro_tap_at(captured);
    assert(metro.hits==1u && !metro.misses && !metro.abs_ms);
    metro.input_grace=0;finish();
    begin(METRO_FULL_BARS);captured=metro_targets[0].at;
    advance_to(captured+metro.window+2u);assert(metro.misses==1u);
    metro_tap_at(captured);assert(!metro.hits && metro.misses==1u && metro.extra==1u);
    /* Alternate 20 ms early/late; duplicate taps can't inflate a score. */
    begin(METRO_FULL_BARS);
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
     * supported, including the early first answer at the guide boundary. */
    for(int32_t offset=-100;offset<=100;offset+=200) {
        metro_settings=metro_default;metro_settings.mode=METRO_GAME;
        metro_settings.meter=18;metro_settings.offset=(int8_t)offset;metro_restart();
        for(uint32_t k=0;k<metro.targets;k++) {
            advance_to(metro_targets[k].at+(offset*(int32_t)FS/1000));metro_tap();
        }
        advance_to(metro.end+metro.window+FS/5u);assert(metro.state==METRO_RESULT);
        assert(metro.hits==metro.targets && !metro.misses && !metro.abs_ms);
    }
    begin(METRO_FULL_BARS);finish();assert(metro.misses==metro.targets && !metro.hits);
    /* Reported Evens/120 BPM setup retains the full requested window.
     * Early and late taps near either edge hit the same even positions. */
    for(int32_t error=-55;error<=55;error+=110) {
        begin(METRO_EVENS);assert(metro.window==FS*60u/1000u);
        for(uint32_t bar=1;bar<8u;bar+=2u)for(uint32_t beat=1;beat<4u;beat+=2u) {
            uint32_t at=metro.start+(bar*4u+beat)*FS/2u;
            advance_to(at+error*(int32_t)FS/1000);metro_tap();
        }
        finish();assert(metro.hits==8 && !metro.misses && !metro.extra);
    }
    for(uint32_t game=METRO_ODDS;game<=METRO_EVENS;game++) {
        begin(game);
        for(uint32_t bar=1;bar<8u;bar+=2u)for(uint32_t beat=0;beat<4u;beat++)
            if(game==METRO_ODDS?(beat&1u):!(beat&1u)) {
                advance_to(metro.start+(bar*4u+beat)*FS/2u);metro_tap();
            }
        finish();assert(!metro.hits && metro.misses==8 && metro.extra==8);
    }
    /* Worst meter and strict nearest-beat window, late misses, odd missing
     * patterns and the single beat meter all have finite target counts. */
    for(uint32_t m=0;m<METRO_METERS_COUNT;m++)for(uint32_t game=0;game<METRO_GAMES_COUNT;game++)
    for(uint32_t cycle=0;cycle<METRO_CYCLES_COUNT;cycle++) {
        begin(game);metro_settings.bpm=300;metro_settings.meter=(uint8_t)m;
        metro_settings.cycle=(uint8_t)cycle;metro_restart();assert(metro.targets && metro.targets<=METRO_TARGETS_MAX);
        uint32_t beats=metro_meters[metro.cfg.meter].beats;
        assert(metro.targets==4u*(game==METRO_FULL_BARS?beats:game==METRO_ODDS?(beats+1u)/2u:beats/2u));
        uint32_t n=0,span=cycle?2u:1u,window=FS*60u/1000u,last=0;
        for(uint32_t bar=0;bar<8u;bar++)if(bar%(span*2u)>=span)
            for(uint32_t beat=0;beat<beats;beat++)if(game==METRO_FULL_BARS || (game==METRO_ODDS?!(beat&1u):(beat&1u))) {
                uint32_t at=metro.start+(uint32_t)((uint64_t)(bar*beats+beat)*FS*240u/metro.den);
                assert(metro_targets[n].at==at);
                if(n) {uint32_t half=(at-last-1u)/2u;if(half<window)window=half;}
                last=at;n++;
            }
        assert(n==metro.targets && window==metro.window);
        for(uint32_t k=0;k<metro.targets;k++) {advance_to(metro_targets[k].at);metro_tap();}
        finish();assert(metro.hits==metro.targets && !metro.misses);
    }
    /* Local keyboard and MIDI edge ownership: holds/repeated note-ons aren't
     * more taps; same pitch on a separate source is an independent tap. */
    begin(METRO_FULL_BARS);advance_to(metro_targets[0].at);
    fm1_in.notes=1;keyboard_block();assert(metro.hits==1u);keyboard_block();assert(!metro.extra);
    fm1_in.notes=0;keyboard_block();fm1_in.notes=1;keyboard_block();assert(metro.extra==1u);
    ear_midi(0x90,0,60,100,1);ear_midi(0x90,0,60,100,1);assert(metro.extra==2u);
    ear_midi(0x90,0,60,100,2);assert(metro.extra==3u);
    ear_source_lost(1);ear_source_lost(2);fm1_in.notes=0;keyboard_block();
    /* SELECT cannot change the one-page knob mapping. */
    metro_settings=metro_default;metro_settings.mode=METRO_GAME;
    host_enc[panel.enc[EN_SELECT]]=1;host_enc[panel.enc[EN_K1]]=2;metro_ui_input(0);assert(metro_settings.bpm==122u);
    host_enc[panel.enc[EN_K2]]=METRO_METER_4_4_SIXTEENTHS;
    host_enc[panel.enc[EN_K3]]=METRO_EVENS;host_enc[panel.enc[EN_K4]]=METRO_CYCLE_2_2;
    metro_ui_input(0);assert(metro_settings.meter==METRO_METER_4_4_SIXTEENTHS && metro_settings.game==METRO_EVENS && metro_settings.cycle==METRO_CYCLE_2_2);
    metro_preset=METRO_FACTORY_COUNT;metro_ui_input(1u<<panel.btn[B_OCTUP]);assert(metro_user[0].game==METRO_EVENS);
    host_enc[panel.enc[EN_PRESET]]=-METRO_FACTORY_COUNT;metro_ui_input(0);assert(metro_settings.mode==METRO_GAME);
    metro_ui_exit();metro_commands();assert(!metro_enabled && metro.state==METRO_STOPPED && !metro_sample());
    assert(ear_ui_start());ear_publish(EAR_CMD_EXIT);ear_audio_tick(CTL);assert(!ear_enabled);
    puts("metronome: one-page controls, all grids/games/cycles, fractional clock/wrap, tap tempo, scoring, misses/extras and input ownership PASS");
    return 0;
}
