/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Included by mix.c. Audio owns the clock, click and judgments. Main publishes
 * complete settings + a command with IRQs masked. No engine/transport clock,
 * allocation, storage, floating point or library math in the render path. */
/* Explicit stable wire IDs; adding/reordering options requires codec review. */
enum eMetroMode { METRO_PRACTICE=0, METRO_KEEP=1, METRO_MISSING=2, METRO_MODES_COUNT };
enum eMetroPattern { METRO_STRAIGHT=0, METRO_OFFBEAT=1, METRO_SWING=2, METRO_TRESILLO=3, METRO_PATTERNS_COUNT };
enum eMetroMeter { METRO_METER_4_4=0, METRO_METER_2_4=1, METRO_METER_3_4=2,
    METRO_METER_6_8=3, METRO_METER_9_8=4, METRO_METER_12_8=5,
    METRO_METER_5_8_23=6, METRO_METER_5_8_32=7, METRO_METER_7_8_223=8,
    METRO_METER_7_8_322=9, METRO_METER_7_8_232=10, METRO_METER_5_4=11,
    METRO_METER_7_4=12, METRO_METER_11_8=13, METRO_METER_13_8=14,
    METRO_METER_15_8=15, METRO_METER_3_2=16, METRO_METER_7_16=17,
    METRO_METER_1_4=18, METRO_METERS_COUNT };
enum eMetroCommand { METRO_CMD_NONE, METRO_CMD_RESTART, METRO_CMD_TOGGLE,
    METRO_CMD_EXIT, METRO_CMD_TAP, METRO_COMMANDS_COUNT };
enum eMetroState { METRO_STOPPED, METRO_COUNTIN, METRO_RUNNING, METRO_RESULT, METRO_STATES_COUNT };
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
    {"7/16","2+2+3",7,16,21}, {"1/4","1",1,4,1}
};
_Static_assert(METRO_METERS_COUNT==sizeof metro_meters/sizeof metro_meters[0],"meter ID/table mismatch");
#define METRO_USER_COUNT 4u
#define METRO_RECORD_BYTES 12u
#define METRO_PREFS_BYTES (METRO_RECORD_BYTES*(METRO_USER_COUNT+1u))
#define METRO_TARGETS_MAX 120u
#define METRO_GAME_BARS 8u
typedef struct { uint16_t bpm; uint8_t meter,pattern,sub,mode,volume,silent,tolerance; int8_t offset; } metro_settings_t;
static const metro_settings_t metro_default={120,METRO_METER_4_4,METRO_STRAIGHT,1,METRO_PRACTICE,60,1,60,0};
static metro_settings_t metro_settings={120,METRO_METER_4_4,METRO_STRAIGHT,1,METRO_PRACTICE,60,1,60,0};
static metro_settings_t metro_user[METRO_USER_COUNT]; /* zero BPM = empty */
static const struct { const char *name; metro_settings_t settings; } metro_factory[]={
    {"Quarter notes",{120,METRO_METER_4_4,METRO_STRAIGHT,1,METRO_PRACTICE,60,1,60}},
    {"Eighth notes",{100,METRO_METER_4_4,METRO_STRAIGHT,2,METRO_PRACTICE,60,1,60}},
    {"Triplets",{90,METRO_METER_4_4,METRO_STRAIGHT,3,METRO_PRACTICE,60,1,60}},
    {"Sixteenths",{80,METRO_METER_4_4,METRO_STRAIGHT,4,METRO_PRACTICE,60,1,60}},
    {"Swing",{110,METRO_METER_4_4,METRO_SWING,2,METRO_PRACTICE,60,1,60}},
    {"Offbeats",{100,METRO_METER_4_4,METRO_OFFBEAT,2,METRO_PRACTICE,60,1,60}},
    {"Tresillo",{100,METRO_METER_4_4,METRO_TRESILLO,2,METRO_PRACTICE,60,1,60}},
    {"Waltz",{100,METRO_METER_3_4,METRO_STRAIGHT,1,METRO_PRACTICE,60,1,60}},
    {"Compound six",{90,METRO_METER_6_8,METRO_STRAIGHT,1,METRO_PRACTICE,60,1,60}},
    {"Five: 2+3",{100,METRO_METER_5_8_23,METRO_STRAIGHT,1,METRO_PRACTICE,60,1,60}},
    {"Seven: 2+2+3",{100,METRO_METER_7_8_223,METRO_STRAIGHT,1,METRO_PRACTICE,60,1,60}},
    {"Eleven",{100,METRO_METER_11_8,METRO_STRAIGHT,1,METRO_PRACTICE,60,1,60}},
    {"Keep: one silent bar",{100,METRO_METER_4_4,METRO_STRAIGHT,1,METRO_KEEP,60,1,60}},
    {"Keep: three silent bars",{100,METRO_METER_4_4,METRO_STRAIGHT,1,METRO_KEEP,60,3,40}},
    {"Fill missing beats",{100,METRO_METER_4_4,METRO_STRAIGHT,1,METRO_MISSING,60,1,60}},
    {"Fill odd seven",{90,METRO_METER_7_8_223,METRO_STRAIGHT,1,METRO_MISSING,60,1,60}}
};
#define METRO_FACTORY_COUNT (sizeof metro_factory/sizeof metro_factory[0])
static volatile uint8_t metro_enabled,metro_request;
static struct {
    metro_settings_t cfg;
    uint32_t now,start,next,period,rem,den,carry,pulse,end,tap_last,tap_sum,tap_intervals[4];
    uint32_t window,version,abs_ms,square_ms;
    int32_t bias_ms,last_ms,input_delay;
    uint16_t extra,hits,misses,click_left,click_phase;
    uint8_t state,beat,bar,judge,targets,tap_count,tap_valid,tap_index,click_pitch,click_gain;
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
static void metro_pack(uint8_t *p,const metro_settings_t *s)
{
    p[0]='M';p[1]=1;p[2]=(uint8_t)s->bpm;p[3]=(uint8_t)(s->bpm>>8);
    p[4]=s->meter;p[5]=(uint8_t)(s->pattern|(s->mode<<4));p[6]=s->sub;p[7]=(uint8_t)(s->offset+100);
    p[8]=s->volume;p[9]=s->silent;p[10]=s->tolerance;p[11]=metro_crc(p);
}
static int metro_unpack(metro_settings_t *s,const uint8_t *p)
{
    uint32_t bpm=p[2]|((uint32_t)p[3]<<8);
    if(p[0]!='M' || p[1]!=1u || p[11]!=metro_crc(p) || bpm<30u || bpm>300u ||
       p[4]>=METRO_METERS_COUNT || (p[5]&15u)>=METRO_PATTERNS_COUNT || (p[5]>>4)>=METRO_MODES_COUNT || p[6]<1u || p[6]>4u ||
       p[7]>200u || p[8]>100u || p[9]<1u || p[9]>4u || p[10]<10u || p[10]>150u)return 0;
    s->bpm=(uint16_t)bpm;s->meter=p[4];s->pattern=p[5]&15u;s->sub=p[6];s->mode=p[5]>>4;s->offset=(int8_t)((int32_t)p[7]-100);
    s->volume=p[8];s->silent=p[9];s->tolerance=p[10];return 1;
}
static uint32_t metro_sub(const metro_settings_t *s)
{ return s->mode!=METRO_PRACTICE?1u:s->pattern==METRO_SWING?3u:s->pattern==METRO_STRAIGHT?s->sub:2u; }
static int metro_target(uint32_t bar,uint32_t beat,const metro_settings_t *s)
{
    if(!bar || bar>METRO_GAME_BARS)return 0;
    if(s->mode==METRO_KEEP)return (bar-1u)%(s->silent+1u)!=0u;
    return s->mode==METRO_MISSING && ((beat&1u) || metro_meters[s->meter].beats==1u);
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
    metro.input_delay=(int32_t)metro.cfg.offset*(int32_t)FS/1000;
    const metro_meter_t *m=&metro_meters[metro.cfg.meter];
    metro.start=metro.now+FS/5u;metro.next=metro.start;metro.pulse=metro.carry=0;
    metro.den=(uint32_t)metro.cfg.bpm*m->den*metro_sub(&metro.cfg);
    metro.period=FS*240u/metro.den;metro.rem=FS*240u%metro.den;
    /* Never match a tap to more than half a beat, even at fast 16th meters. */
    metro.window=FS*(uint32_t)metro.cfg.tolerance/1000u;
    uint32_t half=(FS*240u/((uint32_t)metro.cfg.bpm*m->den)-1u)/2u;
    if(metro.window>half)metro.window=half;
    metro.targets=metro.judge=0;metro.hits=metro.extra=metro.misses=0;
    metro.abs_ms=metro.square_ms=0;metro.bias_ms=metro.last_ms=0;
    uint32_t den=(uint32_t)metro.cfg.bpm*m->den;
    for(uint32_t b=1;b<=METRO_GAME_BARS;b++)for(uint32_t k=0;k<m->beats;k++)if(metro_target(b,k,&metro.cfg)) {
        uint32_t i=metro.targets++;
        metro_targets[i].at=metro.start+((b*m->beats+k)*(FS*240u))/den;metro_targets[i].hit=0;
    }
    metro.end=metro.start+((METRO_GAME_BARS+1u)*m->beats*(FS*240u))/den;
    metro.state=METRO_COUNTIN;metro.beat=metro.bar=0;metro.click_left=0;metro.version++;
}
static void metro_tap(void)
{
    if(metro.cfg.mode==METRO_PRACTICE ||
       (metro.state!=METRO_RUNNING && metro.state!=METRO_COUNTIN))return;
    /* Allow an early first answer at the count-in boundary (notably 1/4).
     * Taps earlier in the count-in have no effect on the score. */
    uint32_t tapped=metro.now-metro.input_delay;
    if(metro.state==METRO_COUNTIN && (!metro.targets ||
       (int32_t)(tapped-metro_targets[0].at)<-(int32_t)metro.window))return;
    uint32_t best=METRO_TARGETS_MAX,distance=metro.window+1u;
    for(uint32_t k=0;k<metro.targets;k++)if(!metro_targets[k].hit) {
        int32_t d=(int32_t)(tapped-metro_targets[k].at);
        uint32_t a=(uint32_t)(d<0?-d:d);
        if(a<distance) {best=k;distance=a;}
    }
    if(best==METRO_TARGETS_MAX) {if(metro.extra<999u)metro.extra++;metro.version++;return;}
    metro_targets[best].hit=1;metro.hits++;
    int32_t d=(int32_t)(tapped-metro_targets[best].at);
    int32_t ms=d*1000/(int32_t)FS;
    metro.last_ms=ms;metro.bias_ms+=ms;metro.abs_ms+=(uint32_t)(ms<0?-ms:ms);
    metro.square_ms+=(uint32_t)(ms*ms);metro.version++;
}
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
    if(c==METRO_CMD_EXIT) {metro.state=METRO_STOPPED;metro.click_left=0;metro_enabled=0;metro.tap_valid=0;}
    else if(c==METRO_CMD_RESTART)metro_restart();
    else if(c==METRO_CMD_TOGGLE) {
        if(metro.state==METRO_COUNTIN || metro.state==METRO_RUNNING) {metro.state=METRO_STOPPED;metro.click_left=0;metro.version++;}
        else metro_restart();
    } else if(c==METRO_CMD_TAP)metro_tempo_tap();
}
/* A short decaying square click, quieter between groups. The final master
 * DC blocker/limiter/volume also apply to it. Fixed 32-bit integer arithmetic. */
static int32_t metro_sample(void)
{
    uint32_t now=metro.now++;
    if(!metro_enabled)return 0;
    if(metro.state==METRO_COUNTIN || metro.state==METRO_RUNNING) {
        uint32_t judged=now-(metro.input_delay>0?(uint32_t)metro.input_delay:0u);
        if(metro.judge<metro.targets && (int32_t)(judged-metro_targets[metro.judge].at)>(int32_t)metro.window) {
            if(!metro_targets[metro.judge].hit)metro.misses++;
            metro.judge++;metro.version++;
        }
        if(metro.cfg.mode!=METRO_PRACTICE && (int32_t)(judged-metro.end)>(int32_t)metro.window) {
            metro.state=METRO_RESULT;metro.click_left=0;metro.version++;
        } else if((int32_t)(now-metro.next)>=0) {
            const metro_meter_t *m=&metro_meters[metro.cfg.meter];
            uint32_t sub=metro_sub(&metro.cfg),beat=metro.pulse/sub,part=metro.pulse%sub;
            metro.beat=(uint8_t)(beat%m->beats);metro.bar=(uint8_t)(beat/m->beats);
            metro.state=metro.bar?METRO_RUNNING:METRO_COUNTIN;
            int sound=1;
            if(metro.cfg.mode!=METRO_PRACTICE) sound=metro.bar<=METRO_GAME_BARS && !metro_target(metro.bar,metro.beat,&metro.cfg);
            else if(metro.cfg.pattern==METRO_OFFBEAT)sound=part==1u;
            else if(metro.cfg.pattern==METRO_SWING)sound=part!=1u;
            else if(metro.cfg.pattern==METRO_TRESILLO) {uint32_t p=metro.pulse%8u;sound=p==0u || p==3u || p==6u;}
            if(sound) {
                uint32_t accent=!part && (m->accents&(1u<<metro.beat));
                metro.click_left=256;metro.click_phase=0;metro.click_pitch=(uint8_t)(accent?8u:part?20u:13u);
                metro.click_gain=(uint8_t)(metro.cfg.volume*(accent?4u:part?1u:2u)/4u);
            }
            metro.pulse++;metro.next+=metro.period;metro.carry+=metro.rem;
            if(metro.carry>=metro.den) {metro.carry-=metro.den;metro.next++;}
            metro.version++;
        }
    }
    if(!metro.click_left)return 0;
    int32_t amp=(int32_t)(metro.click_left*metro.click_gain)/4;
    metro.click_left--;metro.click_phase++;
    if(metro.click_phase>=metro.click_pitch*2u)metro.click_phase=0;
    return metro.click_phase<metro.click_pitch?amp:-amp;
}
