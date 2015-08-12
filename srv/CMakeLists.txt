qore_add_executable(srv
    main.cpp
    applink.c
    socket.cpp

    LIBS
    http2
)

target_link_libraries(srv ws2_32)
