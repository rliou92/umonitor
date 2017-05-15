cmake_minimum_required (VERSION 2.6)
project (read-edid)

option(I2CBUILD "Build I2C get-edid implementation" ON)
option(CLASSICBUILD "Build VBE get-edid implementation" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
	"${CMAKE_SOURCE_DIR}/cmake/Modules")

if (I2CBUILD OR CLASSICBUILD)
	add_subdirectory (get-edid)
endif ()
add_subdirectory (parse-edid)

INSTALL(FILES get-edid.1 DESTINATION share/man/man1)
INSTALL(FILES AUTHORS ChangeLog COPYING README DESTINATION
	share/doc/read-edid)
