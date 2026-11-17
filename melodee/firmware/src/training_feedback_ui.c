/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Shared foreground drawing for pitch and rhythm feedback. */
static uint32_t metro_sqrt(uint32_t x)
{
    uint32_t r=0,bit=1u<<30;
    for(uint32_t i=0;i<16u;i++,bit>>=2) {
        if(x>=r+bit) {x-=r+bit;r=(r>>1)+bit;}else r>>=1;
    }
    return r;
}
/* One bounded integer sqrt per visible row, fractional edge coverage,
 * no supersampling or full-screen buffer. Uses the existing 48-row strips. */
static void training_feedback_circle(uint32_t band,uint32_t radius,uint16_t ink)
{
    for(int32_t y=0;y<48;y++) {
        int32_t dy=(int32_t)band*48+y-132;
        int32_t q=(int32_t)(radius*radius)-dy*dy;
        if(q<0)continue;
        uint32_t span=metro_sqrt((uint32_t)q<<8),fraction=span&15u;
        int32_t half=(int32_t)(span>>4);
        cv_rect(120-half,y,half*2+1,1,ink);
        uint16_t edge=mix565(T_BG,ink,fraction*2u);
        cv_pset(119-half,y,edge);cv_pset(121+half,y,edge);
    }
}
