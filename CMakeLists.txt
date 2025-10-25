cmake_minimum_required(VERSION 3.22)
project(QtWebSocketProtobuf VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core Network WebSockets REQUIRED)

# 引入第三方库 protobuf
set(protobuf_BUILD_TESTS OFF CACHE BOOL "Disable protobuf tests")
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "Disable protobuf examples")
set(protobuf_INSTALL OFF CACHE BOOL "Disable protobuf install")
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "Build protoc binaries")
set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "Build libprotoc")
set(protobuf_BUILD_LIBPROTOBUF ON CACHE BOOL "Build libprotobuf")
set(protobuf_BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries")
add_subdirectory(src/third_party/protobuf/cmake)

set(PUBLIC_HEADERS
    src/qtwebsocketprotobufglobal.h
    src/core/websocketclient.h
    src/core/websocketserver.h
    src/core/threadedwebsocketserver.h
    src/core/abstractmessageprocessor.h
    src/core/abstractmessagerouter.h
    src/core/message.h
    src/core/protobufmessage.h
    src/core/messageserializer.h
    src/core/packetserializer.h
    src/utils/logger.h
    src/core/sendresult.h
    src/core/broadcastresult.h
)

set(SOURCES
    src/core/websocketclient.cpp
    src/core/websocketserver.cpp
    src/core/threadedwebsocketserver.cpp
    src/core/sessionmanager.cpp
    src/core/abstractmessagerouter.cpp
    src/core/abstractmessageprocessor.cpp
    src/core/sendresult.cpp
    src/core/broadcastresult.cpp
    src/core/packetheader.cpp
    src/core/snowflakeid.cpp
    src/core/packetserializer.cpp
    src/core/messageserializer.cpp
    src/utils/logger.cpp
    src/core/message.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${PUBLIC_HEADERS})
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME QtWebSocketProtobuf
)
target_compile_definitions(${PROJECT_NAME} 
    PRIVATE QTWEBSOCKETPROTOBUF_LIBRARY
)
target_include_directories(${PROJECT_NAME} 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME} 
    PUBLIC 
        Qt5::Core 
        Qt5::Network 
        Qt5::WebSockets
        libprotobuf
)

# 生成转发头文件
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/QtWebSocketProtobuf)

foreach(header ${PUBLIC_HEADERS})
    get_filename_component(header_name ${header} NAME)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/include/QtWebSocketProtobuf/${header_name} 
        "#include \"../../${header}\"\n")
endforeach()

set(MAIN_HEADER_CONTENT "#ifndef QTWEBSOCKETPROTOBUF_H\n#define QTWEBSOCKETPROTOBUF_H\n\n")
foreach(header ${PUBLIC_HEADERS})
    get_filename_component(header_name ${header} NAME)
    string(APPEND MAIN_HEADER_CONTENT "#include \"${header_name}\"\n")
endforeach()
string(APPEND MAIN_HEADER_CONTENT "\n#endif\n")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/include/QtWebSocketProtobuf/QtWebSocketProtobuf "${MAIN_HEADER_CONTENT}")

# 生成Qt风格的大驼峰别名头文件
set(PASCAL_CASE_ALIASES
    "WebSocketClient:websocketclient.h"
    "WebSocketServer:websocketserver.h"
    "ThreadedWebSocketServer:threadedwebsocketserver.h"
    "AbstractMessageProcessor:abstractmessageprocessor.h"
    "AbstractMessageRouter:abstractmessagerouter.h"
    "Message:message.h"
    "ProtobufMessage:protobufmessage.h"
    "MessageSerializer:messageserializer.h"
    "PacketSerializer:packetserializer.h"
    "Logger:logger.h"
    "SendResult:sendresult.h"
    "BroadcastResult:broadcastresult.h"
)
foreach(alias ${PASCAL_CASE_ALIASES})
    string(REPLACE ":" ";" alias_parts ${alias})
    list(GET alias_parts 0 pascal_name)
    list(GET alias_parts 1 header_name)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/include/QtWebSocketProtobuf/${pascal_name} 
        "#include \"${header_name}\"\n")
endforeach()

option(BUILD_EXAMPLES "Build example applications" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QtWebSocketProtobuf
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/QtWebSocketProtobuf/QtWebSocketProtobuf
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QtWebSocketProtobuf
)

# 安装Qt风格的大驼峰别名头文件
foreach(alias ${PASCAL_CASE_ALIASES})
    string(REPLACE ":" ";" alias_parts ${alias})
    list(GET alias_parts 0 pascal_name)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/QtWebSocketProtobuf/${pascal_name}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QtWebSocketProtobuf
    )
endforeach()

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
) 
