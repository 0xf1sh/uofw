/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include <pspidstorage.h>
#include <pspsdk.h>

#include "firmware/magpie.c"
#include "firmware/magpie_helper.c"

void sceWlanDrv_driver_90E5530F(void*, s32, void*, s32);

s32 module_start(SceSize argc __attribute__((unused)), void *argp __attribute__((unused)))
{
    u16 key45;

    // sceIdStorage_driver_6FE062D1
    sceIdStorageLookup(0x45, 0, &key45, sizeof(u16));

    if ((key45 & 0xF000) != 0)
    {
        return 1;
    }

    sceWlanDrv_driver_90E5530F(
        g_wlanfirmHelper, // 0x14C
        g_wlanfirmHelperSize, // 1852
        g_wlanfirm, // 0x888
        g_wlanfirmSize // 87168
    );

    return 0;
}
