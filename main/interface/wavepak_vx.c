/*
 * Copyright (c) 2019-2026, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "driver/i2c_master.h"
#include "wavepak_vx.h"

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

void wavepak_vx_init(void) {
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
