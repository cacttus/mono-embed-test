
cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Utils
function(GET_ARCH_CONFIG)
  set(ARCH_CONFIG "_")  

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SIZE Lib)
    string(APPEND ARCH_CONFIG x86)
  else()
    string(APPEND ARCH_CONFIG x64) 
  endif()
   
  # Target
  if(GENERATOR_IS_MULTI_CONFIG)
    message("Error: Generator is MULTI CONFIG")
    return()
    # "Multi-config generators use CMAKE_CONFIGURATION_TYPES as the set of configurations and ignore CMAKE_BUILD_TYPE."
  else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(ARCH_CONFIG ${ARCH_CONFIG}d PARENT_SCOPE)
    else ()
      set(ARCH_CONFIG ${ARCH_CONFIG}r PARENT_SCOPE)
    endif()
  endif() 
endfunction()  
  
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
message("CMAKE_MODULE_PATH='" ${CMAKE_MODULE_PATH} "' CMAKE_PREFIX_PATH='" ${CMAKE_PREFIX_PATH} "' CMAKE_TOOLCHAIN_PATH='" ${CMAKE_TOOLCHAIN_PATH} "'")
    
# ** Global Directories **  
set(VG_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/)
set(VG_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/)
set(VG_BIN_DEBUG_DIR ${VG_BIN_DIR})
set(VG_BIN_RELEASE_DIR ${VG_BIN_DIR})


# VG 
set(VG_PROJECT_VERSION 0.0.1)
set(VG_TARGET_NAME mono-embed-test)
project(${VG_TARGET_NAME} VERSION ${VG_PROJECT_VERSION} LANGUAGES CXX C)

if(WIN32) 
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/ext/") 
message("Set Module Path to " ${CMAKE_MODULE_PATH} "Win32")
endif() 

# VG
add_executable(${VG_TARGET_NAME} 
${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp 
)
  
get_arch_config()  
 
set(VG_OUTPUT_NAME ${VG_TARGET_NAME}-${PROJECT_VERSION}${ARCH_CONFIG})
message("Output: " ${VG_OUTPUT_NAME})
 
set_target_properties(${PROJECT_NAME} PROPERTIES 
  OUTPUT_NAME ${VG_OUTPUT_NAME}
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${VG_BIN_DEBUG_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${VG_BIN_RELEASE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${VG_BIN_DEBUG_DIR}
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${VG_BIN_DEBUG_DIR}
  ARCHIVE_OUTPUT_DIRECTORY ${VG_LIB_DIR}
)
  
# Packages
find_package(Threads REQUIRED) #This is better than -pthread

#set(OpenGL_GL_PREFERENCE LEGACY) #GLVND or LEGACY
#find_package(OpenGL REQUIRED)
#find_package(SDL2 REQUIRED)
#find_package(Vulkan REQUIRED)  
#find_package(mono REQUIRED)

find_package(PkgConfig REQUIRED)

#MONO 
#this should include "/usr/include/mono-2.0/mono/jit"
# This is after installing mono-devel or mono-complete 
set(MONO_INCLUDE_DIRS "/usr/include/mono-2.0/")
set(MONO_LIBRARIES "/usr/lib/libmono-2.0.so")
#set(MONO_LIBRARIES "/usr/lib/libmonosgen-2.0.so")

if(UNIX)
find_package(X11 REQUIRED)
endif()


# Includes
target_include_directories(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS} ${SDLNET_INCLUDE_DIR} 
${VULKAN_INCLUDE_DIRS} ${LUA_INCLUDE_DIR} ${LUAINTF_INCLUDE_DIR} ${MONO_INCLUDE_DIRS})
 
# Libraries
set(ADDL_LIBS "")
if(WIN32)
  set(VG_WIN32_COMPILE_OPTS "")

  #Unicode required for building with Unicode Character Set (vs MBCS)
  string(APPEND VG_WIN32_COMPILE_OPTS " -DUNICODE")
  string(APPEND VG_WIN32_COMPILE_OPTS " -D_UNICODE")

  #Suppress error
  string(APPEND VG_WIN32_COMPILE_OPTS " -D_CRT_NO_VA_START_VALIDATION")

  #Multicore build
  string(APPEND VG_WIN32_COMPILE_OPTS /MP) #NOte: originally CMAKE_CXX_FLAGS

  target_compile_options(${VG_TARGET_NAME} PRIVATE "$<$<CONFIG:RELEASE>:" ${VG_WIN32_COMPILE_OPTS})
  target_compile_options(${VG_TARGET_NAME} PRIVATE "$<$<CONFIG:DEBUG>:" ${VG_WIN32_COMPILE_OPTS} -D_DEBUG -D_GLIBCXX_DEBUG)
 
  find_package(SDL2_net REQUIRED)
  set(VG_ADDL_LIBS "ws2_32.lib" "winmm.lib" "imm32.lib" "version.lib" "dbghelp.lib" "setupapi.lib" "iphlpapi.lib")
elseif(CMAKE_COMPILER_IS_GNUCXX OR MINGW OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")

  # --rdynamic is required to keep the unused c++ symbols from being optimized out so the mono runtime can find and use them. 
  set(VG_GCC_LINK_GLOBAL -pthread -rdynamic ) 
  # "`pkg-config --cflags --libs mono-2`"
  #  expands to .. 
  # -D_REENTRANT -I/usr/lib/pkgconfig/../../include/mono-2.0 -L/usr/lib/pkgconfig/../../lib -lmono-2.0 -lm -lrt -ldl -lpthread
 
  set(VG_MONO_COMP_FLAGS -D_REENTRANT -lm -lrt -ldl )

  message("Mono Flags: " ${VG_MONO_FLAGS})

  target_compile_options(${VG_TARGET_NAME} PRIVATE "$<$<CONFIG:DEBUG>:" -D_DEBUG -D_GLIBCXX_DEBUG ${VG_MONO_COMP_FLAGS} ">")
  target_compile_options(${VG_TARGET_NAME} PRIVATE "$<$<CONFIG:RELEASE>:" -Ofast ${VG_MONO_COMP_FLAGS} ">") 

  target_link_options(${VG_TARGET_NAME} PRIVATE "$<$<CONFIG:RELEASE>:" ${VG_GCC_LINK_GLOBAL} ">")
  target_link_options(${VG_TARGET_NAME} PRIVATE "$<$<CONFIG:DEBUG>:" ${VG_GCC_LINK_GLOBAL} ">")
   
  set(VULKAN_LIBRARIES vulkan)
  set(SDLNET_LIBRARIES SDL2_net) 
  set(VG_ADDL_LIBS ncurses)
endif() 


target_link_directories(${PROJECT_NAME} PRIVATE ${VG_LIB_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${VULKAN_LIBRARIES} ${OPENGL_LIBRARIES} 
${SDL2_LIBRARIES} ${SDLNET_LIBRARIES} ${VG_ADDL_LIBS} ${X11_LIBRARIES} ${LUA_LIBRARIES} ${MONO_LIBRARIES}

#${CMAKE_CURRENT_SOURCE_DIR}/../VulkanGame/lib/libVulkanGame-0.4.1_x86d.a
)
