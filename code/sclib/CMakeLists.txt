cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

add_library(sclib STATIC
	src/accel.c
	src/adc.c
	src/button.c
	src/caster.c
	src/dtw.c
	src/flash_fs.c
	src/led.c
	src/motion_detector.c
	src/sensors.c
	src/signal_store.c
	src/vibration.c
)

target_include_directories(sclib PRIVATE include)
# LittleFS includes.
target_include_directories(sclib PRIVATE ${ZEPHYR_BASE}/../modules/fs/littlefs)
target_include_directories(sclib PRIVATE ${ZEPHYR_BASE}/subsys/fs)

target_link_libraries(sclib PUBLIC zephyr_interface kernel)
