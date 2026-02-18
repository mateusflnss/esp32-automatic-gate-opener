idf_component_register(
    SRCS "espnow_config.c" "nvs_config.c" "gpio_config.c" "state_machine.c" "event_processing.c" "ring_buffer.c" "ota_module.c" "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash
        )
