/* Copyright (C) 2011, 2012, 2013 The uOFW team    
   See the file COPYING for copying permission.
*/

#include "usersystemlib_kernel.h"

#include <common_imp.h>
#include <interruptman.h>

SCE_MODULE_INFO("sceKernelLibrary",
    SCE_MODULE_KERNEL | SCE_MODULE_ATTR_CANT_STOP |
    SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 1);
SCE_MODULE_BOOTSTART("_UserSystemLibInit");

typedef struct {
    u32 unk0[48]; // 0
    u32 id; // 192
    u32 unk2; // 196
    u32 frameSize; // 200
} SceThread;

// .rodata
static const s32 g_userSpaceIntrStackSize = 0x2000; // b68

// .data
static s32 g_b80 = -1; // b80

// .bss
static u8 g_userSpaceIntrStack[0x2000]; // bc0
static SceThread *g_thread; // 2bc0
static u8 g_unk_2bc4[0x3C]; // 2bc4
static u8 g_2c00[0x140]; // 2c00

// module_start
s32 _UserSystemLibInit(SceSize argc __attribute__((unused)), void *argp)
{
    // InterruptManager_EEE43F47
    sceKernelRegisterUserSpaceIntrStack(
        (s32)g_userSpaceIntrStack, // 0xBC0
        g_userSpaceIntrStackSize, // 0x2000
        (s32)&g_thread // 0x2BC0
    );

    // SysMemUserForUser_A6848DF8
    sceKernelSetUsersystemLibWork(
        g_2c00, // 0x2C00
        0x140,
        &g_b80 // 0xB80
    );

    return SCE_ERROR_OK;
}

// Kernel_Library_293B45B8
s32 sceKernelGetThreadId(void)
{
    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    return g_thread->id;
}

// Kernel_Library_D13BDE95
s32 sceKernelCheckThreadStack(void)
{
    u32 available;

    if (g_thread == NULL) {
        // ThreadManForUser_D13BDE95
        return sceKernelCheckThreadStack();
    }

    available = pspGetSp() - g_thread->frameSize;

    if (available < 64) {
        // ThreadManForUser_D13BDE95
        return sceKernelCheckThreadStack();
    }

    return available;
}

// Kernel_Library_DC692EE3
s64 sceKernelTryLockLwMutex(SceLwMutex *mutex, u32 count)
{
    // Kernel_Library_37431849
    if (sceKernelTryLockLwMutex_600(mutex, count) != SCE_ERROR_OK) {
        // 0x800201C4
        return SCE_ERROR_KERNEL_MUTEX_LOCKED;
    }

    return SCE_ERROR_OK;
}

// Kernel_Library_37431849
// reference: http://linux.die.net/man/3/pthread_mutex_trylock
s64 sceKernelTryLockLwMutex_600(SceLwMutex *mutex, u32 count)
{
    u32 tmpCount;
    u32 tmpThid;

    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (count <= 0) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    if (mutex->id < 0) {
        // 0x800201CA
        return SCE_ERROR_KERNEL_LWMUTEX_NOT_FOUND;
    }

    if (mutex->thid == g_thread->id) { // loc_00000340
        if (!(mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE) {
            // 0x800201CF
            return SCE_ERROR_KERNEL_LWMUTEX_RECURSIVE_NOT_ALLOWED;
        }

        if (mutex->lockCount + count < 0) {
            // 0x800201CD
            return SCE_ERROR_KERNEL_LWMUTEX_LOCK_OVERFLOW;
        }

        mutex->lockCount += count;

        return SCE_ERROR_OK;
    }

    if (mutex->thid != 0) {
        // 0x800201CB
        return (1 << 32) | SCE_ERROR_KERNEL_LWMUTEX_LOCKED;
    }

    if (mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE) {
        tmpCount = 0;
    } else {
        tmpCount = count ^ 1;
    }

    if (tmpCount != 0) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    do { // loc_00000320
        /* begin atomic RMW */
        asm __volatile__(
            "ll %0, (%1)"
            : "=r" (tmpThid)
            : "r" (&mutex->thid)
        );

        if (tmpThid != 0) {
            // 0x800201CB
            return (1 << 32) | SCE_ERROR_KERNEL_LWMUTEX_LOCKED;
        }

        tmpThid = g_thread->id;

        /* end atomic RMW */
        /* if an atomic update as occured, %0 will be set to 1 */
        asm __volatile__(
            "sc %0, (%1)"
            : "=r" (tmpThid)
            : "r" (&mutex->thid)
        );
    } while (tmpThid == 0);

    mutex->lockCount = count;

    return SCE_ERROR_OK;
}

// Kernel_Library_1FC64E09
s64 sceKernelLockLwMutexCB(SceLwMutex *mutex, u32 count)
{
    s64 ret;

    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (!sceKernelIsCpuIntrEnable() || g_thread->unk2 != 0) {
        // 0x800201A7
        return SCE_ERROR_KERNEL_WAIT_CAN_NOT_WAIT;
    }

    ret = sceKernelTryLockLwMutex_600(mutex, count);

    /* mutex already locked, block until it is available */
    if ((ret >> 32) != 0) {
        return ThreadManForUser_31327F19(mutex, count);
    }

    return (s32)ret;
}

// Kernel_Library_BEA46419
s64 sceKernelLockLwMutex(SceLwMutex *mutex, u32 count)
{
    s64 ret;

    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (!sceKernelIsCpuIntrEnable() || g_thread->unk2 != 0) {
        // 0x800201A7
        return SCE_ERROR_KERNEL_WAIT_CAN_NOT_WAIT;
    }

    ret = sceKernelTryLockLwMutex_600(mutex, count);

    /* mutex already locked, block until it is available */
    if ((ret >> 32) != 0) {
        return ThreadManForUser_7CFF8CF3(mutex, count);
    }

    return ret;
}

// Kernel_Library_15B6446B
// reference: http://linux.die.net/man/3/pthread_mutex_unlock
s32 sceKernelUnlockLwMutex(SceLwMutex *mutex, u32 count)
{
    u32 tmpCount;
    u32 tmpThid;

    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (count <= 0) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    if (mutex->id < 0) {
        // 0x800201CA
        return SCE_ERROR_KERNEL_LWMUTEX_NOT_FOUND;
    }

    if (mutex->flags & SCE_KERNEL_LWMUTEX_RECURSIVE) {
        tmpCount = 0;
    } else {
        tmpCount = count ^ 1;
    }

    if (tmpCount != 0) {
        // 0x800201BD
        return SCE_ERROR_KERNEL_ILLEGAL_COUNT;
    }

    if (mutex->thid != g_thread->id) {
        return SCE_ERROR_KERNEL_LWMUTEX_UNLOCKED;
    }

    tmpCount = mutex->lockCount;

    if (mutex->lockCount - count > 0) {
        mutex->lockCount -= count;
        return SCE_ERROR_OK;
    }

    // loc_00000408

    if (mutex->lockCount - count < 0) {
        return SCE_ERROR_KERNEL_LWMUTEX_UNLOCK_UNDERFLOW;
    }

    mutex->lockCount = 0;

    /* begin atomic RMW */
    asm __volatile__(
        "ll %0, (%1)"
        : "=r" (tmpThid)
        : "r" (&mutex->unk3)
    );

    if (tmpThid != 0) {
        mutex->lockCount = tmpCount;
        return ThreadManForUser_BEED3A47(mutex, count);
    }

    tmpThid = 0;

    /* end atomic RMW */
    /* if an atomic update as occured, %0 will be set to 1 */
    asm __volatile__(
        "sc %0, (%1)"
        : "=r" (tmpThid)
        : "r" (&mutex->thid)
    );

    if (tmpThid == 0) {
        mutex->lockCount = tmpCount;
        return ThreadManForUser_BEED3A47(mutex, count);
    }

    return SCE_ERROR_OK;
}

// Kernel_Library_3AD10D4D
s32 Kernel_Library_3AD10D4D(SceLwMutex *mutex)
{
    if (g_thread == NULL) {
        // 0x80020064
        return SCE_ERROR_KERNEL_CANNOT_BE_CALLED_FROM_INTERRUPT;
    }

    if (mutex->id < 0) {
        // 0x800201CA
        return SCE_ERROR_KERNEL_LWMUTEX_NOT_FOUND;
    }

    if ((mutex->thid != 0) ^ (g_thread->id != 0)) {
        return 0;
    }

    return mutex->lockCount;
}

// Kernel_Library_C1734599
s32 sceKernelReferLwMutexStatus(SceLwMutex *mutex, u32 *addr)
{
    // ThreadManForUser_4C145944
    return sceKernelReferLwMutexStatusByID(mutex->id, addr);
}

s32 Kernel_Library_F1835CDE(void)
{
    return SCE_ERROR_OK;
}

void *sceKernelMemcpy(void *dst, const void *src, u32 size)
{
    return NULL;
}

void *sceKernelMemset(void *dst, s32 val, u32 size)
{
    return NULL;
}
