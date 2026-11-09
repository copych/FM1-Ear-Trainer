/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include <assert.h>
static FILE *ear_wav;
static uint32_t ear_frames;
static void save_screen(const char *name, uint16_t expected_bg)
{
    ear_ui_draw();
    assert(host_screen[0]==(uint16_t)((expected_bg>>8)|(expected_bg<<8)));
    char path[160];snprintf(path,sizeof path,"build/ear-baseline/%s.ppm",name);
    FILE *shot=fopen(path,"wb");assert(shot);fprintf(shot,"P6\n240 240\n255\n");
    for(uint32_t px=0;px<240u*240u;px++) {
        uint16_t c=(uint16_t)((host_screen[px]>>8)|(host_screen[px]<<8));
        uint8_t rgb[3]={(uint8_t)(((c>>11)&31u)*255u/31u),(uint8_t)(((c>>5)&63u)*255u/63u),(uint8_t)((c&31u)*255u/31u)};
        fwrite(rgb,1,3,shot);
    }
    fclose(shot);
}
static void audio_ms(uint32_t ms)
{
    int32_t out[2*CTL];
    for(uint32_t i=0;i<(FS/1000u)*ms;i+=CTL) {
        mix_block(out,CTL);
        if(ear_wav) for(uint32_t k=0;k<CTL;k++) wav_put(ear_wav,out[2*k],out[2*k+1]);
        ear_frames+=CTL;
    }
}
int main(void)
{
    ui_power_on();
    song.master_q12=4096; /* UI-only fixture has no physical MASTER ADC. */
    settings.palette=0; palette_set(0);
    fm1_in.buttons=1u<<panel.btn[B_HOME];
    assert(ear_ui_input(1u<<panel.btn[B_PLAY]));
    fm1_in.buttons=0;
    ear_wav=fopen("build/ear-baseline/game-demo.wav","wb");
    assert(ear_wav); wav_hdr(ear_wav,0);
    audio_ms(10);
    assert(ear_enabled && ear.state==EAR_PREPARE && trk[0].engine==ENGI_FM6);
    assert(trk[0].preset==0 && trk[0].p[P_VOICE]==V_POLY && ear.root==60 && ear.target<=72);
    audio_ms(1800);
    assert(ear.state==EAR_ANSWER && ear.attempts==0 && ear.playing==-1);
    ear_ui_draw();
    FILE *shot=fopen("build/ear-baseline/ear-answer.ppm","wb");
    assert(shot); fprintf(shot,"P6\n240 240\n255\n");
    for(uint32_t px=0;px<240u*240u;px++) {
        uint16_t c=(uint16_t)((host_screen[px]>>8)|(host_screen[px]<<8));
        uint8_t rgb[3]={(uint8_t)(((c>>11)&31u)*255u/31u),(uint8_t)(((c>>5)&63u)*255u/63u),(uint8_t)((c&31u)*255u/31u)};
        fwrite(rgb,1,3,shot);
    }
    fclose(shot);
    /* A wrong built-in key, replay same exercise, then correct target. */
    fm1_in.notes=1u<<7; audio_ms(10);
    assert(ear.state==EAR_WRONG && ear.attempts==1 && ear.player==60);
    save_screen("ear-wrong",RGB(148,32,42));
    fm1_in.notes=0; audio_ms(10);
    assert(ear.player==-1);
    uint32_t locked_age=vage;
    for(uint32_t k=0;k<27u;k++) {
        fm1_in.notes=1u<<k;audio_ms(2);fm1_in.notes=0;audio_ms(2);
        assert(ear.state==EAR_WRONG && ear.player==-1 && ear.attempts==1);
    }
    midi_event_source=1;
    for(uint32_t note=48;note<=84;note++) {
        midi_event(0x90u,0,note,100);
        assert(ear.state==EAR_WRONG && ear.player==-1 && ear.attempts==1);
        midi_event(0x80u,0,note,0);
    }
    assert(vage==locked_age); /* No new voices while searching the scale. */
    uint32_t target=ear.target, serial=ear.serial;
    assert(ear.need[ear.degree]==1u && ear.score==0u);
    audio_ms(1500);
    assert(ear.state!=EAR_WRONG && ear.serial==serial && ear.target==target && ear.attempts==1u && ear.replays==0u);
    audio_ms(1800);assert(ear.state==EAR_ANSWER && ear.armed);
    ear_publish(EAR_CMD_RETRY);audio_ms(1800);
    assert(ear.state==EAR_ANSWER && ear.target==target && ear.serial==serial);
    fm1_in.notes=1u<<(target-53u);audio_ms(10);
    assert(ear.state==EAR_CORRECT && ear.attempts==2);
    assert(ear.last_award==60u && ear.score==60u && !ear.streak);
    save_screen("ear-correct",RGB(16,104,55));
    audio_ms(800);assert(ear.state==EAR_CORRECT && ear.serial==serial);
    audio_ms(300);assert(ear.serial==serial+1u && !ear.armed && ear.player==-1);
    audio_ms(1800);assert(ear.state==EAR_ANSWER && !ear.armed && !ear.attempts);
    fm1_in.notes=0;audio_ms(10);
    assert(ear.armed);
    /* A key held during playback never becomes an answer at the boundary. */
    ear_publish(EAR_CMD_NEXT);audio_ms(10);
    fm1_in.notes=1u<<(ear.target-53u);audio_ms(1800);
    assert(ear.state==EAR_ANSWER && !ear.armed && !ear.attempts);
    fm1_in.notes=0;audio_ms(10);
    assert(ear.armed);
    midi_event_source=1;
    midi_event(0x90u,0,ear.target,100);
    assert(ear.state==EAR_CORRECT && ear.attempts==1);
    assert(ear.last_award==100u && ear.score==160u && ear.streak==1u);
    midi_event(0x90u,0,ear.target,0);
    assert(ear.player==-1);
    /* Duplicate notes, separate channels and port loss preserve release gating. */
    midi_event(0x90u,1,ear.target,100);
    midi_event(0x90u,1,ear.target,100);
    assert(ear_down[1]==1u);
    ear_source_lost(1u);
    assert(!ear_down[1] && ear.player==-1);
    /* Retry/next interrupt generated playback and release its note. */
    serial=ear.serial;
    ear_publish(EAR_CMD_RETRY);audio_ms(250);
    assert(ear.serial==serial+1u && ear.attempts==0u && ear.score==160u);
    assert(ear.playing==60);
    ear_publish(EAR_CMD_EXIT);audio_ms(10);
    assert(!ear_enabled && ear.state==EAR_OFF && ear.playing==-1 && ear.player==-1);
    for(uint32_t cycle=0;cycle<3u;cycle++) {
        assert(ear_ui_start()); audio_ms(1800);
        assert(ear.state==EAR_ANSWER && !song.playing);
        assert(ear_ui_start()); audio_ms(1800); /* active start is an idempotent retry */
        assert(ear.state==EAR_ANSWER && !song.playing);
        ear_ui_input(1u<<panel.btn[B_HOME]);audio_ms(20);
        assert(!ear_enabled && !song.playing);
    }
    rewind(ear_wav); wav_hdr(ear_wav,ear_frames); fclose(ear_wav); ear_wav=0;
    assert(ear_points(1,0)==100 && ear_points(2,0)==70 && ear_points(3,0)==40);
    assert(ear_points(4,0)==10 && ear_points(5,0)==0 && ear_points(1,1)==90 && ear_points(3,4)==0);
    assert(ear_intervals[EAR_DEGREE_SEVENTH]==11u);
    uint32_t baseline[EAR_DEGREES_COUNT]={0},adapted[EAR_DEGREES_COUNT]={0};
    memset(ear.need,0,sizeof ear.need);ear.seed=12345;
    for(uint32_t k=0;k<7000u;k++) baseline[ear_pick_degree()]++;
    ear.need[EAR_DEGREE_SEVENTH]=6u;ear.seed=12345;
    for(uint32_t k=0;k<7000u;k++) adapted[ear_pick_degree()]++;
    for(uint32_t k=0;k<EAR_DEGREES_COUNT;k++) {
        assert(baseline[k]>800u && adapted[k]>400u);
        printf("degree %u: uniform %u/7000, adapted %u/7000\n",k+2u,baseline[k],adapted[k]);
    }
    assert(adapted[EAR_DEGREE_SEVENTH]>2u*baseline[EAR_DEGREE_SEVENTH]);
    assert(adapted[EAR_DEGREE_SEVENTH]<3100u);
    /* Exercise real knob ownership, clamping, both scales and all count/guess combinations. */
    assert(ear_ui_start());audio_ms(1800);
    host_enc[panel.enc[EN_K1]]=panel.dir[EN_K1];ear_ui_input(0);audio_ms(10);
    assert(ear_settings.scale==EAR_MINOR && ear.root==57u);
    host_enc[panel.enc[EN_K2]]=99*panel.dir[EN_K2];
    host_enc[panel.enc[EN_K3]]=99*panel.dir[EN_K3];
    host_enc[panel.enc[EN_K4]]=99*panel.dir[EN_K4];ear_ui_input(0);audio_ms(10);
    assert(ear.count==4u && ear.guess==4u && ear.gap==1000u);
    audio_ms(5500);assert(ear.state==EAR_ANSWER);
    save_screen("ear-four-hidden",T_BG);
    host_enc[panel.enc[EN_K2]]=-99*panel.dir[EN_K2];
    host_enc[panel.enc[EN_K4]]=-99*panel.dir[EN_K4];ear_ui_input(0);audio_ms(10);
    assert(ear.count==1u && ear.guess==1u && ear.gap==0u);
    for(uint32_t scale=0;scale<EAR_SCALES_COUNT;scale++)
      for(uint32_t count=1;count<=4;count++) for(uint32_t guess=1;guess<=count;guess++) {
        ear_settings.scale=(uint8_t)scale;ear_settings.count=(uint8_t)count;
        ear_settings.guess=(uint8_t)guess;ear_settings.gap=0;
        ear_publish(EAR_CMD_NEXT);audio_ms(2600);
        assert(ear.state==EAR_ANSWER && ear.armed);
        for(uint32_t k=0;k<count;k++) {
            assert(ear.notes[k]>=53 && ear.notes[k]<=79);
            uint32_t offset=ear.notes[k]-ear.root;
            uint32_t valid=offset==0;
            for(uint32_t d=0;d<EAR_DEGREES_COUNT;d++)
                valid|=offset==(scale==EAR_MINOR?ear_minor_intervals[d]:ear_intervals[d]);
            assert(valid);
        }
        if(guess<count) assert(ear.notes[0]==ear.root);
        else {
            uint32_t first=ear.notes[0];ear_publish(EAR_CMD_NEXT);audio_ms(2600);
            assert(ear.notes[0]!=first);
        }
        uint32_t points=ear.score;
        for(uint32_t k=count-guess;k<count;k++) {
            fm1_in.notes=1u<<(ear.notes[k]-53u);audio_ms(10);
            assert(ear.attempts==1u);
            if(k+1<count) assert(ear.state==EAR_ANSWER && !ear.armed && ear.score==points);
            fm1_in.notes=0;audio_ms(10);
        }
        assert(ear.state==EAR_CORRECT && ear.last_award==100u && ear.score==points+100u);
      }
    /* Wrong second submitted note locks the whole multi-note attempt and repeats it. */
    ear_settings.count=4;ear_settings.guess=3;ear_settings.gap=0;
    ear_publish(EAR_CMD_NEXT);audio_ms(2600);
    fm1_in.notes=1u<<(ear.notes[1]-53u);audio_ms(10);fm1_in.notes=0;audio_ms(10);
    assert(ear.entered==1u && ear.attempts==1u);
    midi_event(0x90u,0,52,100);assert(ear.state==EAR_WRONG && ear.attempts==1u);
    midi_event(0x80u,0,52,0);serial=ear.serial;target=ear.target;
    audio_ms(3600);assert(ear.state==EAR_ANSWER && ear.serial==serial && ear.target==target && !ear.entered);
    save_screen("ear-four-mixed",T_BG);
    /* Sustained arpeggio: earlier gates stay open as each new note enters. */
    ear_settings.count=4;ear_settings.guess=1;ear_settings.gap=250;
    ear_publish(EAR_CMD_NEXT);audio_ms(10);
    ear.notes[0]=60;ear.notes[1]=64;ear.notes[2]=67;ear.notes[3]=71;
    audio_ms(210);assert(ear.sounding==1u);
    assert(ear_ui_key_lights()==(1u<<7));
    audio_ms(260);assert(ear.sounding==3u);
    audio_ms(260);assert(ear.sounding==7u);
    assert(ear_ui_key_lights()==((1u<<7)|(1u<<11)|(1u<<14)));
    save_screen("ear-sustained-known",T_BG);
    audio_ms(260);assert(ear.sounding==15u);
    assert(ear_ui_key_lights()==((1u<<7)|(1u<<11)|(1u<<14))); /* B4 stays dark. */
    uint32_t gates=0;for(uint32_t k=0;k<NVOICE;k++) gates+=trk[0].v[k].gate!=0;
    assert(gates==4u);
    audio_ms(800);assert(ear.sounding==15u);
    audio_ms(300);assert(!ear.sounding && ear.playing==-1 && ear.state==EAR_ANSWER);
    assert(!ear_ui_key_lights());
    for(uint32_t k=0;k<NVOICE;k++) assert(!trk[0].v[k].gate);
    /* Zero spacing launches a chord in a single control tick. */
    ear_settings.gap=0;ear_publish(EAR_CMD_NEXT);audio_ms(10);
    ear.notes[0]=60;ear.notes[1]=64;ear.notes[2]=67;ear.notes[3]=71;
    audio_ms(210);assert(ear.sounding==15u);
    ear.guess=ear.count;assert(!ear_ui_key_lights());
    ear_publish(EAR_CMD_EXIT);audio_ms(10);assert(!ear.sounding && ear.playing==-1);
    for(uint32_t k=0;k<NVOICE;k++) assert(!trk[0].v[k].gate);
    printf("FM6 part state: %zu bytes (host layout; target runtime headroom unmeasured)\n",sizeof(fm6_part_t));
    puts("ear trainer: FM6, keyboard/MIDI, wrong/retry/correct, hold gating and cancellation passed");
    return 0;
}
