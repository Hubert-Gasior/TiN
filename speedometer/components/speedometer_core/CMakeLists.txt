set(src_files
    "source/bluetooth.c"
    "source/halAH49E.c"
    "source/i2c.c"
    "source/imuB160.c"
    "source/led.c"
    "source/linearBuffer.c"
)

idf_component_register( SRCS ${src_files}
                        INCLUDE_DIRS "include"
                        REQUIRES bt esp_driver_spi driver nvs_flash
                        )