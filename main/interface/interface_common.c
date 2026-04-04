/*
 * Copyright (c) 2021-2026, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include "adapter/adapter.h"
#include "interface_common.h"
#include "wavepak_vx.h"
#include "xbox_usb.h"

void interface_init(void) {
#if defined(CONFIG_BLUERETRO_SYSTEM_WAVEPAK_VX)
    wavepak_vx_init();
#elif defined(CONFIG_BLUERETRO_SYSTEM_XBOX)
    xbox_init();
#endif
}

const char *interface_get_sys_name(void) {
#if defined(CONFIG_BLUERETRO_SYSTEM_WAVEPAK_VX)
    return "VECTREX";
#elif defined(CONFIG_BLUERETRO_SYSTEM_XBOX)
    return "XBOX";
#else
    return "NONE";
#endif
}
