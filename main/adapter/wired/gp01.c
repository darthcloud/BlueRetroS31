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
    GP00_C0R0 = 0,
    GP00_C1R0,
    GP00_C2R0,
    GP00_C3R0,
    GP00_C0R1,
    GP00_C1R1,
    GP00_C2R1,
    GP00_C3R1,
    GP01_C0R0,
    GP01_C1R0,
    GP01_C2R0,
    GP01_C3R0,
    GP01_C0R1,
    GP01_C1R1,
    GP01_C2R1,
    GP01_C3R1,
    GP02_C0R0,
    GP02_C1R0,
    GP02_C2R0,
    GP02_C3R0,
    GP02_C0R1,
    GP02_C1R1,
    GP02_C2R1,
    GP02_C3R1,
};

static i2c_master_bus_config_t i2c0_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = 33,
    .sda_io_num = 4,
    .glitch_ignore_cnt = 7,
};
static i2c_master_bus_config_t i2c1_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_1,
    .scl_io_num = 27,
    .sda_io_num = 15,
    .glitch_ignore_cnt = 7,
};
static i2c_device_config_t wp0_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x40,
    .scl_speed_hz = 1000000,
};
static i2c_device_config_t wp1_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x48,
    .scl_speed_hz = 1000000,
};
static i2c_device_config_t wp2_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x50,
    .scl_speed_hz = 1000000,
};
static i2c_master_bus_handle_t i2c0_handle;
static i2c_master_bus_handle_t i2c1_handle;
static i2c_master_dev_handle_t wp00_handle;
static i2c_master_dev_handle_t wp01_handle;
static i2c_master_dev_handle_t wp02_handle;
static i2c_master_dev_handle_t wp10_handle;
static i2c_master_dev_handle_t wp11_handle;
static i2c_master_dev_handle_t wp12_handle;

static DRAM_ATTR const uint8_t gp01_axes_idx[ADAPTER_MAX_AXES] =
{
/*  AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY, TRIG_L, TRIG_R  */
    0,       1,       0,       1,       0,      1
};

static DRAM_ATTR const struct ctrl_meta gp01_axes_meta[GP01_AXES_MAX] =
{
    {.size_min = -512, .size_max = 511, .neutral = 0x200, .abs_max = 0x1FF, .abs_min = 0x200, .polarity = 1},
    {.size_min = -512, .size_max = 511, .neutral = 0x200, .abs_max = 0x1FF, .abs_min = 0x200, .polarity = 1},
};

struct gp01_map {
    union {
        uint8_t btns[4];
        uint32_t btns_raw;
    };
    struct {
        uint8_t axis_addr;
        uint16_t axis;
    } __packed axes[2];
} __packed;

static const uint32_t gp01_mask[4] = {0xBBFF0F0F, 0x00000000, 0x00000000, BR_COMBO_MASK};
static const uint32_t gp01_desc[4] = {0x0000000F, 0x00000000, 0x00000000, 0x00000000};
static DRAM_ATTR const uint32_t gp01_btns_mask[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    BIT(GP01_C0R0), BIT(GP00_C0R0), BIT(GP01_C0R1), BIT(GP00_C0R1),
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};

void gp01_init(void) {
    i2c_new_master_bus(&i2c0_config, &i2c0_handle);
    i2c_master_bus_add_device(i2c0_handle, &wp0_cfg, &wp00_handle);
    i2c_master_bus_add_device(i2c0_handle, &wp1_cfg, &wp01_handle);
    i2c_master_bus_add_device(i2c0_handle, &wp2_cfg, &wp02_handle);
    uint8_t pot_test[] = {0x91, 0x00, 0x80};
    uint8_t oamp_init[] = {0x3E, 0xC0};
    i2c_master_transmit(wp00_handle, (uint8_t *)pot_test, sizeof(pot_test), -1);
    i2c_master_transmit(wp00_handle, (uint8_t *)oamp_init, sizeof(oamp_init), -1);
    i2c_master_transmit(wp01_handle, (uint8_t *)pot_test, sizeof(pot_test), -1);
    i2c_master_transmit(wp01_handle, (uint8_t *)oamp_init, sizeof(oamp_init), -1);
    i2c_master_transmit(wp02_handle, (uint8_t *)pot_test, sizeof(pot_test), -1);
    printf("%s: I2C init done\n", __FUNCTION__);
}

void IRAM_ATTR gp01_init_buffer(int32_t dev_mode, struct wired_data *wired_data) {
    struct gp01_map *map = (struct gp01_map *)wired_data->output;
    struct gp01_map *map_mask = (struct gp01_map *)wired_data->output_mask;

    map->btns_raw = 0x00000000;
    map_mask->btns_raw = 0xFFFFFFFF;
    for (uint32_t i = 0; i < 2; i++) {
        map->axes[i].axis_addr = 0x91;
        map->axes[i].axis = gp01_axes_meta[i].neutral;
    }
    memset(&map_mask->axes, 0x00, sizeof(map_mask->axes));
}

void gp01_meta_init(struct wired_ctrl *ctrl_data) {
    memset((void *)ctrl_data, 0, sizeof(*ctrl_data)*4);

    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        for (uint32_t j = 0; j < GP01_AXES_MAX; j++) {
            switch (config.out_cfg[i].dev_mode) {
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
                map_tmp.btns_raw |= gp01_btns_mask[i];
                map_mask &= ~gp01_btns_mask[i];
                wired_data->cnt_mask[i] = ctrl_data->btns[0].cnt_mask[i];
            }
            else if (map_mask & gp01_btns_mask[i]) {
                map_tmp.btns_raw &= ~gp01_btns_mask[i];
                wired_data->cnt_mask[i] = 0;
            }
        }
    }

    for (uint32_t i = 0; i < GP01_AXES_MAX; i++) {
        if (ctrl_data->map_mask[0] & (axis_to_btn_mask(i) & gp01_desc[0])) {
            if (ctrl_data->axes[i].value > ctrl_data->axes[i].meta->size_max) {
                map_tmp.axes[gp01_axes_idx[i]].axis = ctrl_data->axes[i].meta->size_max;
            }
            else if (ctrl_data->axes[i].value < ctrl_data->axes[i].meta->size_min) {
                map_tmp.axes[gp01_axes_idx[i]].axis = ctrl_data->axes[i].meta->size_min;
            }
            else {
                map_tmp.axes[gp01_axes_idx[i]].axis = (uint16_t)(ctrl_data->axes[i].value + ctrl_data->axes[i].meta->neutral);
            }
            map_tmp.axes[gp01_axes_idx[i]].axis <<= 6;
        }
        wired_data->cnt_mask[axis_to_btn_id(i)] = ctrl_data->axes[i].cnt_mask;
    }

    memcpy(wired_data->output, (void *)&map_tmp, sizeof(map_tmp));

    i2c_master_transmit(wp00_handle, (uint8_t *)&map_tmp.axes[0], 3, -1);
    i2c_master_transmit(wp01_handle, (uint8_t *)&map_tmp.axes[1], 3, -1);
    uint8_t gp_btns[2] = {0x3F, 0x00};
    gp_btns[1] = map_tmp.btns[0];
    i2c_master_transmit(wp00_handle, gp_btns, 2, -1);
    gp_btns[1] = map_tmp.btns[1];
    i2c_master_transmit(wp01_handle, gp_btns, 2, -1);


    TESTS_CMDS_LOG("\"wired_output\": {\"axes\": [%d, %d], \"btns\": %d},\n",
        map_tmp.axes[gp01_axes_idx[0]], map_tmp.axes[gp01_axes_idx[1]], map_tmp.buttons);
    BT_MON_LOG("\"wired_output\": {\"axes\": [%02X, %02X], \"btns\": %04X},\n",
        map_tmp.axes[gp01_axes_idx[0]], map_tmp.axes[gp01_axes_idx[1]], map_tmp.buttons);
}

void gp01_from_generic(int32_t dev_mode, struct wired_ctrl *ctrl_data, struct wired_data *wired_data) {
    switch (dev_mode) {
        case DEV_MOUSE:
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
}
