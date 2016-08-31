# CMake Version 
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

# Project Name
PROJECT(TFTPClient)

#
# Configuration
#
# SET(CMAKE_VERBOSE_MAKEFILE ON)
#

#
if ("${build}" MATCHES "debug")
 SET (CMAKE_BUILD_TYPE "debug")
else ("${build}" MATCHES "debug")
 SET (CMAKE_BUILD_TYPE "release")
endif ("${build}" MATCHES "debug")

#
# Add Compiler Option and Macro
#
ADD_DEFINITIONS(-DCALIB_CFG_BUILD_MODE="${CMAKE_BUILD_TYPE}")

# Set Build Directories
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(
    /usr/local/include/
)

link_directories(
    /usr/local/lib/
)

#
# Check OS and Processor Information 
#
SET(CMAKE_CXX_FLAGS "-std=c++11")
ADD_DEFINITIONS(-Wall -DCALIB_CFG_OS="${CMAKE_SYSTEM_NAME}" -DCALIB_CFG_CPU="${CMAKE_SYSTEM_PROCESSOR}")

# Configuration

add_executable (TFTPClient
    src/tftp_client.cpp
    src/TFTP.ASIO/AsioTFTPClient.cpp
    src/TFTP.ASIO/AsioTFTPClient.h
    src/TFTP.ASIO/tftp_impl/TFTPClientTransaction.cpp
    src/TFTP.ASIO/tftp_impl/TFTPClientTransaction.h    
    src/TFTP.ASIO/tftp_impl/TFTPTransaction.cpp
    src/TFTP.ASIO/tftp_impl/TFTPTransaction.h
    src/TFTP.ASIO/tftp_impl/TFTPFile.cpp
    src/TFTP.ASIO/tftp_impl/TFTPFile.h
    src/TFTP.ASIO/tftp_impl/TFTPPacket.h    
    src/TFTP.ASIO/tftp_impl/TFTPTransport.h
)

add_executable (TFTPServer
    src/tftp_server.cpp
    src/TFTP.ASIO/AsioTFTPServer.cpp
    src/TFTP.ASIO/AsioTFTPServer.h
    src/TFTP.ASIO/tftp_impl/TFTPServerTransaction.cpp
    src/TFTP.ASIO/tftp_impl/TFTPServerTransaction.h
    src/TFTP.ASIO/tftp_impl/TFTPTransaction.cpp
    src/TFTP.ASIO/tftp_impl/TFTPTransaction.h
    src/TFTP.ASIO/tftp_impl/TFTPFile.cpp
    src/TFTP.ASIO/tftp_impl/TFTPFile.h
    src/TFTP.ASIO/tftp_impl/TFTPPacket.h    
    src/TFTP.ASIO/tftp_impl/TFTPTransport.h
)

TARGET_LINK_LIBRARIES(TFTPClient
    boost_chrono
    boost_date_time
    boost_regex
    boost_system
    boost_thread
    pthread
)

TARGET_LINK_LIBRARIES(TFTPServer
    boost_chrono
    boost_date_time
    boost_regex
    boost_system
    boost_thread
    pthread
)
