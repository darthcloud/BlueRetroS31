/*
 * Copyright (c) 2019-2026, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GP01_H_
#define _GP01_H_
#include "adapter/adapter.h"

void gp01_init(void);
void gp01_meta_init(struct wired_ctrl *ctrl_data);
void gp01_init_buffer(int32_t dev_mode, struct wired_data *wired_data);
void gp01_from_generic(int32_t dev_mode, struct wired_ctrl *ctrl_data, struct wired_data *wired_data);
void gp01_fb_to_generic(int32_t dev_mode, struct raw_fb *raw_fb_data, struct generic_fb *fb_data);
void gp01_gen_turbo_mask(struct wired_data *wired_data);

#endif /* _GP01_H_ */
