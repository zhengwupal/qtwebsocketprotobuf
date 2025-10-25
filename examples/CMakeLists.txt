set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/protos/example_message.proto
)

# 手动生成protobuf代码
set(PROTO_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/example_message.pb.cc
)
set(PROTO_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/example_message.pb.h
)

# 添加自定义命令来生成protobuf代码
add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND $<TARGET_FILE:protoc>
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/protos ${CMAKE_CURRENT_SOURCE_DIR}/protos/example_message.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protos/example_message.proto protoc
    COMMENT "Generating protobuf code for example_message.proto"
)

add_library(ExampleMessage STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(ExampleMessage
    PUBLIC 
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ExampleMessage PUBLIC libprotobuf)

# 禁用自动 MOC，因为 protobuf 生成的文件不需要 MOC
set_target_properties(ExampleMessage PROPERTIES AUTOMOC OFF)

add_library(ExampleMessageRouter
    examplemessagerouter.cpp
    examplemessagerouter.h
)
target_include_directories(ExampleMessageRouter
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(ExampleMessageRouter
    PRIVATE
        QtWebSocketProtobuf
        Qt5::Core
        ExampleMessage
)

add_library(ExampleMessageProcessor
    examplemessageprocessor.cpp
    examplemessageprocessor.h
)
target_include_directories(ExampleMessageProcessor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(ExampleMessageProcessor
    PRIVATE
        QtWebSocketProtobuf
        Qt5::Core
        ExampleMessage
        ExampleMessageRouter
)

add_library(TimeoutManager
    timeoutmanager.cpp
    timeoutmanager.h
)
target_include_directories(TimeoutManager
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(TimeoutManager
    PRIVATE
        Qt5::Core
)

install(TARGETS ExampleMessage ExampleMessageProcessor ExampleMessageRouter TimeoutManager
    LIBRARY DESTINATION lib/QtWebSocketProtobuf/examples
    ARCHIVE DESTINATION lib/QtWebSocketProtobuf/examples
    RUNTIME DESTINATION bin/QtWebSocketProtobuf/examples
)
install(FILES 
    examplemessageprocessor.h
    examplemessagerouter.h
    timeoutmanager.h
    messagevalidationutils.h
    messagetypeutils.h
    DESTINATION include/QtWebSocketProtobuf/examples
)

install(FILES ${PROTO_HDRS}
    DESTINATION include/QtWebSocketProtobuf/examples
)

add_subdirectory(basic)
add_subdirectory(latencytest)
