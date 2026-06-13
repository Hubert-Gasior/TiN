set(src_files
    "bluetooth_test.c"
    "halAH59E_test.c"
    "i2c_test.c"
    "imuB160_test.c"
    "linearBuffer_test.c"
    "main_test.c"
)

idf_component_register(SRCS ${src_files}
                       INCLUDE_DIRS "."
                       REQUIRES unity speedometer_core )