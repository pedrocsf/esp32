# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/pacto/Downloads/arduino-ide_2.3.6_Linux_64bit/esp-idf/components/bootloader/subproject"
  "/home/pacto/Documentos/PlatformIO/Projects/esp32/build/bootloader"
  "/home/pacto/Documentos/PlatformIO/Projects/esp32/build/bootloader-prefix"
  "/home/pacto/Documentos/PlatformIO/Projects/esp32/build/bootloader-prefix/tmp"
  "/home/pacto/Documentos/PlatformIO/Projects/esp32/build/bootloader-prefix/src/bootloader-stamp"
  "/home/pacto/Documentos/PlatformIO/Projects/esp32/build/bootloader-prefix/src"
  "/home/pacto/Documentos/PlatformIO/Projects/esp32/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/pacto/Documentos/PlatformIO/Projects/esp32/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/pacto/Documentos/PlatformIO/Projects/esp32/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
