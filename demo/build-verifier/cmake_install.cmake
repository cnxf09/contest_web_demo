# Install script for directory: /home/cat/longfellow-zk/lib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/cat/longfellow-zk/reference/verifier-service/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/cat/longfellow-zk/build-verifier/testing/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/util/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/algebra/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/arrays/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/merkle/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/ligero/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/proto/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/random/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/sumcheck/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/gf2k/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/cbor/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/ec/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/zk/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/cbor_parser/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/compiler/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/ecdsa/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/logic/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/mac/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/mdoc/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/sha/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/tests/anoncred/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/tests/sha3/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/tests/base64/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/tests/jwt/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/tests/ripemd/cmake_install.cmake")
  include("/home/cat/longfellow-zk/build-verifier/circuits/tests/mdoc/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/cat/longfellow-zk/build-verifier/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
