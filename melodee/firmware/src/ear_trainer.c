/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Sequence game. Included by seq.c: all mutable game/input state belongs to
 * the audio ISR. UI publishes one command under the existing IRQ protocol.
 * Generated notes call the existing voice manager, never the input collector.
 * No allocation, storage, logging or waits. */
enum eEarState { EAR_OFF, EAR_PREPARE, EAR_FIRST, EAR_GAP, EAR_SECOND, EAR_SETTLE,
                 EAR_ANSWER, EAR_WRONG, EAR_CORRECT, EAR_STATES_COUNT };
enum eEarCommand { EAR_CMD_NONE, EAR_CMD_NEXT, EAR_CMD_RETRY, EAR_CMD_EXIT, EAR_CMD_REPEAT, EAR_COMMANDS_COUNT };
enum eEarDegree { EAR_DEGREE_SECOND, EAR_DEGREE_THIRD, EAR_DEGREE_FOURTH,
    EAR_DEGREE_FIFTH, EAR_DEGREE_SIXTH, EAR_DEGREE_SEVENTH, EAR_DEGREE_OCTAVE, EAR_DEGREES_COUNT };
static const uint8_t ear_intervals[EAR_DEGREES_COUNT]={2,4,5,7,9,11,12};
enum eEarScale { EAR_MAJOR, EAR_MINOR, EAR_SCALES_COUNT };
enum eEarRange { EAR_RANGE_OCTAVE, EAR_RANGE_FULL, EAR_RANGES_COUNT };
#define EAR_PRESETS_COUNT (FM6_NFAC+P5_FACTORY_N)
_Static_assert(FM6_NFAC==24u && P5_FACTORY_N==200u,"migrate saved sound IDs before changing factory bank sizes");
_Static_assert(EAR_PRESETS_COUNT<=256u,"training sound IDs fit the checked preference byte");
/* Stable sound IDs: existing FM6 0..23, Prophet factory programs 24..223.
 * The hardware adapter maps these to its engine/preset namespace. */
static uint32_t ear_preset_engine(uint32_t preset) { return preset<FM6_NFAC?ENGI_FM6:ENGI_PROPHET; }
static uint32_t ear_preset_index(uint32_t preset) { return preset<FM6_NFAC?preset:preset-FM6_NFAC+1u; }
static const uint8_t ear_minor_intervals[EAR_DEGREES_COUNT]={2,3,5,7,8,10,12};
/* Main publishes settings and NEXT atomically under the existing IRQ mask. */
typedef struct { uint16_t gap; uint8_t scale, count, guess, range, preset; } ear_settings_t;
static ear_settings_t ear_settings={250,EAR_MAJOR,2,1,EAR_RANGE_OCTAVE,0};
#define MELODEE_EAR_TRAINER 1
#define EAR_SETTINGS_BYTES 12u
/* Main-only codec: explicit wire IDs/little endian, never compiler enum layout. */
static uint8_t ear_settings_crc(const uint8_t *p)
{
    uint32_t crc=0;
    for(uint32_t k=0;k<EAR_SETTINGS_BYTES-1u;k++) {
        crc^=p[k];
        for(uint32_t bit=0;bit<8u;bit++) crc=((crc<<1)^((crc&128u)?7u:0u))&255u;
    }
    return (uint8_t)crc;
}
static void ear_settings_pack(uint8_t *p)
{
    p[0]='E';p[1]='A';p[2]='R';p[3]=ear_settings.preset<FM6_NFAC?1u:2u;
    p[4]=ear_settings.scale==EAR_MINOR?1u:0u;
    p[5]=ear_settings.range==EAR_RANGE_FULL?1u:0u;
    p[6]=ear_settings.count;p[7]=ear_settings.guess;p[8]=ear_settings.preset;
    p[9]=(uint8_t)ear_settings.gap;p[10]=(uint8_t)(ear_settings.gap>>8);
    p[11]=ear_settings_crc(p);
}
static int ear_settings_unpack(ear_settings_t *out,const uint8_t *p,uint32_t n)
{
    if(n!=EAR_SETTINGS_BYTES || p[0]!='E' || p[1]!='A' || p[2]!='R' || (p[3]!=1u && p[3]!=2u) ||
       p[11]!=ear_settings_crc(p) || p[4]>1u || p[5]>1u ||
       p[6]<1u || p[6]>4u || p[7]<1u || p[7]>p[6] ||
       p[8]>=(p[3]==1u?FM6_NFAC:EAR_PRESETS_COUNT)) return 0;
    uint32_t gap=p[9]|((uint32_t)p[10]<<8);
    if(gap>1000u || gap%25u) return 0;
    out->scale=p[4]?EAR_MINOR:EAR_MAJOR;out->range=p[5]?EAR_RANGE_FULL:EAR_RANGE_OCTAVE;
    out->count=p[6];out->guess=p[7];out->preset=p[8];out->gap=(uint16_t)gap;
    return 1;
}
static uint32_t ear_settings_ms; /* main: debounce settings writes */
/* Forty independent rating ladders: scale x range x (hidden of total).
 * Runtime data stays in the pool. Main alone serializes/checkpoints it. */
#define EAR_MODES_COUNT 40u
#define EAR_RATING_MAX 2000u
#define EAR_RATINGS_BYTES 168u
_Static_assert(EAR_MODES_COUNT==EAR_SCALES_COUNT*EAR_RANGES_COUNT*10u,"migrate rating mode IDs before adding scales/ranges");
static uint16_t ear_ratings[EAR_MODES_COUNT][2] __attribute__((section(".pool")));
static volatile uint8_t ear_ratings_dirty;
static uint32_t ear_ratings_saved_ms;
static uint32_t ear_mode(uint32_t scale,uint32_t range,uint32_t count,uint32_t guess)
{ return (scale*EAR_RANGES_COUNT+range)*10u+count*(count-1u)/2u+guess-1u; }
static uint8_t ear_rating_crc(const uint8_t *p)
{
    uint32_t crc=0;
    for(uint32_t k=0;k<EAR_RATINGS_BYTES-1u;k++) {
        crc^=p[k];
        for(uint32_t b=0;b<8u;b++)crc=((crc<<1)^((crc&128u)?7u:0u))&255u;
    }
    return (uint8_t)crc;
}
static void ear_ratings_pack(uint8_t *p)
{
    p[0]='E';p[1]='R';p[2]='S';p[3]=1;p[4]=EAR_MODES_COUNT;p[5]=p[166]=0;
    for(uint32_t k=0;k<EAR_MODES_COUNT;k++)for(uint32_t j=0;j<2u;j++) {
        uint32_t at=6u+4u*k+2u*j,v=ear_ratings[k][j];p[at]=(uint8_t)v;p[at+1]=(uint8_t)(v>>8);
    }
    p[167]=ear_rating_crc(p);
}
static int ear_ratings_unpack(const uint8_t *p,uint32_t n)
{
    if(n!=EAR_RATINGS_BYTES || p[0]!='E' || p[1]!='R' || p[2]!='S' || p[3]!=1u ||
       p[4]!=EAR_MODES_COUNT || p[5] || p[166] || p[167]!=ear_rating_crc(p))return 0;
    for(uint32_t k=0;k<EAR_MODES_COUNT;k++) {
        uint32_t at=6u+4u*k,current=p[at]|((uint32_t)p[at+1]<<8),best=p[at+2]|((uint32_t)p[at+3]<<8);
        if(current>best || best>EAR_RATING_MAX)return 0;
    }
    for(uint32_t k=0;k<EAR_MODES_COUNT;k++)for(uint32_t j=0;j<2u;j++) {
        uint32_t at=6u+4u*k+2u*j;ear_ratings[k][j]=(uint16_t)(p[at]|((uint32_t)p[at+1]<<8));
    }
    ear_ratings_dirty=0;return 1;
}
/* Main brackets preset publication; audio/input cannot start notes mid-load. */
static volatile uint8_t ear_enabled, ear_request, ear_loading, ear_saving;
static struct {
    uint32_t left, seed, serial, attempts;
    uint32_t score, solved;
    uint16_t replays, streak;
    int16_t last_award;
    int32_t playing, player;
    uint8_t state, root, target, armed, degree, need[EAR_DEGREES_COUNT];
    uint8_t notes[4], degrees[4], count, guess, index, entered, scale, sounding, range;
    uint16_t gap;
    uint8_t matched, mode, rated;
} ear = { .playing = -1, .player = -1 };
/* Bounded integer Elo-like update. At the mode's reference rating, a clean
 * answer wins 10 and an error loses 10. Expected success rises with rating,
 * so the score converges instead of rewarding endless easy repetitions. */
static void ear_rate(int success)
{
    uint32_t mode=ear.mode;
    int32_t rating=ear_ratings[mode][0];
    int32_t reference=100+(int32_t)ear.guess*120+(int32_t)(ear.count-1u)*60+(int32_t)ear.range*150;
    int32_t expected=50+(rating-reference)/20;
    if(expected<5)expected=5;if(expected>95)expected=95;
    int32_t delta=success?(100-expected+4)/5:-(expected+4)/5;
    int32_t next=rating+delta;if(next<0)next=0;if(next>EAR_RATING_MAX)next=EAR_RATING_MAX;
    ear.last_award=(int16_t)(next-rating);ear.score=(uint32_t)next;
    ear_ratings[mode][0]=(uint16_t)next;
    if(next>ear_ratings[mode][1])ear_ratings[mode][1]=(uint16_t)next;
    if(next!=rating)ear_ratings_dirty=1;
}
static uint32_t ear_random(void)
{
    if(!ear.seed) ear.seed=0xEA12345u;
    ear.seed^=ear.seed<<13;ear.seed^=ear.seed>>17;ear.seed^=ear.seed<<5;
    return ear.seed;
}
/* Scale-relative pitch class, including tonic octaves in the octave bucket. */
static uint32_t ear_note_degree(uint32_t note)
{
    uint32_t pitch=(note+12u-ear.root%12u)%12u;
    const uint8_t *intervals=ear.scale==EAR_MINOR?ear_minor_intervals:ear_intervals;
    for(uint32_t d=0;d<EAR_DEGREES_COUNT;d++)
        if(pitch==intervals[d]%12u) return d;
    return EAR_DEGREES_COUNT;
}
/* Two fixed 27-key scans: weighted sampling without replacement, no retries.
 * All scale notes on F3..G5 are eligible, above and below the reference. */
static uint32_t ear_pick_note(uint32_t excluded)
{
    uint32_t total=0;
    for(uint32_t key=0;key<27u;key++) {
        if(ear.range==EAR_RANGE_OCTAVE && (53u+key<ear.root || 53u+key>ear.root+12u)) continue;
        uint32_t d=ear_note_degree(53u+key);
        if(!(excluded&(1u<<key)) && d<EAR_DEGREES_COUNT) total+=4u+2u*ear.need[d];
    }
    uint32_t ticket=ear_random()%total; /* >=8 scale keys, at most four exclusions. */
    for(uint32_t key=0;key<27u;key++) {
        if(ear.range==EAR_RANGE_OCTAVE && (53u+key<ear.root || 53u+key>ear.root+12u)) continue;
        uint32_t d=ear_note_degree(53u+key);
        if((excluded&(1u<<key)) || d>=EAR_DEGREES_COUNT) continue;
        uint32_t weight=4u+2u*ear.need[d];
        if(ticket<weight) return 53u+key;
        ticket-=weight;
    }
    return ear.root; /* Unreachable for valid game settings. */
}
static uint32_t ear_keys;
static uint16_t ear_down[4];
/* Port/channel/pitch ownership, including keys held across playback.
 * Keep this larger buffer in the existing pool, not scarce .data/.bss. */
static uint32_t ear_midi_keys[4][16][4] __attribute__((section(".pool")));
static void ear_generated(int32_t note)
{
    if(note<0) {
        for(uint32_t k=0;k<4u;k++) if(ear.sounding&(1u<<k))
            trk_note_off(&trk[0],ear.notes[k]);
        ear.sounding=0;ear.playing=-1;return;
    }
    /* Repeated pitches retrigger the same tone rather than stacking duplicates. */
    for(uint32_t k=0;k<4u;k++) if((ear.sounding&(1u<<k)) && ear.notes[k]==note) {
        trk_note_off(&trk[0],(uint32_t)note);ear.sounding&=(uint8_t)~(1u<<k);
    }
    ear.playing=note;ear.sounding|=(uint8_t)(1u<<ear.index);
    trk_note_on(&trk[0],(uint32_t)note,100u);
}
static int ear_held(void)
{
    return ear_keys || ear_down[0] || ear_down[1] || ear_down[2] || ear_down[3];
}
static void ear_answer(uint32_t note)
{
    if (ear.state != EAR_ANSWER || !ear.armed) return;
    if(!ear.entered && ear.attempts<255u) ear.attempts++;
    uint32_t found=4u;
    for(uint32_t k=ear.count-ear.guess;k<ear.count;k++)
        if(note==ear.notes[k] && !(ear.matched&(1u<<k)))found=k;
    if(found<4u) {
        ear.matched|=(uint8_t)(1u<<found);ear.entered++;
        if(ear.entered<ear.guess)return; /* Any order, including simultaneous presses. */
    }
    ear.state = found<4u ? EAR_CORRECT : EAR_WRONG;
    ear.armed=0; /* One submitted note per attempt, including audible feedback. */
    ear.left=FS; /* One second of feedback before next/repeat. */
    if(ear.state==EAR_CORRECT) {
        ear.last_award=0;
        if(ear.attempts==1u && !ear.replays && !ear.rated)ear_rate(1);
        ear.rated=1;
        if(ear.solved<999999u) ear.solved++;
        if(ear.attempts==1u && !ear.replays) {
            if(ear.streak<999u) ear.streak++;
            for(uint32_t k=ear.count-ear.guess;k<ear.count;k++)
                if(ear.need[ear.degrees[k]]) ear.need[ear.degrees[k]]--;
        } else ear.streak=0;
    } else {
        ear_rate(0);ear.rated=1;
        ear.streak=0;
        /* Unmatched tones are the uncertainty; arrival order has no meaning. */
        for(uint32_t k=ear.count-ear.guess;k<ear.count;k++)if(!(ear.matched&(1u<<k))) {
            ear.degree=ear.degrees[k];if(ear.need[ear.degree]<6u)ear.need[ear.degree]++;
        }
    }
}
static void ear_player_stop(void)
{
    if(ear.player>=0) trk_note_off(&trk[0],(uint32_t)ear.player);
    ear.player=-1;
}
static void ear_player_press(uint32_t note)
{
    if(ear_loading || ear_saving || ear.state!=EAR_ANSWER || !ear.armed) return;
    ear_player_stop();
    ear.player=(int32_t)note;
    trk_note_on(&trk[0],note,100u);
    ear_answer(note);
}
static void ear_player_release(uint32_t note)
{
    if(ear.player!=(int32_t)note) return;
    if(note>=53u && note<=79u && (ear_keys&(1u<<(note-53u)))) return;
    for(uint32_t s=0;s<4u;s++) for(uint32_t ch=0;ch<16u;ch++)
        if(ear_midi_keys[s][ch][note>>5]&(1u<<(note&31u))) return;
    ear_player_stop();
}
/* Local keys have a fixed chromatic F3..G5 layout in this first exercise. */
static void ear_keyboard(uint32_t cur)
{
    uint32_t changed=cur^ear_keys;
    ear_keys=cur;
    for(uint32_t k=0;k<27u;k++) if(changed&(1u<<k)) {
        if(cur&(1u<<k)) ear_player_press(53u+k);
        else ear_player_release(53u+k);
    }
}
static void ear_source_lost(uint32_t source);
/* Observe raw MIDI while inactive too, so a pre-existing hold cannot answer.
 * Generated playback never enters this raw input path. */
static int ear_midi(uint32_t st,uint32_t ch,uint32_t note,uint32_t vel,uint32_t source)
{
    if (source>3u || ch>=16u || note>=128u) return ear_enabled;
    uint32_t *bits=&ear_midi_keys[source][ch][note>>5], bit=1u<<(note&31u), was=*bits&bit;
    if(st==0x90u && vel) {
        *bits|=bit;
        if(!was) ear_down[source]++;
        if(ear_enabled && !was) ear_player_press(note);
    } else if(st==0x80u || (st==0x90u && !vel)) {
        *bits&=~bit;
        if(was) ear_down[source]--;
        if(ear_enabled) ear_player_release(note);
    } else if(st==0xB0u && (note==120u || note==123u)) ear_source_lost(source);
    return ear_enabled;
}
static void ear_source_lost(uint32_t source)
{
    if(source<4u) for(uint32_t ch=0;ch<16u;ch++) for(uint32_t k=0;k<4u;k++) ear_midi_keys[source][ch][k]=0;
    if(source<4u) ear_down[source]=0;
    if(ear_enabled && ear.player>=0) ear_player_release((uint32_t)ear.player);
}
static void ear_audio_tick(uint32_t n)
{
    if(ear_saving) return; /* Preserve the quiet answer pause during main's flash commit. */
    uint32_t cmd=ear_request;
    ear_request=EAR_CMD_NONE;
    if(cmd==EAR_CMD_EXIT) {
        if(ear.serial && !ear.rated && ear.state!=EAR_OFF)ear_rate(0);
        ear_generated(-1); ear_player_stop(); ear.state=EAR_OFF; ear_enabled=0; return;
    }
    if(!ear_enabled) return;
    if(ear_loading) {
        ear_generated(-1);ear_player_stop();ear.armed=0;
        ear.state=EAR_PREPARE;ear.left=FS/5u;return;
    }
    /* A solved question cannot be replayed for another points award. */
    if(cmd==EAR_CMD_RETRY && ear.state==EAR_CORRECT) cmd=EAR_CMD_NEXT;
    if(cmd==EAR_CMD_NONE && (ear.state==EAR_CORRECT || ear.state==EAR_WRONG)) {
        if(n<ear.left) ear.left-=n;
        else cmd=ear.state==EAR_CORRECT?EAR_CMD_NEXT:EAR_CMD_REPEAT;
    }
    if(cmd==EAR_CMD_NEXT || cmd==EAR_CMD_RETRY || cmd==EAR_CMD_REPEAT) {
        ear_generated(-1);
        ear_player_stop();
        if(cmd==EAR_CMD_NEXT) {
            if(ear.serial && !ear.rated && ear.state!=EAR_OFF)ear_rate(0);
            if(ear.serial && ear.state!=EAR_CORRECT) ear.streak=0; /* Skip/exit breaks a clean run. */
            uint32_t previous=ear.notes[0];
            ear.scale=ear_settings.scale;ear.count=ear_settings.count;
            ear.guess=ear_settings.guess;ear.gap=ear_settings.gap;ear.range=ear_settings.range;
            ear.root=ear.scale==EAR_MINOR?57u:60u; /* A3 / C4: playable octaves. */
            ear.mode=(uint8_t)ear_mode(ear.scale,ear.range,ear.count,ear.guess);
            ear.score=ear_ratings[ear.mode][0];ear.rated=0;
            uint32_t used=0;
            for(uint32_t k=0;k<ear.count;k++) {
                if(!k && ear.guess<ear.count) ear.notes[k]=ear.root;
                else {
                    uint32_t excluded=used;
                    if(!k && ear.serial && previous>=53u && previous<=79u)
                        excluded|=1u<<(previous-53u);
                    ear.notes[k]=(uint8_t)ear_pick_note(excluded);
                }
                used|=1u<<(ear.notes[k]-53u);
                ear.degrees[k]=(uint8_t)ear_note_degree(ear.notes[k]);
            }
            ear.degree=ear.degrees[ear.count-1u];ear.target=ear.notes[ear.count-1u];
            ear.serial++;ear.attempts=0;ear.replays=0;ear.last_award=0;
        } else if(cmd==EAR_CMD_RETRY && ear.replays<999u) {
            if(!ear.rated) {ear_rate(0);ear.rated=1;}
            ear.replays++;
        }
        ear.state=EAR_PREPARE; ear.left=FS/5u; ear.armed=0;ear.index=0;ear.entered=0;ear.matched=0;
    }
    if(ear.state==EAR_PREPARE) {
        /* Loading a preset/engine may fade asynchronously. Never spend the
         * first note's gate while its engine is still switching. */
        if(trk[0].engine!=ear_preset_engine(ear_settings.preset) || trk[0].xf_on) return;
        if(n<ear.left) { ear.left-=n; return; }
        ear.state=EAR_FIRST;ear_generated(ear.notes[0]);
        if(!ear.gap) {
            for(uint32_t k=1;k<ear.count;k++) { ear.index=(uint8_t)k;ear_generated(ear.notes[k]); }
        }
        ear.left=ear.index+1u<ear.count?(FS*ear.gap)/1000u:FS;
        return;
    }
    if(ear.state==EAR_ANSWER) {
        if(!ear_held()) ear.armed=1;
        return;
    }
    if(ear.state<EAR_FIRST || ear.state>EAR_SETTLE) return;
    /* Onset spacing, with every earlier note held until one second after last. */
    if(n<ear.left) { ear.left-=n; return; }
    switch(ear.state) {
    case EAR_FIRST:
    case EAR_SECOND:
        if(ear.index+1u>=ear.count) {
            ear_generated(-1);ear.state=EAR_SETTLE;ear.left=FS/10u;
        } else {
            ear.index++;ear_generated(ear.notes[ear.index]);ear.state=EAR_SECOND;
            ear.left=ear.index+1u<ear.count?(FS*ear.gap)/1000u:FS;
        }
        break;
    case EAR_GAP: ear_generated(ear.notes[ear.index]); ear.state=EAR_SECOND; ear.left=FS/2u; break;
    case EAR_SETTLE: ear.state=EAR_ANSWER; ear.armed=!ear_held(); break;
    default: break;
    }
}
