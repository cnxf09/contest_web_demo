# Install script for directory: /Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/lib

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

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/testing/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/util/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/algebra/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/arrays/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/merkle/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/ligero/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/proto/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/random/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/sumcheck/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/gf2k/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/cbor/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/ec/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/zk/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/cbor_parser/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/compiler/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/ecdsa/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/logic/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/mac/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/mdoc/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/sha/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/examples/anoncred/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/examples/mdoc_anoncred/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/examples/delegation_demo/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/tests/anoncred/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/tests/sha3/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/tests/base64/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/tests/jwt/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/tests/ripemd/cmake_install.cmake")
  include("/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/circuits/tests/mdoc/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/cnxf/信安赛/代理签名/translated_atrps/code/longfellow-zk/build-delegation/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
