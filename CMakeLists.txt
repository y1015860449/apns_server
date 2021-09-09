cmake_minimum_required(VERSION 2.6)
PROJECT(apnsserver)

AUX_SOURCE_DIRECTORY(./ SRC_LIST)
AUX_SOURCE_DIRECTORY(./jsoncpp SRC_LIST2)

#SET(LIBRARY_OUTPUT_PATH ./)
SET(EXECUTABLE_OUTPUT_PATH  ../bin)
if(APPLE)
    SET(PB_LIB_DIR ../base/pb/lib/mac)
    SET(SECURITY_LIB_DIR ../base/security/lib/mac)
elseif(UNIX)
    SET(PB_LIB_DIR ../base/pb/lib/linux)
    SET(SECURITY_LIB_DIR ../base/security/lib/linux)
endif()


ADD_DEFINITIONS( -g -W -Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DAC_HAS_INFO
-DAC_HAS_WARNING -DAC_HAS_ERROR -DAC_HAS_CRITICAL -DTIXML_USE_STL
-DAC_HAS_DEBUG -DLINUX_DAEMON -std=c++11)

INCLUDE_DIRECTORIES(./  ./jsoncpp ./slog)
LINK_DIRECTORIES(./ ../bin ${PB_LIB_DIR} ../lib ./lib ${SECURITY_LIB_DIR})

#ADD_XXXX必须在TARGET_LINK_LIBRARIES前面，否则会报错
#ADD_LIBRARY(${PROJECTNAME} SHARED/STATIC ${SRC_LIST})
ADD_EXECUTABLE(apns_server ${SRC_LIST} ${SRC_LIST2})

TARGET_LINK_LIBRARIES(apns_server uuid slog nghttp2 crypto ssl pthread )
