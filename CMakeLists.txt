#  Copyright (c) 2017, Niklas Gürtler
# 
#  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
#  following conditions are met:
# 
#  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
#     disclaimer.
# 
#  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
#     following disclaimer in the documentation and/or other materials provided with the distribution.
# 
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 
cmake_minimum_required(VERSION 3.5)


project(usbclient)

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG				"/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1 /MP /W4")
    set(CMAKE_CXX_FLAGS_MINSIZEREL			"/MT /O1 /Ob1 /D DNDEBUG /MP /W4")
    set(CMAKE_CXX_FLAGS_RELEASE				"/MT /O2 /Ob2 /D DNDEBUG /MP /W4")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO		"/MT /Zi /O2 /Ob1 /D DNDEBUG /MP /W4")
endif()
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS						"${CMAKE_CXX_FLAGS} -Wall -Wextra -ffunction-sections -fdata-sections")
	set(CMAKE_EXE_LINKER_FLAGS				"${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -pthread")

	set(CMAKE_CXX_FLAGS_MINSIZEREL			"${CMAKE_CXX_FLAGS_MINSIZEREL} -flto")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL	"${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -s -flto")

	set(CMAKE_CXX_FLAGS_RELEASE				"${CMAKE_CXX_FLAGS_RELEASE} -flto")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE		"${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
endif()

set (USE_PKG_CONFIG "false")

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBUSB libusb-1.0>=1.0.20)
	if(LIBUSB_FOUND)
		set (USE_PKG_CONFIG "true")
	endif()
endif()

if(NOT USE_PKG_CONFIG)
	if(MSVC)
		if (${CMAKE_SIZEOF_VOID_P} EQUAL "8")
		    set(CMAKE_EXE_LINKER_FLAGS_DEBUG				"${CMAKE_EXE_LINKER_FLAGS_DEBUG} /LIBPATH:libusb-msvc\\MS64\\dll")
		    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL			"${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /LIBPATH:libusb-msvc\\MS64\\dll")
		    set(CMAKE_EXE_LINKER_FLAGS_RELEASE				"${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /LIBPATH:libusb-msvc\\MS64\\static")
		    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO		"${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /LIBPATH:libusb-msvc\\MS64\\static")
			
			set(CMAKE_MSVCIDE_RUN_PATH						"libusb-msvc\\MS64\\dll")
		else()
		    set(CMAKE_EXE_LINKER_FLAGS_DEBUG				"${CMAKE_EXE_LINKER_FLAGS_DEBUG} /LIBPATH:libusb-msvc\\MS32\\dll")
		    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL			"${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /LIBPATH:libusb-msvc\\MS32\\dll")
		    set(CMAKE_EXE_LINKER_FLAGS_RELEASE				"${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /LIBPATH:libusb-msvc\\MS32\\static")
		    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO		"${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /LIBPATH:libusb-msvc\\MS32\\static")

			set(CMAKE_MSVCIDE_RUN_PATH						"libusb-msvc\\MS32\\dll")
		endif()
	else()
		message(STATUS "LIBUSB not found!")
	endif()
endif()

add_executable(usbclient src/main.cc) 
set_property(TARGET usbclient PROPERTY CXX_STANDARD 11)

if(USE_PKG_CONFIG)
	include_directories(${LIBUSB_INCLUDE_DIRS})
	target_link_libraries(usbclient ${LIBUSB_LDFLAGS})
	target_include_directories(usbclient PUBLIC ${usbclient_INCLUDE_DIRS})
	target_compile_options(usbclient PUBLIC ${usbclient_CFLAGS_OTHER})
else()
	include_directories("libusb-msvc\\include\\libusb-1.0")
	target_link_libraries(usbclient "libusb-1.0.lib")
endif()
