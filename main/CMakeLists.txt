idf_component_register(
    SRCS "app_main.c" "chaon_script.c" "chaon_hid.c" "chaon_vm.c"
         "chaon_status.c" "chaon_screen.c" "chaon_lcd.c" "chaon_control.c"
         "chaon_sd_probe.c" "chaon_sd_sector.c" "chaon_sd_diag.c"
         "chaon_fat_ro.c" "chaon_fat_file.c" "chaon_inspect.c"
         "chaon_inspect_mailbox.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_driver_sdmmc sdmmc esp_timer
)
