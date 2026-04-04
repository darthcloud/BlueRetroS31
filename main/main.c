/*
 * Copyright (c) 2019-2026, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_ota_ops.h>
#include <soc/efuse_reg.h>
#include <esp_efuse.h>
#include <esp_rom_sys.h>
#include "system/fs.h"
#include "adapter/adapter.h"
#include "adapter/config.h"
#include "bluetooth/host.h"
#include "interface/interface_common.h"
#include "adapter/memory_card.h"
#include "tests/ws_srv.h"
#include "tests/coverage.h"
#include "sdkconfig.h"

void app_main()
{
    uint32_t err = 0;
    const esp_partition_t *running = NULL;

    adapter_init();
    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        adapter_init_buffer(i);
    }

    interface_init();

    printf("# SYSTEM FW: %s\n", wired_get_sys_name());

    running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    esp_ota_get_state_partition(running, &ota_state);
    printf("# Booted from %s\n", running->label);

#ifdef CONFIG_BLUERETRO_COVERAGE
    cov_init();
#endif

#ifndef CONFIG_BLUERETRO_QEMU
    if (fs_init()) {
        err_led_set();
        err = 1;
        printf("FS init fail!\n");
    }
#endif

    /* Initialize NVS for PHY cal and default config */
    int32_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    config_init(DEFAULT_CFG);
    mc_init_mem();

#ifndef CONFIG_BLUERETRO_BT_DISABLE
    if (bt_host_init()) {
        err = 1;
        printf("Bluetooth init fail!\n");
    }
#endif


#ifndef CONFIG_BLUERETRO_QEMU
    mc_init();
#endif

#ifdef CONFIG_BLUERETRO_WS_CMDS
    ws_srv_init();
#endif

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        if (err) {
            printf("Boot error on pending img, rollback to the previous version.");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
        else {
            printf("Pending img valid!");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}
