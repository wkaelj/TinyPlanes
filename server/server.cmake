set(SERVER_NAME ${PROJECT_NAME}_server)

add_executable(${SERVER_NAME} ${CMAKE_CURRENT_LIST_DIR}/server.c)

target_link_libraries(${SERVER_NAME} PRIVATE ${SHARED_NAME} cutils)
