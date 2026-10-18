idf_component_register(
    SRCS "app_main.c" "chaon_script.c" "chaon_hid.c" "chaon_vm.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio esp_timer
)
