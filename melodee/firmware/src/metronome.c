/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Included by mix.c. Audio owns the clock, click and judgments. Main publishes
 * complete settings + a command with IRQs masked. No engine/transport clock,
 * allocation, storage, floating point or library math in the render path. */
/* Explicit stable wire IDs; adding/reordering options requires codec review. */
enum eMetroMode { METRO_PRACTICE=0, METRO_GAME=1, METRO_MODES_COUNT };
enum eMetroGame { METRO_FULL_BARS=0, METRO_ODDS=1, METRO_EVENS=2, METRO_GAMES_COUNT };
enum eMetroCycle { METRO_CYCLE_1_1=0, METRO_CYCLE_2_2=1, METRO_CYCLES_COUNT };
enum eMetroMeter { METRO_METER_4_4=0, METRO_METER_2_4=1, METRO_METER_3_4=2,
    METRO_METER_6_8=3, METRO_METER_9_8=4, METRO_METER_12_8=5,
    METRO_METER_5_8_23=6, METRO_METER_5_8_32=7, METRO_METER_7_8_223=8,
    METRO_METER_7_8_322=9, METRO_METER_7_8_232=10, METRO_METER_5_4=11,
    METRO_METER_7_4=12, METRO_METER_11_8=13, METRO_METER_13_8=14,
    METRO_METER_15_8=15, METRO_METER_3_2=16, METRO_METER_7_16=17,
    METRO_METER_1_4=18, METRO_METER_4_4_EIGHTHS=19,
    METRO_METER_4_4_TRIPLETS=20, METRO_METER_4_4_SIXTEENTHS=21, METRO_METERS_COUNT };
enum eMetroCommand { METRO_CMD_NONE, METRO_CMD_RESTART, METRO_CMD_TOGGLE,
    METRO_CMD_EXIT, METRO_CMD_TAP, METRO_COMMANDS_COUNT };
enum eMetroState { METRO_STOPPED, METRO_AUDIBLE, METRO_RUNNING, METRO_RESULT, METRO_STATES_COUNT };
enum eMetroTargetState { METRO_TARGET_PENDING, METRO_TARGET_HIT, METRO_TARGET_MISSED, METRO_TARGET_STATES_COUNT };
enum eMetroFeedback { METRO_FEEDBACK_NONE, METRO_FEEDBACK_GOOD, METRO_FEEDBACK_BAD, METRO_FEEDBACKS_COUNT };
typedef struct { const char *name,*groups; uint8_t beats,den; uint16_t accents; } metro_meter_t;
/* Accents identify group starts, including different ways to hear 5 and 7. */
static const metro_meter_t metro_meters[]={
    {"4/4","1+1+1+1",4,4,1}, {"2/4","1+1",2,4,1}, {"3/4","1+1+1",3,4,1},
    {"6/8","3+3",6,8,9}, {"9/8","3+3+3",9,8,73}, {"12/8","3+3+3+3",12,8,585},
    {"5/8","2+3",5,8,5}, {"5/8","3+2",5,8,9},
    {"7/8","2+2+3",7,8,21}, {"7/8","3+2+2",7,8,41}, {"7/8","2+3+2",7,8,37},
    {"5/4","3+2",5,4,9}, {"7/4","4+3",7,4,17},
    {"11/8","3+3+3+2",11,8,585}, {"13/8","3+3+3+2+2",13,8,2633},
    {"15/8","3+3+3+3+3",15,8,4681}, {"3/2","1+1+1",3,2,1},
    {"7/16","2+2+3",7,16,21}, {"1/4","1",1,4,1},
    /* Pulse-grid variants: subdivision stays one; the grid defines the
     * numerator/denominator, duration and quarter-note accents directly. */
    {"4/4e","4/4 EIGHTHS",8,8,85}, {"4/4t","4/4 TRIPLETS",12,12,585},
    {"4/4s","4/4 SIXTEENTHS",16,16,4369}
};
_Static_assert(METRO_METERS_COUNT==sizeof metro_meters/sizeof metro_meters[0],"meter ID/table mismatch");
#define METRO_USER_COUNT 4u
#define METRO_RECORD_BYTES 12u
#define METRO_PREFS_BYTES (METRO_RECORD_BYTES*(METRO_USER_COUNT+1u))
#define METRO_TARGETS_MAX 120u
#define METRO_GAME_BARS 8u
_Static_assert(METRO_GAME_BARS%4u==0u && METRO_TARGETS_MAX>=64u && METRO_TARGETS_MAX<=255u,"review cycle target bounds");
_Static_assert(METRO_GAME_BARS*16u<=0xFFFFFFFFu/(FS*240u),"sample schedule multiplication overflows");
enum { METRO_CLICK_VOLUME=100, METRO_SUBDIVISION=1, METRO_WINDOW_MS=60,
       METRO_LEGACY_METERS=19, METRO_LEGACY_PATTERNS=4, METRO_LEGACY_MODES=3 };
typedef struct { uint16_t bpm; uint8_t meter,mode,game,cycle; int8_t offset; } metro_settings_t;
static const metro_settings_t metro_default={120,METRO_METER_4_4,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0};
static metro_settings_t metro_settings={120,METRO_METER_4_4,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0};
static metro_settings_t metro_user[METRO_USER_COUNT]; /* zero BPM = empty */
static const struct { const char *name; metro_settings_t settings; } metro_factory[]={
    {"Quarter notes",{120,METRO_METER_4_4,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Eighth notes",{100,METRO_METER_4_4_EIGHTHS,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Triplets",{90,METRO_METER_4_4_TRIPLETS,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Sixteenths",{80,METRO_METER_4_4_SIXTEENTHS,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"March",{110,METRO_METER_2_4,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Waltz",{100,METRO_METER_3_4,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Compound six",{90,METRO_METER_6_8,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Five: 2+3",{100,METRO_METER_5_8_23,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Seven: 2+2+3",{100,METRO_METER_7_8_223,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Eleven",{100,METRO_METER_11_8,METRO_PRACTICE,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Full bars 1/1",{100,METRO_METER_4_4,METRO_GAME,METRO_FULL_BARS,METRO_CYCLE_1_1,0}},
    {"Full bars 2/2",{100,METRO_METER_4_4,METRO_GAME,METRO_FULL_BARS,METRO_CYCLE_2_2,0}},
    {"Odds 1/1",{100,METRO_METER_4_4,METRO_GAME,METRO_ODDS,METRO_CYCLE_1_1,0}},
    {"Evens 1/1",{100,METRO_METER_4_4,METRO_GAME,METRO_EVENS,METRO_CYCLE_1_1,0}},
    {"Odds 2/2",{100,METRO_METER_7_8_223,METRO_GAME,METRO_ODDS,METRO_CYCLE_2_2,0}},
    {"Evens 2/2",{100,METRO_METER_7_8_223,METRO_GAME,METRO_EVENS,METRO_CYCLE_2_2,0}}
};
#define METRO_FACTORY_COUNT (sizeof metro_factory/sizeof metro_factory[0])
static volatile uint8_t metro_enabled,metro_request;
static struct {
    metro_settings_t cfg;
    uint32_t now,start,next,period,rem,den,carry,pulse,end,tap_last,tap_sum,tap_intervals[4];
    uint32_t window,version,abs_ms,square_ms,input_grace,feedback_at;
    int32_t bias_ms,last_ms,input_delay;
    uint16_t extra,hits,misses,click_left,click_phase,tap_left,tap_phase;
    uint8_t state,beat,bar,judge,targets,tap_count,tap_valid,tap_index,click_pitch,click_gain,feedback;
} metro;
static struct { uint32_t at; uint8_t hit; } metro_targets[METRO_TARGETS_MAX] __attribute__((section(".pool")));
static uint8_t metro_crc(const uint8_t *p)
{
    uint32_t c=0;
    for(uint32_t k=0;k<METRO_RECORD_BYTES-1u;k++) {
        c^=p[k];for(uint32_t b=0;b<8u;b++)c=((c<<1)^((c&128u)?7u:0u))&255u;
    }
    return (uint8_t)c;
}
static void metro_normalize(metro_settings_t *s)
{
    /* An even pulse needs at least two positions. Keep this degenerate
     * one-pulse meter playable when selecting the Evens rule. */
    if(s->mode==METRO_GAME && s->game==METRO_EVENS && metro_meters[s->meter].beats<2u)
        s->meter=METRO_METER_2_4;
}
static void metro_pack(uint8_t *p,const metro_settings_t *s)
{
    p[0]='M';p[1]=2;p[2]=(uint8_t)s->bpm;p[3]=(uint8_t)(s->bpm>>8);
    p[4]=s->meter;p[5]=(uint8_t)(s->game|(s->mode<<4));p[6]=METRO_SUBDIVISION;p[7]=(uint8_t)(s->offset+100);
    p[8]=METRO_CLICK_VOLUME;p[9]=s->cycle;p[10]=METRO_WINDOW_MS;p[11]=metro_crc(p);
}
static int metro_unpack(metro_settings_t *s,const uint8_t *p)
{
    uint32_t bpm=p[2]|((uint32_t)p[3]<<8);
    if(p[0]!='M' || (p[1]!=1u && p[1]!=2u) || p[11]!=metro_crc(p) || bpm<30u || bpm>300u || p[7]>200u)return 0;
    metro_settings_t next=metro_default;
    next.bpm=(uint16_t)bpm;next.meter=p[4];next.offset=(int8_t)((int32_t)p[7]-100);
    if(p[1]==1u) {
        if(p[4]>=METRO_LEGACY_METERS || (p[5]&15u)>=METRO_LEGACY_PATTERNS || (p[5]>>4)>=METRO_LEGACY_MODES ||
           p[6]<1u || p[6]>4u || p[8]>100u || p[9]<1u || p[9]>4u || p[10]<10u || p[10]>150u)return 0;
        next.mode=(p[5]>>4)?METRO_GAME:METRO_PRACTICE;
        next.game=(p[5]>>4)==2u?METRO_EVENS:METRO_FULL_BARS;
        next.cycle=p[9]>1u?METRO_CYCLE_2_2:METRO_CYCLE_1_1;
        if(next.meter==METRO_METER_4_4 && !(p[5]&15u) && p[6]>1u)
            next.meter=(uint8_t)(METRO_METER_4_4_EIGHTHS+p[6]-2u);
        if(next.meter==METRO_METER_1_4)next.game=METRO_FULL_BARS;
    } else {
        if(p[4]>=METRO_METERS_COUNT || (p[5]&15u)>=METRO_GAMES_COUNT || (p[5]>>4)>=METRO_MODES_COUNT ||
           p[6]!=METRO_SUBDIVISION || p[8]!=METRO_CLICK_VOLUME || p[9]>=METRO_CYCLES_COUNT || p[10]!=METRO_WINDOW_MS)return 0;
        next.mode=p[5]>>4;next.game=p[5]&15u;next.cycle=p[9];
    }
    metro_normalize(&next);*s=next;return 1;
}
static int metro_hidden_bar(uint32_t bar,const metro_settings_t *s)
{
    uint32_t bars=s->cycle==METRO_CYCLE_2_2?2u:1u;
    return bar && bar<=METRO_GAME_BARS && (bar-1u)%(bars*2u)>=bars;
}
static int metro_target(uint32_t bar,uint32_t beat,const metro_settings_t *s)
{
    if(s->mode!=METRO_GAME || !metro_hidden_bar(bar,s))return 0;
    return s->game==METRO_FULL_BARS || (s->game==METRO_ODDS?!(beat&1u):!!(beat&1u));
}
/* Main/UI-only result calculation. Timing credit falls linearly to zero at
 * the matching window; misses and extra taps enlarge the denominator. */
static uint32_t metro_grade(uint32_t hits,uint32_t targets,uint32_t extra,uint32_t abs_ms,uint32_t window)
{
    uint32_t ms=window*1000u/FS;
    uint32_t credit=hits*ms>abs_ms?hits*ms-abs_ms:0;
    return targets && ms?100u*credit/((targets+extra)*ms):0;
}
/* All timing uses wrapping sample counters; every run is <2^31 samples. */
static void metro_restart(void)
{
    metro.cfg=metro_settings;
    metro_normalize(&metro.cfg);
    metro.input_delay=(int32_t)metro.cfg.offset*(int32_t)FS/1000;
    const metro_meter_t *m=&metro_meters[metro.cfg.meter];
    metro.start=metro.now;metro.next=metro.start;metro.pulse=metro.carry=0;
    metro.den=(uint32_t)metro.cfg.bpm*m->den;
    metro.period=FS*240u/metro.den;metro.rem=FS*240u%metro.den;
    /* Cap by actual neighboring targets, not the intervening audible pulses.
     * Odd meters can put two odd targets next to one another across hidden
     * bar boundaries, so use the generated schedule rather than a multiplier. */
    metro.window=FS*METRO_WINDOW_MS/1000u;
    metro.targets=metro.judge=0;metro.hits=metro.extra=metro.misses=0;
    metro.abs_ms=metro.square_ms=0;metro.bias_ms=metro.last_ms=0;
    uint32_t den=(uint32_t)metro.cfg.bpm*m->den;
    for(uint32_t b=1;b<=METRO_GAME_BARS;b++)for(uint32_t k=0;k<m->beats;k++)if(metro_target(b,k,&metro.cfg)) {
        uint32_t i=metro.targets++;
        metro_targets[i].at=metro.start+(((b-1u)*m->beats+k)*(FS*240u))/den;metro_targets[i].hit=METRO_TARGET_PENDING;
        if(i) {
            uint32_t half=(metro_targets[i].at-metro_targets[i-1u].at-1u)/2u;
            if(metro.window>half)metro.window=half;
        }
    }
    metro.end=metro.start+(METRO_GAME_BARS*m->beats*(FS*240u))/den;
    metro.state=metro.cfg.mode==METRO_PRACTICE?METRO_RUNNING:METRO_AUDIBLE;
    metro.beat=0;metro.bar=1;metro.click_left=metro.tap_left=0;metro.feedback=METRO_FEEDBACK_NONE;metro.version++;
}
/* Native input uses now; the browser supplies the captured audible sample
 * time so device/output latency and message delivery don't become tap error. */
static void metro_tap_at(uint32_t input_time)
{
    if(metro.state!=METRO_RUNNING && metro.state!=METRO_AUDIBLE)return;
    /* Input-owned edge, never an automatically scheduled beat. Separate
     * envelope keeps a simultaneous guide click and player tap audible. */
    metro.tap_left=384;metro.tap_phase=0;metro.feedback_at=metro.now;
    metro.feedback=METRO_FEEDBACK_GOOD;metro.version++;
    if(metro.cfg.mode==METRO_PRACTICE)return;
    uint32_t tapped=input_time-metro.input_delay;
    uint32_t best=METRO_TARGETS_MAX,distance=metro.window+1u;
    for(uint32_t k=0;k<metro.targets;k++)if(metro_targets[k].hit==METRO_TARGET_PENDING) {
        int32_t d=(int32_t)(tapped-metro_targets[k].at);
        uint32_t a=(uint32_t)(d<0?-d:d);
        if(a<distance) {best=k;distance=a;}
    }
    if(best==METRO_TARGETS_MAX) {if(metro.extra<999u)metro.extra++;metro.feedback=METRO_FEEDBACK_BAD;return;}
    metro_targets[best].hit=METRO_TARGET_HIT;metro.hits++;
    int32_t d=(int32_t)(tapped-metro_targets[best].at);
    int32_t ms=d*1000/(int32_t)FS;
    metro.last_ms=ms;metro.bias_ms+=ms;metro.abs_ms+=(uint32_t)(ms<0?-ms:ms);
    metro.square_ms+=(uint32_t)(ms*ms);metro.version++;
}
static void metro_tap(void) {metro_tap_at(metro.now);}
/* Tap tempo averages the last four intervals; long gaps reset, no isolated
 * tap changes BPM. Audio publishes BPM; main observes it and defers saving. */
static void metro_tempo_tap(void)
{
    uint32_t d=metro.now-metro.tap_last;metro.tap_last=metro.now;
    if(!metro.tap_valid || d>FS*2u || d<FS/5u) {
        metro.tap_valid=1;metro.tap_count=0;metro.tap_sum=0;metro.tap_index=0;return;
    }
    if(metro.tap_count==4u)metro.tap_sum-=metro.tap_intervals[metro.tap_index];
    else metro.tap_count++;
    metro.tap_intervals[metro.tap_index]=d;metro.tap_index=(uint8_t)((metro.tap_index+1u)&3u);
    metro.tap_sum+=d;
    uint32_t bpm=(FS*60u*metro.tap_count+metro.tap_sum/2u)/metro.tap_sum;
    metro_settings.bpm=(uint16_t)(bpm<30u?30u:bpm>300u?300u:bpm);
    if(metro.state!=METRO_STOPPED && metro.state!=METRO_RESULT)metro_restart();
    metro.version++;
}
/* Called before input dispatch each control block. */
static void metro_commands(void)
{
    uint32_t c=metro_request;metro_request=METRO_CMD_NONE;
    if(c==METRO_CMD_EXIT) {metro.state=METRO_STOPPED;metro.click_left=metro.tap_left=0;metro.feedback=METRO_FEEDBACK_NONE;metro_enabled=0;metro.tap_valid=0;}
    else if(c==METRO_CMD_RESTART)metro_restart();
    else if(c==METRO_CMD_TOGGLE) {
        if(metro.state==METRO_AUDIBLE || metro.state==METRO_RUNNING) {metro.state=METRO_STOPPED;metro.click_left=metro.tap_left=0;metro.feedback=METRO_FEEDBACK_NONE;metro.version++;}
        else metro_restart();
    } else if(c==METRO_CMD_TAP)metro_tempo_tap();
}
/* A short decaying square click, quieter between groups. The final master
 * DC blocker/limiter/volume also apply to it. Fixed 32-bit integer arithmetic. */
static int32_t metro_sample(void)
{
    uint32_t now=metro.now++;
    if(!metro_enabled)return 0;
    if(metro.state==METRO_AUDIBLE || metro.state==METRO_RUNNING) {
        /* Browser-only grace allows audible-clock input messages to arrive.
         * Native input_grace stays zero; the matching window is unchanged. */
        uint32_t judged=now-metro.input_grace-(metro.input_delay>0?(uint32_t)metro.input_delay:0u);
        if(metro.judge<metro.targets && (int32_t)(judged-metro_targets[metro.judge].at)>(int32_t)metro.window) {
            if(metro_targets[metro.judge].hit==METRO_TARGET_PENDING) {
                metro_targets[metro.judge].hit=METRO_TARGET_MISSED;metro.misses++;
                metro.feedback=METRO_FEEDBACK_BAD;metro.feedback_at=now;
            }
            metro.judge++;metro.version++;
        }
        if(metro.cfg.mode!=METRO_PRACTICE && (int32_t)(judged-metro.end)>(int32_t)metro.window) {
            metro.state=METRO_RESULT;metro.click_left=0;metro.version++;
        } else if((int32_t)(now-metro.next)>=0 &&
                  (metro.cfg.mode==METRO_PRACTICE || (int32_t)(now-metro.end)<0)) {
            const metro_meter_t *m=&metro_meters[metro.cfg.meter];
            uint32_t beat=metro.pulse;
            metro.beat=(uint8_t)(beat%m->beats);metro.bar=(uint8_t)(beat/m->beats+1u);
            metro.state=metro.cfg.mode==METRO_GAME && !metro_hidden_bar(metro.bar,&metro.cfg)?METRO_AUDIBLE:METRO_RUNNING;
            int sound=1;
            if(metro.cfg.mode==METRO_GAME) sound=metro.bar<=METRO_GAME_BARS && !metro_target(metro.bar,metro.beat,&metro.cfg);
            if(sound) {
                uint32_t accent=m->accents&(1u<<metro.beat);
                metro.click_left=512;metro.click_phase=0;metro.click_pitch=(uint8_t)(accent?8u:13u);
                metro.click_gain=(uint8_t)(accent?METRO_CLICK_VOLUME*7u/5u:METRO_CLICK_VOLUME);
            }
            metro.pulse++;metro.next+=metro.period;metro.carry+=metro.rem;
            if(metro.carry>=metro.den) {metro.carry-=metro.den;metro.next++;}
            metro.version++;
        }
    }
    int32_t result=0;
    if(metro.click_left) {
        int32_t amp=(int32_t)(metro.click_left*metro.click_gain)/4;
        metro.click_left--;metro.click_phase++;
        if(metro.click_phase>=metro.click_pitch*2u)metro.click_phase=0;
        result=metro.click_phase<metro.click_pitch?amp:-amp;
    }
    if(metro.tap_left) {
        int32_t amp=(int32_t)(metro.tap_left*96u)/4;
        metro.tap_left--;metro.tap_phase++;
        if(metro.tap_phase>=38u)metro.tap_phase=0;
        result+=metro.tap_phase<19u?amp:-amp;
    }
    return result; /* peak <= 17,920 guide + 9,216 tap, before master limiter */
}
