idf_component_register(
    SRCS "app_main.c" "chaon_script.c" "chaon_hid.c" "chaon_vm.c"
         "chaon_status.c" "chaon_screen.c" "chaon_lcd.c" "chaon_control.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_timer
)
