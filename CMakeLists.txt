cmake_minimum_required(VERSION 3.20)

project(dosbox VERSION 0.74.3 LANGUAGES C CXX)

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

set(PACKAGE "dosbox")
set(PACKAGE_NAME "dosbox")
set(PACKAGE_TARNAME "dosbox")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "dosbox ${PROJECT_VERSION}")
set(PACKAGE_BUGREPORT "")
set(PACKAGE_URL "")
set(VERSION "${PROJECT_VERSION}")

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

set(DOSBOX_SOURCES
  src/dosbox.cpp
  src/cpu/callback.cpp src/cpu/cpu.cpp src/cpu/flags.cpp src/cpu/modrm.cpp src/cpu/core_full.cpp
  src/cpu/paging.cpp src/cpu/core_normal.cpp src/cpu/core_simple.cpp src/cpu/core_prefetch.cpp
  src/cpu/core_dyn_x86.cpp src/cpu/core_dynrec.cpp
  src/debug/debug.cpp src/debug/debug_gui.cpp src/debug/debug_disasm.cpp src/debug/debug_win32.cpp
  src/dos/dos.cpp src/dos/dos_devices.cpp src/dos/dos_execute.cpp src/dos/dos_files.cpp
  src/dos/dos_ioctl.cpp src/dos/dos_memory.cpp src/dos/dos_misc.cpp src/dos/dos_classes.cpp
  src/dos/dos_programs.cpp src/dos/dos_tables.cpp src/dos/drives.cpp src/dos/drive_virtual.cpp
  src/dos/drive_local.cpp src/dos/drive_cache.cpp src/dos/drive_fat.cpp src/dos/drive_iso.cpp
  src/dos/dos_mscdex.cpp src/dos/dos_keyboard_layout.cpp src/dos/cdrom.cpp src/dos/cdrom_ioctl_win32.cpp
  src/dos/cdrom_aspi_win32.cpp src/dos/cdrom_ioctl_linux.cpp src/dos/cdrom_image.cpp src/dos/cdrom_ioctl_os2.cpp
  src/fpu/fpu.cpp
  src/gui/sdlmain.cpp src/gui/sdl_mapper.cpp src/gui/render.cpp src/gui/render_scalers.cpp src/gui/midi.cpp src/gui/sdl_gui.cpp
  src/hardware/adlib.cpp src/hardware/dma.cpp src/hardware/gameblaster.cpp src/hardware/hardware.cpp
  src/hardware/iohandler.cpp src/hardware/joystick.cpp src/hardware/keyboard.cpp src/hardware/memory.cpp
  src/hardware/mixer.cpp src/hardware/pcspeaker.cpp src/hardware/pic.cpp src/hardware/sblaster.cpp
  src/hardware/tandy_sound.cpp src/hardware/timer.cpp src/hardware/vga.cpp src/hardware/vga_attr.cpp
  src/hardware/vga_crtc.cpp src/hardware/vga_dac.cpp src/hardware/vga_draw.cpp src/hardware/vga_gfx.cpp
  src/hardware/vga_other.cpp src/hardware/vga_memory.cpp src/hardware/vga_misc.cpp src/hardware/vga_seq.cpp
  src/hardware/vga_xga.cpp src/hardware/vga_s3.cpp src/hardware/vga_tseng.cpp src/hardware/vga_paradise.cpp
  src/hardware/cmos.cpp src/hardware/disney.cpp src/hardware/gus.cpp src/hardware/mpu401.cpp
  src/hardware/ipx.cpp src/hardware/ipxserver.cpp src/hardware/dbopl.cpp
  src/hardware/mame/saa1099.cpp src/hardware/mame/sn76496.cpp
  src/hardware/serialport/directserial.cpp src/hardware/serialport/libserial.cpp
  src/hardware/serialport/serialdummy.cpp src/hardware/serialport/serialport.cpp
  src/hardware/serialport/softmodem.cpp src/hardware/serialport/misc_util.cpp src/hardware/serialport/nullmodem.cpp
  src/ints/mouse.cpp src/ints/xms.cpp src/ints/ems.cpp src/ints/int10.cpp src/ints/int10_char.cpp
  src/ints/int10_memory.cpp src/ints/int10_misc.cpp src/ints/int10_modes.cpp src/ints/int10_vesa.cpp
  src/ints/int10_pal.cpp src/ints/int10_put_pixel.cpp src/ints/int10_video_state.cpp src/ints/int10_vptable.cpp
  src/ints/bios.cpp src/ints/bios_disk.cpp src/ints/bios_keyboard.cpp
  src/misc/cross.cpp src/misc/messages.cpp src/misc/programs.cpp src/misc/setup.cpp src/misc/support.cpp
  src/shell/shell.cpp src/shell/shell_batch.cpp src/shell/shell_cmds.cpp src/shell/shell_misc.cpp
  src/libs/gui_tk/gui_tk.cpp
)

add_executable(dosbox ${DOSBOX_SOURCES})

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

message(STATUS "Configured DOSBox ${PROJECT_VERSION} with CMake")
