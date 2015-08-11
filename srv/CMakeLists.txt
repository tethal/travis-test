qore_add_executable(srv
    main.cpp
    applink.c
    ssl.cpp

    LIBS
    http2
)

target_link_libraries(srv ws2_32)
