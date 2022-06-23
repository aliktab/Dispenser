
cmake_minimum_required(VERSION 3.3)

project(Dispenser)


# service functions

function (add_target target_name)

  add_executable(${target_name} ${target_name}/src/${target_name}.cpp)

  set_target_properties(${target_name} PROPERTIES LINKER_LANGUAGE C)

endfunction (add_target)


function (add_lib lib_name)

  add_library(${lib_name} src/${lib_name}.cpp)

endfunction (add_lib)


# common for particle

include_directories(
    ../../Particle/firmware/hal/src/photon/wiced/RTOS/NuttX/ver7.8/include
    ../../Particle/firmware/system/inc
    ../../Particle/firmware/hal/inc
    ../../Particle/firmware/hal/shared
    ../../Particle/firmware/hal/src/stm32f2xx
    ../../Particle/firmware/services/inc
    ../../Particle/firmware/wiring/inc
    ../../Particle/firmware/user/inc
    ../../Particle/powershield/src
    ../../Particle/Adafruit/SSD1306/src
    ../../Particle/OledDsp/src
    ../../Particle/OneWire/src
    ../../Particle/TLC59xx/src
    ../../Particle/ArduinoJson/src
    ../../Particle/ArduinoJson/src/ArduinoJson
  )


# add targets

add_target(Dispenser)
