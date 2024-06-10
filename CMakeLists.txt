cmake_minimum_required(VERSION 3.9)

project(hearing_aid_gatt)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)

if (ENABLE_ASAN)
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
   set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0)


add_executable(gatt_dump
   src/Bluetooth.cxx
   src/Characteristic.cxx
   src/GVariantDump.cxx

   gatt_dump.cxx
)
target_link_libraries(gatt_dump PkgConfig::GLIB)

