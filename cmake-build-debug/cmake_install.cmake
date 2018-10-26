# Install script for directory: /home/matteo/Dropbox/sis

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/matteo/Dropbox/sis/cmake-build-debug/blif2vst/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/vst2blif/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/espresso/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/jedi/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/nova/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/options/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/sis/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/sred/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/stamina/cmake_install.cmake")
  include("/home/matteo/Dropbox/sis/cmake-build-debug/utility/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/matteo/Dropbox/sis/cmake-build-debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
