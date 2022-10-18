project(chat_server C)

add_executable(server server.c)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)