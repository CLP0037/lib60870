#cmake verson，指定cmake版本
#cmake_minimum_required(VERSION 2.8)
cmake_minimum_required(VERSION 3.13)

SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
# automagically detect if we should cross-compile

if(DEFINED ENV{TOOLCHAIN})
	set(CMAKE_C_COMPILER	$ENV{TOOLCHAIN}gcc)
	set(CMAKE_CXX_COMPILER	$ENV{TOOLCHAIN}g++)
	set(CMAKE_AR	"$ENV{TOOLCHAIN}ar" CACHE FILEPATH "CW archiver" FORCE)
endif()


#INCLUDE(${QT_USE_FILE})
#set(ENV{Qt5Core_DIR} "/usr/lib/arm-linux-gnueabihf/cmake/Qt5Core")
#MESSAGE( STATUS $ENV{Qt5Core_DIR} )
#MESSAGE( STATUS "QTDIR?:         " ${QT_USE_FILE} )

#project name，指定项目的名称，一般和项目的文件夹名称对应
project(lib60870-C)
ENABLE_TESTING()
#设置基本tag（版本信息）：2.0.0
set(LIB_VERSION_MAJOR "2")
set(LIB_VERSION_MINOR "0")
set(LIB_VERSION_PATCH "0")

#In order for find_package to be successful, Qt 5 must be found below the CMAKE_PREFIX_PATH,
#or the Qt5<Module>_DIR must be set in the CMake cache to the location of the Qt5WidgetsConfig.cmake file.
#The easiest way to use CMake is to set the CMAKE_PREFIX_PATH environment variable to the install prefix of Qt 5.

#set (QTD "${QTDIR}")
set(QTD "$ENV{QTDIR}")
MESSAGE( STATUS "QTDIR:         " ${QTD} )

IF (WIN32)      #qDebug in windows, not in Linux as gcc does not work with C including c++ header
#5.2.1  QTDIR：D:\Qt\Qt5.2.1
#set(CMAKE_PREFIX_PATH $ENV{QTDIR}/5.2.1/msvc2010/lib/cmake/Qt5Core)
#5.9.7  QTDIR：C:\Qt\Qt5.9.7\5.9.7\msvc2017_64
set(CMAKE_PREFIX_PATH $ENV{QTDIR}/lib/cmake/Qt5Core)
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/src/inc/vs  #stdbool.h
    ${QTD}/include/QtCore
    ${QTD}/include
)
find_package(Qt5Core REQUIRED)
ENDIF (WIN32)
MESSAGE( STATUS "CMAKE_CURRENT_LIST_DIR:         " ${CMAKE_CURRENT_LIST_DIR} )


## Appends the cmake/modules path to MAKE_MODULE_PATH variable.
#${CMAKE_CURRENT_SOURCE_DIR} : CMakeLists.txt所在路径
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/third_party/cmake/modules/")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"  ${CMAKE_MODULE_PATH})
message(STATUS "CMAKE_MODULE_PATH : " ${CMAKE_MODULE_PATH})

include(GetGitRevisionDescription)
git_describe(VERSION --tags --dirty=-d)

###parse the version information into pieces.
#VERSION ~= s/\.[a-f0-9]{5,}//
#GIT_VERSION ~= s/-/"."
#GIT_VERSION ~= s/g/""
#VERSION ~= s/\.[a-f0-9]{5,}
STRING( REPLACE "-" "." VERSION ${VERSION})
STRING( REPLACE "g" "" VERSION ${VERSION})
#STRING( REPLACE "g" "" VERSION ${VERSION})
MESSAGE( STATUS "VERSION:         " ${VERSION} )

string(REGEX REPLACE "^v([a-f0-9]+)\\..*" "\\1" LIB_VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[a-f0-9]+\\.([a-f0-9]+).*" "\\1" LIB_VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[a-f0-9]+\\.[a-f0-9]+\\.([a-f0-9]+).*" "\\1" LIB_VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[a-f0-9]+\\.[a-f0-9]+\\.[a-f0-9]+.([a-f0-9]+).*" "\\1" LIB_VERSION_SHA1 "${VERSION}")
#set(VERSION "${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH}.${LIB_VERSION_SHA1}")
set(VERSION "${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH}.${LIB_VERSION_SHA1}")
#MESSAGE(VERSION)
MESSAGE( STATUS "VERSION:[" ${VERSION} "]" )

#set(LIB_VERSION_MAJOR "2")
#set(LIB_VERSION_MINOR "0")
#set(LIB_VERSION_PATCH "0")




macro(ADD_C_FLAGS flags)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flags}")
endmacro()

# feature checks
include(CheckLibraryExists)
check_library_exists(rt clock_gettime "time.h" CONFIG_SYSTEM_HAS_CLOCK_GETTIME)

# check if we are on a little or a big endian
include (TestBigEndian)
test_big_endian(PLATFORM_IS_BIGENDIAN)

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/dependencies/mbedtls-2.6.0)
set(WITH_MBEDTLS 1)
endif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/dependencies/mbedtls-2.6.0)

option(BUILD_EXAMPLES "Build the examples" ON)
option(BUILD_TESTS "Build the tests" ON)


include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/src/inc/api
    ${CMAKE_CURRENT_LIST_DIR}/src/inc/internal

   # ${CMAKE_CURRENT_LIST_DIR}/../../../../Qt_vs/Qt5.2.1/5.2.1/msvc2010/include/QtCore
   # ${CMAKE_CURRENT_LIST_DIR}/../../../../Qt_vs/Qt5.2.1/5.2.1/msvc2010/include/
    ${CMAKE_CURRENT_LIST_DIR}/src/common/inc
    ${CMAKE_CURRENT_LIST_DIR}/src/hal/inc
    ${CMAKE_CURRENT_LIST_DIR}/src/tls
)

if(WITH_MBEDTLS)
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/src/tls/mbedtls
    ${CMAKE_CURRENT_LIST_DIR}/dependencies/mbedtls-2.6.0/include
)

file(GLOB tls_SRCS ${CMAKE_CURRENT_LIST_DIR}/dependencies/mbedtls-2.6.0/library/*.c)

add_definitions(-DCONFIG_CS104_SUPPORT_TLS=1)
add_definitions(-DMBEDTLS_CONFIG_FILE="mbedtls_config.h")

endif(WITH_MBEDTLS)


set(API_HEADERS 
	${CMAKE_CURRENT_LIST_DIR}/src/hal/inc/hal_time.h 
	${CMAKE_CURRENT_LIST_DIR}/src/hal/inc/hal_thread.h
	${CMAKE_CURRENT_LIST_DIR}/src/hal/inc/hal_filesystem.h 
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/cs101_master.h
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/cs101_slave.h
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/cs104_slave.h
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/iec60870_master.h
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/iec60870_slave.h
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/iec60870_common.h
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/information_objects
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/cs104_connection.h
	${CMAKE_CURRENT_LIST_DIR}/src/inc/api/link_layer_parameters.h
	${CMAKE_CURRENT_LIST_DIR}/src/tls/tls_api.h
)


# write the detected stuff to this file
# configure_file(config/lib60870_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config/lib60870_config.h)

if(BUILD_EXAMPLES)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/examples)
endif(BUILD_EXAMPLES)

if(BUILD_TESTS)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tests)
endif(BUILD_TESTS)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src)

INSTALL(FILES ${API_HEADERS} DESTINATION include/lib60870 COMPONENT Development)

IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
INCLUDE(InstallRequiredSystemLibraries)
 
SET(CPACK_PACKAGE_DESCRIPTION "IEC 60870-5-101/104 master/slave library")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "IEC 60870-5-101/104 master/slave library")
SET(CPACK_PACKAGE_VENDOR "MZ Automation GmbH")
SET(CPACK_PACKAGE_CONTACT "info@mz-automation.de")
SET(CPACK_PACKAGE_VERSION_MAJOR "${LIB_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${LIB_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${LIB_VERSION_PATCH}")
SET(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}_${CMAKE_SYSTEM_PROCESSOR}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
 
SET(CPACK_COMPONENTS_ALL Libraries Development Applications)
#set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
INCLUDE(CPack)
 
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
