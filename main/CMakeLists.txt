file(GLOB my_srcs 
main.cpp *.c *.cpp hw/*.c hw/*.cpp dsp/*.c dsp/*.cpp)

idf_component_register(
SRCS ${my_srcs}
INCLUDE_DIRS "." "hw" "dsp"
REQUIRES freertos bootloader_support nvs_flash driver spi_flash heap esp_rom fatfs sdmmc vfs app_update esp32)
