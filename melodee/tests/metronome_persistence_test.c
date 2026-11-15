/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
#define PERSISTENCE_TEST_NO_MAIN
#include "persistence_test.c"
#include <assert.h>
int main(void)
{
    reset();persist_boot();
    for(uint32_t m=0;m<METRO_METERS_COUNT;m++)for(uint32_t mode=0;mode<METRO_MODES_COUNT;mode++) {
        metro_settings=metro_default;metro_settings.bpm=300;metro_settings.meter=(uint8_t)m;
        metro_settings.mode=(uint8_t)mode;metro_settings.pattern=METRO_SWING;
        metro_settings.sub=4;metro_settings.volume=0;metro_settings.tolerance=150;metro_settings.silent=4;metro_settings.offset=-100;
        for(uint32_t k=0;k<METRO_USER_COUNT;k++) {metro_user[k]=metro_settings;metro_user[k].bpm=(uint16_t)(30u+k);}
        persist_t p={0};settings_export(&p);uint8_t before[METRO_RECORD_BYTES];metro_pack(before,&metro_settings);
        metro_settings=metro_default;memset(metro_user,0,sizeof metro_user);assert(settings_import(&p,sizeof p));
        uint8_t after[METRO_RECORD_BYTES];metro_pack(after,&metro_settings);assert(!memcmp(before,after,sizeof before));
        for(uint32_t k=0;k<METRO_USER_COUNT;k++)assert(metro_user[k].bpm==30u+k && metro_user[k].offset==-100);
    }
    uint8_t record[METRO_RECORD_BYTES];metro_pack(record,&metro_settings);metro_settings_t s=metro_default;
    for(uint32_t k=0;k<sizeof record;k++) {record[k]^=1u;assert(!metro_unpack(&s,record));record[k]^=1u;}
    const uint8_t fields[]={1,3,4,5,6,7,8,9,10},bad[]={2,255,255,255,0,201,101,0,9};
    for(uint32_t k=0;k<sizeof fields;k++) {
        uint8_t b[METRO_RECORD_BYTES];memcpy(b,record,sizeof b);b[fields[k]]=bad[k];b[11]=metro_crc(b);
        assert(!metro_unpack(&s,b));
    }
    /* Per-slot corruption never destroys other slots, ear prefs or favorites. */
    persist_t p={0};settings_export(&p);p.favorites.factory[14][3]=0xA5;p.favorites.factory[4][7]=0x5A;
    p.favorites.factory[13][8]^=1u;assert(settings_import(&p,sizeof p));
    assert(!metro_user[2].bpm && metro_user[3].bpm==33 && ear_settings.count==2);
    settings_export(&p);assert(p.favorites.factory[14][3]==0xA5 && p.favorites.factory[4][7]==0x5A);
    /* No settings flash erases while metronome is open, even stopped. */
    metro_enabled=1;metro.state=METRO_RUNNING;uint32_t count=erases;settings_save();settings_poll();assert(erases==count && persist_pending);
    metro.state=METRO_STOPPED;settings_poll();assert(erases==count);
    metro_enabled=0;metro_settings.bpm=173;settings_poll();assert(!persist_pending && erases>count);
    metro_settings=metro_default;memset(metro_user,0,sizeof metro_user);persist_boot();assert(metro_settings.bpm==173 && metro_user[3].bpm==33);
    /* Interrupted A/B save retains the previous valid object. */
    metro_settings.bpm=200;fail_after=0;settings_save();assert(persist_pending==2u);
    fail_after=-1;metro_settings=metro_default;persist_boot();assert(metro_settings.bpm==173);
    puts("metronome persistence: all meters/modes, four slots, CRC/bounds, neighbors, deferred flash, reboot and interrupted A/B save PASS");
    return 0;
}
