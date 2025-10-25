add_executable(client
    client.cpp
)
target_link_libraries(client
    PRIVATE
    QtWebSocketProtobuf
    Qt5::Core
    ExampleMessageProcessor
    ExampleMessageRouter
    ExampleMessage
    TimeoutManager
)

add_executable(server
    server.cpp
)
target_link_libraries(server
    PRIVATE
    QtWebSocketProtobuf
    Qt5::Core
    ExampleMessageProcessor
    ExampleMessageRouter
    ExampleMessage
)

add_executable(threadedserver
    threadedserver.cpp
    ${COMMON_SOURCES}
)
target_include_directories(threadedserver 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_BINARY_DIR}/examples
)
target_link_libraries(threadedserver
    PRIVATE
        QtWebSocketProtobuf
        Qt5::Core
        ExampleMessageProcessor
        ExampleMessageRouter
        ExampleMessage
)

install(TARGETS client server threadedserver
    RUNTIME DESTINATION bin/QtWebSocketProtobuf/examples
)
