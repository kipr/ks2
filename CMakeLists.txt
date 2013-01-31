project(ks2)

cmake_minimum_required(VERSION 2.6.0)

find_package(Qt4 REQUIRED)

set(INCLUDE ${CMAKE_SOURCE_DIR}/include)
set(SRC ${CMAKE_SOURCE_DIR}/src)
set(UI ${CMAKE_SOURCE_DIR}/ui)
set(RC ${CMAKE_SOURCE_DIR}/rc)

include_directories(${INCLUDE} ${CMAKE_BINARY_DIR})

file(GLOB INCLUDES ${INCLUDE}/*.hpp)
file(GLOB SOURCES ${SRC}/*.cpp)
file(GLOB UIS ${UI}/*.ui)
file(GLOB RCS ${RC}/*.qrc)

set(QT_USE_QTNETWORK TRUE)

include(${QT_USE_FILE})

qt4_wrap_cpp(SOURCES ${INCLUDES})
qt4_wrap_ui(SOURCES ${UIS})
qt4_add_resources(SOURCES ${RCS})

IF(WIN32)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/../prefix/include)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/../prefix/lib)
ENDIF(WIN32)

IF(APPLE)
	SET(CMAKE_OSX_SYSROOT "${OSX_DEVELOPER_ROOT}/SDKs/MacOSX10.6.sdk")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
	LINK_DIRECTORIES(/Library/Frameworks/)
	ADD_EXECUTABLE(ks2 MACOSX_BUNDLE ${SOURCES})
ELSEIF(UNIX)
	add_definitions(-fPIC)
	ADD_EXECUTABLE(ks2 ${SOURCES})
ELSEIF(WIN32)
	add_definitions("-Wl,--enable-auto-import")
	ADD_EXECUTABLE(ks2 WIN32 ${SOURCES} ${CMAKE_SOURCE_DIR}/compat/mingw.c)
	target_link_libraries(ks2 ws2_32)
ENDIF()

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/deploy)

target_link_libraries(ks2 ${QT_LIBRARIES} kovanserial pcompiler kar)

#######################################
#            Installation             #
#######################################

macro(copy_files NAME FILES DESTINATION)
  add_custom_target(copy_${NAME} ALL
    COMMENT "Copying files: ${FILES}")

  foreach(FILENAME ${FILES})
    get_filename_component(REAL_FILENAME ${FILENAME} NAME)
    set(DST "${DESTINATION}/${REAL_FILENAME}")

    add_custom_command(
      TARGET copy_${NAME}
      COMMAND ${CMAKE_COMMAND} -E copy "${FILENAME}" "${DST}"
      )
  endforeach(FILENAME)
endmacro(copy_files)

IF(APPLE)
	copy_files(target ${RC}/target.h ${CMAKE_SOURCE_DIR}/deploy/ks2.app/Contents/prefix/usr/include)
	copy_files(board ${RC}/2013.board ${CMAKE_SOURCE_DIR}/deploy/ks2.app/Contents/)
	copy_files(platform.hints ${RC}/platform.hints ${CMAKE_SOURCE_DIR}/deploy/ks2.app/Contents)
ELSE(APPLE)
	copy_files(target ${RC}/target.h ${CMAKE_SOURCE_DIR}/deploy/prefix/usr/include)
	copy_files(board ${RC}/2013.board ${CMAKE_SOURCE_DIR}/deploy/)
	copy_files(platform.hints ${RC}/platform.hints ${CMAKE_SOURCE_DIR}/deploy)
ENDIF(APPLE)

IF(APPLE)
	ADD_CUSTOM_TARGET(ks2_MacDeployQt ALL 
		COMMAND ${QT_BINARY_DIR}/macdeployqt ${CMAKE_SOURCE_DIR}/deploy/ks2.app -verbose=0
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS ks2)
	
	ADD_CUSTOM_TARGET(Info.plist ALL 
		COMMAND ${CMAKE_COMMAND} -E copy ${RC}/Info.plist ${CMAKE_SOURCE_DIR}/deploy/ks2.app/Contents
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS ks2)
	
	ADD_CUSTOM_TARGET(Icons ALL 
		COMMAND ${CMAKE_COMMAND} -E copy ${RC}/mac_icon.icns ${CMAKE_SOURCE_DIR}/deploy/ks2.app/Contents/Resources
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		DEPENDS ks2 Info.plist ks2_MacDeployQt)
ENDIF()
