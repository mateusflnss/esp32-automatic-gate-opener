idf_component_register(
    SRCS "ota_module.c" "espnow_comm.c" "state_machine.c" "rolling_code.c" "button_handler.c" "ring_buffer.c" "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_driver_gpio 
)
