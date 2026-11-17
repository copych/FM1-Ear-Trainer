/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Standalone hat lanes from the existing DRUM kits. Main prepares the six
 * trigger snapshots before opening Metronome; audio only copies a snapshot
 * and renders fixed CTL-sized blocks into private SRAM. No track, shared kit
 * scratch, resource allocation or floating-point setup is touched by a hit. */
typedef union { dr8_t analog; dlk_ch_t lane; } metro_hat_voice_t;
static struct {
    metro_hat_voice_t seed[METRO_SOUNDS_COUNT-1u][2],voice;
    dr_scratch_t scratch;
    int16_t params[P_COUNT];
    int32_t samples[CTL];
    uint8_t kit,active,pos;
} metro_hats __attribute__((section(".pool")));
static uint8_t metro_hats_ready; /* zeroed BSS; pool contents are undefined at boot */
static void metro_hat_clear(void)
{metro_hats.active=0;metro_hats.pos=CTL;metro_hats.kit=METRO_SOUND_CLICK;}
/* Main only, before metro_enabled is published. Independent of song/patch. */
static void metro_hat_prepare(void)
{
    if(metro_hats_ready)return;
    memset(&metro_hats,0,sizeof metro_hats);
    int16_t *p=metro_hats.params;
    p[P_E0]=DK_808;p[P_E1]=p[P_E2]=p[P_E3]=p[P_E4]=64;p[P_E5]=100;
    const int16_t q[8]={0,64,64,64,64,100,0,0};
    for(uint32_t kit=METRO_SOUND_808;kit<METRO_SOUNDS_COUNT;kit++)for(uint32_t hat=0;hat<2u;hat++) {
        metro_hat_voice_t *v=&metro_hats.seed[kit-1u][hat];
        if(kit==METRO_SOUND_808)dr8_hit(&v->analog,hat?DR_CH:DR_OH,100,p);
        else {
            dlk_ch_t *c=&v->lane;
            c->nst=0x2F6E2B1;c->gain=dr8_level(100,100);c->v=100*DLK_HIT_V;
            c->sound=(uint8_t)(hat?D9_CH:D9_OH);c->on=1;
            if(kit==METRO_SOUND_CR78)(void)cr_hit(c,c->sound,0,q,0,0);
            else (void)fmk_hit(c,c->sound,0,q,0);
        }
    }
    metro_hat_clear();metro_hats_ready=1;
}
static void metro_hat_trigger(uint32_t sound,uint32_t strong)
{
    if(!metro_hats_ready || sound<=METRO_SOUND_CLICK || sound>=METRO_SOUNDS_COUNT)return;
    /* One hat at a time: a CH replaces/chokes the OH at its exact pulse. */
    metro_hats.voice=metro_hats.seed[sound-1u][strong?0u:1u];
    metro_hats.kit=(uint8_t)sound;metro_hats.active=1;metro_hats.pos=CTL;
}
static int32_t metro_hat_sample(void)
{
    if(metro_hats.pos>=CTL) {
        if(!metro_hats.active)return 0;
        int32_t *y=metro_hats.samples;
        for(uint32_t i=0;i<CTL;i++)y[i]=0;
        if(metro_hats.kit==METRO_SOUND_808) {
            dr8_run_into(&metro_hats.voice.analog,metro_hats.params,y,CTL,&metro_hats.scratch);
            metro_hats.active=metro_hats.voice.analog.on;
        } else {
            dlk_ch_t *c=&metro_hats.voice.lane;
            metro_hats.active=(uint8_t)(metro_hats.kit==METRO_SOUND_CR78?cr_run(c,y,CTL):fmk_run(c,y,CTL));
            c->t+=CTL;
            for(uint32_t i=0;i<CTL;i++)y[i]=(int32_t)(((int64_t)y[i]*c->gain)>>15);
        }
        for(uint32_t i=0;i<CTL;i++)y[i]=soft_knee(y[i]+(y[i]>>2),24000);
        metro_hats.pos=0;
    }
    return metro_hats.samples[metro_hats.pos++];
}
