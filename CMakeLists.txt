
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
    ../firmware/hal/src/photon/wiced/RTOS/NuttX/ver7.8/include
    ../firmware/system/inc
    ../firmware/hal/inc
    ../firmware/hal/shared
    ../firmware/hal/src/stm32f2xx
    ../firmware/services/inc
    ../firmware/wiring/inc
    ../firmware/user/inc
    ../powershield/src
    ../Adafruit/SSD1306/src
    ../OledDsp/src
    ../OneWire/src
    ../TLC59xx/src
    ../ArduinoJson/src
    ../ArduinoJson/src/ArduinoJson
  )


# add targets

add_target(Dispenser)
