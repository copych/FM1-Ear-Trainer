/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
#include <assert.h>
static void check_erase_guard(void);
#define PERSIST_ERASE_HOOK check_erase_guard
#define PERSISTENCE_TEST_NO_MAIN
#include "persistence_test.c"
static uint32_t guarded_erases;
static void check_erase_guard(void)
{
    if(ear_enabled) {
        assert(ear_saving && ear.state==EAR_ANSWER && !ear_held() && !ear_request);
        assert(!ear.sounding && ear.player<0);
        for(uint32_t k=0;k<NPART;k++)for(uint32_t v=0;v<NVOICE;v++) assert(!trk[k].v[v].active);
        ear.armed=1;
        uint32_t age=vage, attempts=ear.attempts;
        ear_player_press(60);
        assert(vage==age && ear.attempts==attempts && ear.player<0);
        guarded_erases++;
    }
}
static void defaults(void)
{
    ear_settings_t next={250,EAR_MAJOR,2,1,EAR_RANGE_OCTAVE,0};ear_settings=next;
}
static void reboot(void)
{
    ear_enabled=ear_request=ear_loading=ear_saving=0;defaults();persist_boot();
}
int main(void)
{
    reset();defaults();persist_boot();
    /* Every combination round-trips through the real PER5 codec, preserving neighbors. */
    for(uint32_t scale=0;scale<EAR_SCALES_COUNT;scale++)
    for(uint32_t range=0;range<EAR_RANGES_COUNT;range++)
    for(uint32_t count=1;count<=4;count++)for(uint32_t guess=1;guess<=count;guess++)
    for(uint32_t preset=0;preset<FM6_NFAC;preset++) {
        ear_settings_t want={1000,(uint8_t)scale,(uint8_t)count,(uint8_t)guess,(uint8_t)range,(uint8_t)preset};
        ear_settings=want;persist_t p={0};favorites.factory[1][7]=0xA5;
        settings_export(&p);defaults();assert(settings_import(&p,sizeof p));
        assert(ear_settings.scale==want.scale && ear_settings.range==want.range &&
            ear_settings.count==count && ear_settings.guess==guess && ear_settings.preset==preset && ear_settings.gap==1000);
        assert(p.favorites.factory[1][7]==0xA5 && sizeof p==604u);
    }
    uint8_t wire[EAR_SETTINGS_BYTES];ear_settings_pack(wire);
    ear_settings_t out;
    for(uint32_t n=0;n<EAR_SETTINGS_BYTES;n++) assert(!ear_settings_unpack(&out,wire,n));
    for(uint32_t k=0;k<EAR_SETTINGS_BYTES;k++) {
        wire[k]^=1u;assert(!ear_settings_unpack(&out,wire,sizeof wire));wire[k]^=1u;
    }
    /* Correct CRC cannot make invalid values/version legal. */
    const uint8_t field[]={3,4,5,6,7,8,9,10},bad[]={2,2,2,0,5,24,1,255};
    for(uint32_t k=0;k<sizeof field;k++) {
        uint8_t broken[EAR_SETTINGS_BYTES];memcpy(broken,wire,sizeof broken);
        broken[field[k]]=bad[k];broken[11]=ear_settings_crc(broken);
        assert(!ear_settings_unpack(&out,broken,sizeof broken));
    }
    persist_t legacy={0};settings_export(&legacy);
    memset(legacy.favorites.factory[EAR_PREF_ROW]+EAR_PREF_BYTE,0,EAR_SETTINGS_BYTES);
    legacy.palette=palette_to_stored(2);assert(!st_save(OBJ_SETTINGS,&legacy,sizeof legacy));reboot();
    assert(ear_settings.count==2 && ear_settings.guess==1 && ear_settings.gap==250 && !ear_settings.preset);
    assert(settings.palette==2 && favorites.factory[1][7]==0xA5);
    /* A changed preference waits through debounce, pending commands, sound and held input. */
    ear_settings.scale=EAR_MINOR;ear_settings.range=EAR_RANGE_FULL;
    ear_settings.count=4;ear_settings.guess=3;ear_settings.gap=725;ear_settings.preset=17;
    ear_settings_ms=fm1_ms;ear_enabled=1;ear.state=EAR_ANSWER;ear.player=-1;ear.sounding=0;
    ear_keys=0;memset(ear_down,0,sizeof ear_down);
    uint32_t before=erases;settings_save();assert(persist_pending && erases==before);
    fm1_ms+=1000;ear_request=EAR_CMD_NEXT;settings_poll();assert(erases==before);
    ear_request=EAR_CMD_NONE;ear.state=EAR_FIRST;settings_poll();assert(erases==before);
    ear.state=EAR_ANSWER;trk[0].v[0].active=1;settings_poll();assert(erases==before);
    for(uint32_t k=0;k<NPART;k++)for(uint32_t v=0;v<NVOICE;v++)trk[k].v[v].active=0;
    ear_keys=1;settings_poll();assert(erases==before);ear_keys=0;
    settings_poll();assert(erases==before+1 && !persist_pending && !ear_saving && guarded_erases);
    reboot();assert(ear_settings.scale==EAR_MINOR && ear_settings.range==EAR_RANGE_FULL &&
        ear_settings.count==4 && ear_settings.guess==3 && ear_settings.gap==725 && ear_settings.preset==17);
    before=erases;settings_save();assert(erases==before && !persist_pending);
    /* Cut programming leaves the previous A/B record valid, then the queued retry commits. */
    ear_settings.preset=2;fail_after=0;settings_save();assert(persist_pending==2);
    persist_t survived;assert(st_load(OBJ_SETTINGS,&survived,sizeof survived)==sizeof survived);
    defaults();assert(settings_import(&survived,sizeof survived) && ear_settings.preset==17);
    ear_settings.preset=2;fail_after=-1;fm1_ms+=1000;settings_poll();assert(!persist_pending);
    reboot();assert(ear_settings.preset==2 && ear_settings.gap==725);
    /* CRC-damaged preference subrecord falls back without losing other settings. */
    persist_t damaged=persist_saved;damaged.favorites.factory[EAR_PREF_ROW][EAR_PREF_BYTE+4]^=1;
    assert(!st_save(OBJ_SETTINGS,&damaged,sizeof damaged));reboot();
    assert(!ear_settings.preset && ear_settings.count==2 && settings.palette==2 && favorites.factory[1][7]==0xA5);
    puts("Ear settings PASS: PER5 round-trip, legacy/corrupt/truncated validation, idle guard, debounce, reboot and interrupted A/B retry");
    return 0;
}
