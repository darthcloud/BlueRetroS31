/*
 * Copyright (c) 2021-2026, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INTERFACE_COMMON_H_
#define _INTERFACE_COMMON_H_

#ifdef CONFIG_BLUERETRO_INTERFACE_DEBUG_LOG
#include <stdio.h>
#define BR_IFACE_DBG_LOG(...) printf(__VA_ARGS__)
#else
#define BR_IFACE_DBG_LOG(...)
#endif
void interface_init(void);
const char *interface_get_sys_name(void);

#endif /* _INTERFACE_COMMON_H_ */
