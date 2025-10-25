add_executable(latencyclient
    latencyclient.cpp
    latencyanalyzer.cpp
)
target_link_libraries(latencyclient
    PRIVATE
        QtWebSocketProtobuf
        Qt5::Core
        ExampleMessageProcessor
        ExampleMessageRouter
        ExampleMessage
)

add_executable(latencyserver
    latencyserver.cpp
)
target_link_libraries(latencyserver
    PRIVATE
        QtWebSocketProtobuf
        Qt5::Core
        ExampleMessageProcessor
        ExampleMessageRouter
        ExampleMessage
)

add_executable(latencythreadedserver
    latencythreadedserver.cpp
)
target_link_libraries(latencythreadedserver
    PRIVATE
        QtWebSocketProtobuf
        Qt5::Core
        ExampleMessageProcessor
        ExampleMessageRouter
        ExampleMessage
)

install(TARGETS latencyclient latencyserver latencythreadedserver
    RUNTIME DESTINATION bin/QtWebSocketProtobuf/examples
)

install(FILES 
    latencyanalyzer.h
    DESTINATION include/QtWebSocketProtobuf/examples/latencytest
)
