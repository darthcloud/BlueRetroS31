/*
 * Copyright (c) 2019-2026, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "driver/i2c_master.h"
#include "zephyr/types.h"
#include "tools/util.h"
#include "adapter/adapter.h"
#include "adapter/config.h"
#include "adapter/wired/wired.h"
#include "adapter/wireless/wireless.h"
#include "tests/cmds.h"
#include "bluetooth/mon.h"
#include "gp01.h"

#define GP01_AXES_MAX 2
enum {
    GP01_KP1,
    GP01_KP2,
    GP01_KP3,
    GP01_START,
    GP01_KP4,
    GP01_KP5,
    GP01_KP6,
    GP01_PAUSE,
    GP01_T_TRIG,
    GP01_B_TRIG,
};

static i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = 19,
    .sda_io_num = 22,
    .glitch_ignore_cnt = 7,
};
static i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x08,
    .scl_speed_hz = 1000000,
};
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

static DRAM_ATTR const uint8_t gp01_axes_idx[ADAPTER_MAX_AXES] =
{
/*  AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY, TRIG_L, TRIG_R  */
    0,       1,       0,       1,       0,      1
};

static DRAM_ATTR const struct ctrl_meta gp01_axes_meta[GP01_AXES_MAX] =
{
    {.size_min = -512, .size_max = 511, .neutral = 0x200, .abs_max = 0x1FF, .abs_min = 0x200},
    {.size_min = -512, .size_max = 511, .neutral = 0x200, .abs_max = 0x1FF, .abs_min = 0x200, .polarity = 1},
};

static DRAM_ATTR const struct ctrl_meta gp01_mouse_axes_meta[GP01_AXES_MAX] =
{
    {.size_min = -512, .size_max = 511, .neutral = 0x200, .abs_max = 0x1FF, .abs_min = 0x200},
    {.size_min = -512, .size_max = 511, .neutral = 0x200, .abs_max = 0x1FF, .abs_min = 0x200, .polarity = 1},
};

struct gp01_map {
    struct {
        uint8_t btn_addr;
        uint16_t buttons;
    } __packed btn;
    struct {
        uint8_t axes_addr;
        uint16_t axes[2];
    } __packed axes;
} __packed;

static const uint32_t gp01_mask[4] = {0xBBFF0F0F, 0x00000000, 0x00000000, BR_COMBO_MASK};
static const uint32_t gp01_desc[4] = {0x0000000F, 0x00000000, 0x00000000, 0x00000000};
static DRAM_ATTR const uint32_t gp01_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(GP01_KP4), BIT(GP01_KP6), 0, BIT(GP01_KP2),
    0, 0, 0, 0,
    BIT(GP01_T_TRIG), BIT(GP01_KP3), BIT(GP01_B_TRIG), BIT(GP01_KP1),
    BIT(GP01_START), BIT(GP01_PAUSE), 0, BIT(GP01_KP5),
    0, 0, 0, 0,
    0, 0, 0, 0,
};

static const uint32_t gp01_mouse_mask[4] = {0x110000F0, 0x00000000, 0x00000000, BR_COMBO_MASK};
static const uint32_t gp01_mouse_desc[4] = {0x000000F0, 0x00000000, 0x00000000, 0x00000000};
static const uint32_t gp01_mouse_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(GP01_T_TRIG), 0, 0, 0,
    BIT(GP01_B_TRIG), 0, 0, 0,
};

void gp01_init(void) {
    i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    uint8_t pot_test[] = {0x91, 0x00, 0x80, 0x00, 0x80};
    i2c_master_transmit(dev_handle, (uint8_t *)pot_test, sizeof(pot_test), -1);
    printf("%s: I2C init done\n", __FUNCTION__);
}

void IRAM_ATTR gp01_init_buffer(int32_t dev_mode, struct wired_data *wired_data) {
    struct gp01_map *map = (struct gp01_map *)wired_data->output;
    struct gp01_map *map_mask = (struct gp01_map *)wired_data->output_mask;

    map->btn.btn_addr = 0x3E;
    map->btn.buttons = 0x0000;
    map_mask->btn.buttons = 0xFFFF;
    map->axes.axes_addr = 0x91;
    for (uint32_t i = 0; i < GP01_AXES_MAX; i++) {
        map->axes.axes[gp01_axes_idx[i]] = gp01_axes_meta[i].neutral;
    }
    memset(map_mask->axes.axes, 0x00, sizeof(map_mask->axes));
}

void gp01_meta_init(struct wired_ctrl *ctrl_data) {
    memset((void *)ctrl_data, 0, sizeof(*ctrl_data)*4);

    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        for (uint32_t j = 0; j < GP01_AXES_MAX; j++) {
            switch (config.out_cfg[i].dev_mode) {
                case DEV_MOUSE:
                    ctrl_data[i].mask = gp01_mouse_mask;
                    ctrl_data[i].desc = gp01_mouse_desc;
                    ctrl_data[i].axes[j + 2].meta = &gp01_mouse_axes_meta[j];
                    break;
                case DEV_PAD:
                default:
                    ctrl_data[i].mask = gp01_mask;
                    ctrl_data[i].desc = gp01_desc;
                    ctrl_data[i].axes[j].meta = &gp01_axes_meta[j];
                    break;
            }
        }
    }
}

static void gp01_ctrl_from_generic(struct wired_ctrl *ctrl_data, struct wired_data *wired_data) {
    struct gp01_map map_tmp;
    uint32_t map_mask = 0xFFFF;

    memcpy((void *)&map_tmp, wired_data->output, sizeof(map_tmp));

    for (uint32_t i = 0; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if (ctrl_data->map_mask[0] & generic_btns_mask[i]) {
            if (ctrl_data->btns[0].value & generic_btns_mask[i]) {
                map_tmp.btn.buttons |= gp01_btns_mask[i];
                map_mask &= ~gp01_btns_mask[i];
                wired_data->cnt_mask[i] = ctrl_data->btns[0].cnt_mask[i];
            }
            else if (map_mask & gp01_btns_mask[i]) {
                map_tmp.btn.buttons &= ~gp01_btns_mask[i];
                wired_data->cnt_mask[i] = 0;
            }
        }
    }

    for (uint32_t i = 0; i < GP01_AXES_MAX; i++) {
        if (ctrl_data->map_mask[0] & (axis_to_btn_mask(i) & gp01_desc[0])) {
            if (ctrl_data->axes[i].value > ctrl_data->axes[i].meta->size_max) {
                map_tmp.axes.axes[gp01_axes_idx[i]] = ctrl_data->axes[i].meta->size_max;
            }
            else if (ctrl_data->axes[i].value < ctrl_data->axes[i].meta->size_min) {
                map_tmp.axes.axes[gp01_axes_idx[i]] = ctrl_data->axes[i].meta->size_min;
            }
            else {
                map_tmp.axes.axes[gp01_axes_idx[i]] = (uint16_t)(ctrl_data->axes[i].value + ctrl_data->axes[i].meta->neutral);
            }
            map_tmp.axes.axes[gp01_axes_idx[i]] <<= 6;
        }
        wired_data->cnt_mask[axis_to_btn_id(i)] = ctrl_data->axes[i].cnt_mask;
    }

    memcpy(wired_data->output, (void *)&map_tmp, sizeof(map_tmp));

    i2c_master_transmit(dev_handle, (uint8_t *)&map_tmp.axes, sizeof(map_tmp.axes), -1);

    TESTS_CMDS_LOG("\"wired_output\": {\"axes\": [%d, %d], \"btns\": %d},\n",
        map_tmp.axes[gp01_axes_idx[0]], map_tmp.axes[gp01_axes_idx[1]], map_tmp.buttons);
    BT_MON_LOG("\"wired_output\": {\"axes\": [%02X, %02X], \"btns\": %04X},\n",
        map_tmp.axes[gp01_axes_idx[0]], map_tmp.axes[gp01_axes_idx[1]], map_tmp.buttons);
}

static void gp01_mouse_from_generic(struct wired_ctrl *ctrl_data, struct wired_data *wired_data) {
    struct gp01_map map_tmp;
    int32_t *raw_axes = (int32_t *)(wired_data->output + 4);

    memcpy((void *)&map_tmp, wired_data->output, sizeof(map_tmp));

    for (uint32_t i = 0; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if (ctrl_data->map_mask[0] & BIT(i)) {
            if (ctrl_data->btns[0].value & generic_btns_mask[i]) {
                map_tmp.btn.buttons |= gp01_mouse_btns_mask[i];
            }
            else {
                map_tmp.btn.buttons &= ~gp01_mouse_btns_mask[i];
            }
        }
    }

    for (uint32_t i = 2; i < 4; i++) {
        if (ctrl_data->map_mask[0] & (axis_to_btn_mask(i) & gp01_mouse_desc[0])) {
            if (ctrl_data->axes[i].relative) {
                atomic_add(&raw_axes[gp01_axes_idx[i]], ctrl_data->axes[i].value);
            }
            else {
                raw_axes[gp01_axes_idx[i]] = ctrl_data->axes[i].value;
            }
        }
    }

    memcpy(wired_data->output, (void *)&map_tmp, sizeof(map_tmp) - 8);
}

void gp01_from_generic(int32_t dev_mode, struct wired_ctrl *ctrl_data, struct wired_data *wired_data) {
    switch (dev_mode) {
        case DEV_MOUSE:
            gp01_mouse_from_generic(ctrl_data, wired_data);
            break;
        case DEV_PAD:
        default:
            gp01_ctrl_from_generic(ctrl_data, wired_data);
            break;
    }
}

void gp01_fb_to_generic(int32_t dev_mode, struct raw_fb *raw_fb_data, struct generic_fb *fb_data) {
    fb_data->wired_id = raw_fb_data->header.wired_id;
    fb_data->type = raw_fb_data->header.type;

    /* This stop rumble when BR timeout trigger */
    if (raw_fb_data->header.data_len == 0) {
        fb_data->state = 0;
        fb_data->lf_pwr = fb_data->hf_pwr = 0;
    }
    else {
        fb_data->state = raw_fb_data->data[0];
        fb_data->lf_pwr = (fb_data->state) ? 0xFF : 0x00;
        fb_data->hf_pwr = (fb_data->state) ? 0xFF : 0x00;
    }
}

void IRAM_ATTR gp01_gen_turbo_mask(struct wired_data *wired_data) {
    struct gp01_map *map_mask = (struct gp01_map *)wired_data->output_mask;

    memset(map_mask, 0xFF, sizeof(*map_mask));

    wired_gen_turbo_mask_btns16_pos(wired_data, &map_mask->btn.buttons, gp01_btns_mask);
    wired_gen_turbo_mask_axes16(wired_data, map_mask->axes.axes, GP01_AXES_MAX, gp01_axes_idx, gp01_axes_meta);
}
