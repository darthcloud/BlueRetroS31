idf_build_get_property(project_dir PROJECT_DIR)

set(cov_srcs
    "main.c"
    "adapter/adapter.c"
    "adapter/config.c"
    "adapter/gameid.c"
    "adapter/hid_parser.c"
    "adapter/kb_monitor.c"
    "adapter/macro.c"
    "adapter/mapping_quirks.c"
    "adapter/memory_card.c"
    "adapter/wired/npiso.c"
    "adapter/wired/cdi.c"
    "adapter/wired/saturn.c"
    "adapter/wired/pce.c"
    "adapter/wired/genesis.c"
    "adapter/wired/real.c"
    "adapter/wired/jag.c"
    "adapter/wired/pcfx.c"
    "adapter/wired/jvs.c"
    "adapter/wired/n64.c"
    "adapter/wired/dc.c"
    "adapter/wired/gc.c"
    "adapter/wired/ps.c"
    "adapter/wired/sea.c"
    "adapter/wired/wii.c"
    "adapter/wired/xbox.c"
    "adapter/wired/gp01.c"
    "adapter/wired/parallel_1p.c"
    "adapter/wired/parallel_2p.c"
    "adapter/wired/wired.c"
    "adapter/wireless/hid_generic.c"
    "adapter/wireless/ps3.c"
    "adapter/wireless/wii.c"
    "adapter/wireless/ps.c"
    "adapter/wireless/sw.c"
    "adapter/wireless/sw2.c"
    "adapter/wireless/wireless.c"
    "bluetooth/host.c"
    "bluetooth/hci.c"
    "bluetooth/l2cap.c"
    "bluetooth/sdp.c"
    "bluetooth/att.c"
    "bluetooth/att_cfg.c"
    "bluetooth/att_hid.c"
    "bluetooth/smp.c"
    "bluetooth/mon.c"
    "bluetooth/hidp/hidp.c"
    "bluetooth/hidp/generic.c"
    "bluetooth/hidp/ps3.c"
    "bluetooth/hidp/wii.c"
    "bluetooth/hidp/ps.c"
    "bluetooth/hidp/sw.c"
    "bluetooth/hidp/sw2.c"
    "tools/ps1_gameid.c"
)

if(CONFIG_IDF_TARGET_ESP32)
    list(APPEND cov_srcs
        "tools/devcrypto.c"
    )
endif()

if(CONFIG_BLUERETRO_BT_EXTERNAL_H4)
    list(APPEND cov_srcs
        "bluetooth/hci_uart.c"
    )
endif()

set(srcs
    "system/fs.c"
    "wired/wired_common.c"
    "wired/wired_rtos.c"
    "wired/parallel.c"
)

if(CONFIG_IDF_TARGET_ESP32)
    list(APPEND srcs
    )
endif()

if(CONFIG_BLUERETRO_SYSTEM_XBOX)
    list(APPEND srcs
        "wired/xbox_usb.c"
    )
endif()

if(CONFIG_BLUERETRO_WS_CMDS)
    list(APPEND srcs
        "tests/ws_srv.c"
        "tests/cmds.c"
    )
endif()

if(CONFIG_BLUERETRO_COVERAGE)
    list(APPEND srcs
        "tests/coverage.c"
    )
endif()

#set(embed
#    ${project_dir}/keys/xbox.pem
#)

set(components
    "app_update"
    "bt"
    "driver"
    "efuse"
    "esp_driver_gpio"
    "esp_driver_i2c"
    "esp_driver_ledc"
    "esp_driver_rmt"
    "esp_driver_spi"
    "esp_driver_uart"
    "esp_hal_clock"
    "esp_ringbuf"
    "esp_timer"
    "nvs_flash"
    "spiffs"
)

idf_component_register(
    SRCS "${srcs}" "${cov_srcs}"
    PRIV_INCLUDE_DIRS "."
    LDFRAGMENTS linker.lf
    PRIV_REQUIRES "${components}"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-address-of-packed-member)
target_compile_options(${COMPONENT_LIB} PRIVATE -fno-builtin-memset)
target_compile_options(${COMPONENT_LIB} PRIVATE -fno-builtin-memcpy)

if(CONFIG_BLUERETRO_COVERAGE)
    target_compile_options(${COMPONENT_LIB} PRIVATE -fprofile-info-section)

    set_source_files_properties(
        ${cov_srcs}
        PROPERTIES COMPILE_FLAGS --coverage
    )

    target_link_libraries(${COMPONENT_TARGET} gcov)
endif()

idf_build_set_property(COMPILE_DEFINITIONS "-DBLUERETRO" APPEND)

if(CONFIG_IDF_TARGET_ESP32S2)
    idf_build_set_property(COMPILE_DEFINITIONS "-DCONFIG_ATOMIC_OPERATIONS_BUILTIN" APPEND)

    #create_esp_enc_img(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    #    ${embed} ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}_secure.bin app)
endif()

