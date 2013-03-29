/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

SCE_MODULE_INFO("sceClockgen_Driver", 0x1007, 1, 9);
SCE_MODULE_START("_sceClockgenModuleStart");
SCE_MODULE_REBOOT_BEFORE("_sceClockgenModuleRebootBefore");
SCE_SDK_VERSION(SDK_VERSION);

#define LEPTON_CLOCK    (1 << 4) //8
#define AUDIO_CLOCK     (1 << 5) //16

//0x000008F0
typedef struct
{
    u32 mutex;        //0
    u32 protocol;     //4
    u8 registers[6];  //8
    u16 padding;      //E
} ClockGenContext;

ClockGenContext g_Cy27040 = 
{
    0xFFFFFFFF,
    0,
    0,
    {0,0,0,0,0,0,0}
};

//0x00000900
PspSysEventHandler g_ClockGenSysEv =
{
    sizeof(PspSysEventHandler),
    "SceClockgen",
    0x00FFFF00,
    _sceClockgenSysEventHandler,
    0,
    0, 
    NULL, 
    {0,0,0,0,0,0,0,0,0}
};

//0x00000000
int sceClockgenSetup() //sceClockgen_driver_50F22765
{
    if(g_Cy27040.protocol != 0)
    {    
        int s0;
        for(s0 = 0; s0 < 3; s0++)
        {
            u8 table[16];
            u8 back[16];
            table[0] = s0 - 128;
            int ret = sceI2cMasterTransmitReceive(210, table, 1, 210, back, 1); //sceI2c_driver_47BDEAAA
            if(ret < 0) return ret;
            g_Cy27040.registers[s0] = g_Cy27040.registers[s0+3] = back[0];
        }
    }
    else
    {
        u8 table[16];
        u8 back[16];
        table[0] = 0;
    
        int ret = sceI2cMasterTransmitReceive(210, table, 1, 210, back, 16);
        if(ret < 0) return ret;
        if(back[0] == 0) return 0;
        
        int s0;
        for(s0 = 0; (s0 < 3) || (s0 < back[0]); s0++)
        {
            g_Cy27040.registers[s0] = g_Cy27040.registers[s0+3] = back[s0];
        }
    }
    return 0;
}

//0x000000F8
u32 sceClockgenSetSpectrumSpreading(u32 arg) //sceClockgen_driver_C9AF3102
{
    u32 reg_value;
    u32 ret_value;
    
    if(arg < 0)
    {
        reg_value = g_Cy27040.registers[5] & 0x7;
        ret_value = arg;
        if((g_Cy27040.registers[2] & 0x7) == reg_value) return ret_value;
    } else {
        switch(g_Cy27040.registers[0] & 0xF)
        {
            case 0x8:
            case 0xF:
            case 0x7:
            case 0x3:
            case 0x9:
            case 0xA:
                if((arg >= 0) && (arg < 2))
                {
                    reg_value = 0;
                    ret_value = 0;
                }
                else if((arg >= 0) && (arg < 7))
                {
                    reg_value = 1;
                    ret_value = 5;
                }
                else if((arg >= 7) && (arg < 15))
                {
                    reg_value = 2;
                    ret_value = 10;
                }
                else if((arg >= 15) && (arg < 25))
                {
                    reg_value = 4;
                    ret_value = 20;
                }
                else if(arg >= 25)
                {
                    reg_value = 6;
                    ret_value = 30;
                }
                break;
            case 0x4:
                if((arg >= 0) && (arg < 2))
                {
                    reg_value = 0;
                    ret_value = 0;
                }
                else if((arg >= 2) && (arg < 7))
                {
                    reg_value = 1;
                    ret_value = 5;
                }
                else if((arg >= 7) && (arg < 12))
                {
                    reg_value = 2;
                    ret_value = 10;
                }
                else if((arg >= 12) && (arg < 17))
                {
                    reg_value = 3;
                    ret_value = 15;
                }
                else if((arg >= 17) && (arg < 22))
                {
                    reg_value = 4;
                    ret_value = 20;
                }
                else if((arg >= 22) && (arg < 27))
                {
                    reg_value = 5;
                    ret_value = 25;
                }
                else if(arg >= 27)
                {
                    reg_value = 6;
                    ret_value = 30;
                }
                break;
            default:
                return 0x80000004;
        }
    }
    int res = sceKernelLockMutex(g_Cy27040.mutex, 1, 0);
    if(res < 0) return res;
    g_Cy27040.registers[2] = reg_value | (g_Cy27040.registers[2] & 0xFFFFFFF8);
    res = _cy27040_write_register(2, g_Cy27040.registers[2]);
    sceKernelUnlockMutex(g_Cy27040.mutex, 1, 0);
    if(res < 0) return res;
    return ret_value;
}

//0x000002B4
int _sceClockgenModuleStart(SceSize args, void *argp)
{
    sceI2cSetClock(4, 4); //sceI2c_driver_62C7E1E4
    int mutexid = sceKernelCreateMutex("SceClockgen", 1, 0, 0); //ThreadManForKernel_B7D098C6
    if(mutexid >= 0)
    {
        g_Cy27040.mutex = mutexid;
        sceKernelRegisterSysEventHandler(&g_ClockGenSysEv); //sceSysEventForKernel_CD9E4BB5
    }
    g_Cy27040.protocol = 1;
    sceClockgenSetup();
    return 0;
}

//0x00000320
int _sceClockgenModuleRebootBefore(SceSize args, void *argp)
{
    u32 unk1 = g_Cy27040.registers[5];
    u32 unk2 = g_Cy27040.registers[2];
    
    u32 check = unk1 & 7;
    u32 check2 = unk2 & 7;
    unk2 &= 0xFFFFFFF8;
    unk1 = unk2 | check;
    if(check2 != check)
    {
        _cy27040_write_register(2, unk1);
    }
    if(g_Cy27040.mutex >= 0)
    {
        sceKernelDeleteMutex(g_Cy27040.mutex); //ThreadManForKernel_F8170FBE
        g_Cy27040.mutex = 0xFFFFFFFF;
    }
    sceKernelUnregisterSysEventHandler(&g_ClockGenSysEv); //sceSysEventForKernel_D7D3FDCD
    return 0;
}

//0x00000398
void sceClockgenInit()
{
    sceI2cSetClock(4, 4);
    int mutexid = sceKernelCreateMutex("SceClockgen", 1, 0, 0);
    if(mutexid >= 0)
    {
        g_Cy27040.mutex = mutexid;
        sceKernelRegisterSysEventHandler(&g_ClockGenSysEv);
    }
}

//0x000003EC
void sceClockgenEnd() //sceClockgen_driver_36F9B49D
{
    if(g_Cy27040.mutex >= 0)
    {
        sceKernelDeleteMutex(g_Cy27040.mutex);
        g_Cy27040.mutex = 0xFFFFFFFF;
    }
    sceKernelUnregisterSysEventHandler(&g_ClockGenSysEv);
}

//0x00000438
void sceClockgenSetProtocol(u32 prot) //sceClockgen_driver_3F6B7C6B
{
    g_Cy27040.protocol = prot;
}

//0x00000448
u8 sceClockgenGetRevision() //sceClockgen_driver_CE36529C
{
    return g_Cy27040.registers[0];
}

//0x00000454
u8 sceClockgenGetRegValue(u32 arg) //sceClockgen_driver_0FD28D8B
{
    if(arg >= 3)
    {
        return g_Cy27040.registers[arg];
    }
    return 0x80000102;
}

//0x0000047C
u32 sceClockgenAudioClkSetFreq(u32 arg) //sceClockgen_driver_DAB6E612
{
    if(arg == 0xAC44)
    {
        return _sceClockgenSetControl1(1, 0);
    }
    else if(arg == 0xBB80)
    {
        return _sceClockgenSetControl1(1, 1);
    }
    return 0x800001FE;
}

//0x000004BC
u32 sceClockgenAudioClkEnable() //sceClockgen_driver_A1D23B2C
{
    return _sceClockgenSetControl1(AUDIO_CLOCK, 1);
}

//0x000004DC
u32 sceClockgenAudioClkDisable() //sceClockgen_driver_DED4C698
{
    return _sceClockgenSetControl1(AUDIO_CLOCK, 0);
}

//0x000004FC
u32 sceClockgenLeptonClkEnable()  //sceClockgen_driver_7FF82F6F
{
    return _sceClockgenSetControl1(LEPTON_CLOCK, 1);
}

//0x0000051C
u32 sceClockgenLeptonClkDisable() //sceClockgen_driver_DBE5F283
{
    return _sceClockgenSetControl1(LEPTON_CLOCK, 0);
}

//0x0000053C
int _sceClockgenSysEventHandler(int ev_id, char *ev_name, void *param, int *result)
{
    if(ev_id == 0x10000)
    {
        sceKernelTryLockMutex(g_Cy27040.mutex, 1); //ThreadManForKernel_0DDCD2C9
        return 0;
    }
    if((0x10000 < ev_id) == 0) return 0;
    if(ev_id == 0x100000)
    {
        g_Cy27040.registers[1] &= 0xFFFFFFF7;
        _cy27040_write_register(1, g_Cy27040.registers[1] & 0xF7);
        _cy27040_write_register(2, g_Cy27040.registers[2]);
        sceKernelUnlockMutex(g_Cy27040.mutex, 1);
        return 0;
    }
}

//0x000005DC
u32 _sceClockgenSetControl1(int bus, int mode)
{
    int ret = sceKernelLockMutex(g_Cy27040.mutex, 1); //ThreadManForKernel_B011B11F
    if(ret >= 0)
    {
        int ret2 = g_Cy27040.registers[1] & bus;
        
        int val = g_Cy27040.registers[1] | bus;
        int val2 = g_Cy27040.registers[1] & ~bus;
        
        if(ret2 > 0) ret = ret2;
        
        if(mode != 0)
        {
            val2 = val & 0xFF;
        }
        if(val2 != g_Cy27040.registers[1])
        {
            g_Cy27040.registers[1] = val2;
            _cy27040_write_register(1, val2);
        }
        sceKernelUnlockMutex(g_Cy27040.mutex, 1); //ThreadManForKernel_6B30100F
    }
    return ret;
}

//0x00000680
u32 _cy27040_write_register(u8 regid, u8 val)
{
    u8 table[16];
    table[0] = regid - 128;
    table[1] = val;
    if(regid < 3)
    {
        int ret = sceI2cMasterTransmit(210, table, 2); //sceI2c_driver_8CBD8CCF
        if(ret < 0) return ret;
    }
    return 0x80000102;
}
