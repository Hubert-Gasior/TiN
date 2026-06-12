# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# --- DODAJ TEN BLOK ---
# Jeśli celem kompilacji jest komputer (Windows/Linux), zamień sprzęt na mocki
if(CONFIG_IDF_TARGET_LINUX)
    idf_build_mock_component(nvs_flash)
    idf_build_mock_component(bt)
    
    # Uwaga: jeśli NimBLE rzuci błędem o brakach, odkomentuj poniższą linię:
    # idf_build_mock_component(nimble) 
endif()
# ----------------------

add_compile_options("-DSP_DEBUG")
project(speedometer)
