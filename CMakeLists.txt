cmake_minimum_required(VERSION 3.20)

project(dosbox-virtisa VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckIncludeFiles)

find_package(SDL REQUIRED)
find_package(OpenGL)
find_package(PNG)
find_package(PkgConfig)

if(PkgConfig_FOUND)
  pkg_check_modules(SDL_NET QUIET SDL_net)
  pkg_check_modules(ALSA QUIET alsa)
endif()

check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("pwd.h" HAVE_PWD_H)
check_include_file("ddraw.h" HAVE_DDRAW_H)

check_function_exists(realpath HAVE_REALPATH)
check_function_exists(mprotect C_HAVE_MPROTECT)

check_type_size("unsigned char" SIZEOF_UNSIGNED_CHAR)
check_type_size("unsigned short" SIZEOF_UNSIGNED_SHORT)
check_type_size("unsigned int" SIZEOF_UNSIGNED_INT)
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)
check_type_size("void *" SIZEOF_INT_P)

set(STDC_HEADERS 1)
set(ENVIRON_INCLUDED 1)
set(ENVIRON_LINKED 1)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(MACOSX 1)
elseif(CMAKE_SYSTEM_NAME MATCHES ".*BSD")
  set(BSD 1)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
  set(C_TARGETCPU X86_64)
  set(C_UNALIGNED_MEMORY 1)
  set(C_DYNREC 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
  set(C_TARGETCPU X86)
  set(C_UNALIGNED_MEMORY 1)
  set(C_DYNAMIC_X86 1)
  set(C_FPU_X86 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^powerpc")
  set(C_TARGETCPU POWERPC)
  set(C_UNALIGNED_MEMORY 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^m68k")
  set(C_TARGETCPU M68K)
  set(C_UNALIGNED_MEMORY 1)
else()
  set(C_TARGETCPU UNKNOWN)
endif()

set(C_FPU 1)
set(C_DIRECTSERIAL 1)
set(C_HAS_ATTRIBUTE 1)
set(C_HAS_BUILTIN_EXPECT 1)
set(C_ATTRIBUTE_ALWAYS_INLINE 1)
set(C_ATTRIBUTE_FASTCALL 1)

if(PNG_FOUND)
  set(C_SSHOT 1)
endif()

if(SDL_NET_FOUND)
  set(C_MODEM 1)
  set(C_IPX 1)
endif()

if(OpenGL_FOUND)
  set(C_OPENGL 1)
endif()

if(ALSA_FOUND)
  set(HAVE_ALSA 1)
endif()

set(CMAKE_REQUIRED_LIBRARIES)
check_c_source_compiles("#include <dirent.h>\nint main(){struct dirent d; d.d_type = 0; return 0;}" DIRENT_HAS_D_TYPE)
check_c_source_compiles("#include <sys/resource.h>\nint main(){return setpriority(PRIO_PROCESS, 0, PRIO_MIN + PRIO_MAX);}" C_SET_PRIORITY)

set(PACKAGE "dosbox-virtisa")
set(PACKAGE_NAME "DOSBox-VirtIsa")
set(PACKAGE_TARNAME "dosbox-virtisa")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "DOSBox-VirtIsa ${PROJECT_VERSION}")
set(PACKAGE_BUGREPORT "")
set(PACKAGE_URL "")
set(VERSION "${PROJECT_VERSION}")

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

file(GLOB DOSBOX_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
  src/cpu/*.cpp
  src/debug/*.cpp
  src/dos/*.cpp
  src/fpu/*.cpp
  src/gui/*.cpp
  src/hardware/*.cpp
  src/hardware/mame/*.cpp
  src/hardware/serialport/*.cpp
  src/ints/*.cpp
  src/misc/*.cpp
  src/shell/*.cpp
  src/libs/gui_tk/*.cpp
)

add_executable(dosbox ${DOSBOX_SOURCES})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(dosbox PRIVATE
    -Wno-format
    -Wno-address-of-packed-member
  )
endif()

target_include_directories(dosbox PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SDL_INCLUDE_DIR}
  ${SDL_NET_INCLUDE_DIRS}
  ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(dosbox PRIVATE ${SDL_LIBRARY})

if(SDL_NET_FOUND)
  target_link_libraries(dosbox PRIVATE ${SDL_NET_LIBRARIES})
endif()
if(PNG_FOUND)
  target_link_libraries(dosbox PRIVATE PNG::PNG)
endif()
if(OpenGL_FOUND)
  target_link_libraries(dosbox PRIVATE OpenGL::GL)
endif()
if(ALSA_FOUND)
  target_link_libraries(dosbox PRIVATE ${ALSA_LIBRARIES})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(dosbox PRIVATE m pthread)
endif()

message(STATUS "Configured DOSBox-VirtIsa ${PROJECT_VERSION} with CMake")
