/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
#define PERSISTENCE_TEST_NO_MAIN
#include "persistence_test.c"
#include <assert.h>
int main(void)
{
    reset();persist_boot();
    for(uint32_t m=0;m<METRO_METERS_COUNT;m++)for(uint32_t mode=0;mode<METRO_MODES_COUNT;mode++)
    for(uint32_t game=0;game<METRO_GAMES_COUNT;game++)for(uint32_t cycle=0;cycle<METRO_CYCLES_COUNT;cycle++) {
        metro_settings=metro_default;metro_settings.bpm=300;metro_settings.meter=(uint8_t)m;
        metro_settings.mode=(uint8_t)mode;metro_settings.game=(uint8_t)game;
        metro_settings.cycle=(uint8_t)cycle;metro_settings.offset=-100;metro_normalize(&metro_settings);
        for(uint32_t k=0;k<METRO_USER_COUNT;k++) {metro_user[k]=metro_settings;metro_user[k].bpm=(uint16_t)(30u+k);}
        persist_t p={0};settings_export(&p);uint8_t before[METRO_RECORD_BYTES];metro_pack(before,&metro_settings);
        metro_settings=metro_default;memset(metro_user,0,sizeof metro_user);assert(settings_import(&p,sizeof p));
        uint8_t after[METRO_RECORD_BYTES];metro_pack(after,&metro_settings);assert(!memcmp(before,after,sizeof before));
        for(uint32_t k=0;k<METRO_USER_COUNT;k++)assert(metro_user[k].bpm==30u+k && metro_user[k].offset==-100);
    }
    uint8_t record[METRO_RECORD_BYTES];metro_pack(record,&metro_settings);metro_settings_t s=metro_default;
    for(uint32_t k=0;k<sizeof record;k++) {record[k]^=1u;assert(!metro_unpack(&s,record));record[k]^=1u;}
    const uint8_t fields[]={1,3,4,5,6,7,8,9,10},bad[]={3,255,255,255,0,201,99,2,59};
    for(uint32_t k=0;k<sizeof fields;k++) {
        uint8_t b[METRO_RECORD_BYTES];memcpy(b,record,sizeof b);b[fields[k]]=bad[k];b[11]=metro_crc(b);
        assert(!metro_unpack(&s,b));
    }
    /* Version-one preferences and all user slots migrate without changing
     * tempo or calibration. Fixed controls replace old volume/tolerance. */
    for(uint32_t mode=0;mode<3u;mode++)for(uint32_t sub=1;sub<=4u;sub++) {
        uint8_t old[METRO_RECORD_BYTES]={'M',1,173,0,0,(uint8_t)(mode<<4),(uint8_t)sub,77,23,4,150,0};
        old[11]=metro_crc(old);metro_settings_t migrated;
        assert(metro_unpack(&migrated,old));
        assert(migrated.bpm==173 && migrated.offset==-23);
        assert(migrated.mode==(mode?METRO_GAME:METRO_PRACTICE));
        assert(migrated.game==(mode==2u?METRO_EVENS:METRO_FULL_BARS));
        assert(migrated.cycle==METRO_CYCLE_2_2);
        assert(migrated.meter==(sub==1u?0u:METRO_METER_4_4_EIGHTHS+sub-2u));
        uint8_t upgraded[METRO_RECORD_BYTES];metro_pack(upgraded,&migrated);
        assert(upgraded[1]==2 && upgraded[6]==1 && upgraded[8]==100 && upgraded[10]==60);
        old[4]=19;old[11]=metro_crc(old);s=metro_default;
        assert(!metro_unpack(&s,old) && s.bpm==metro_default.bpm);
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
