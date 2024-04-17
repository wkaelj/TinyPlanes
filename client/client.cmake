
file(GLOB SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.c ${CMAKE_CURRENT_LIST_DIR}/render/render.c)

set(CLIENT_NAME ${PROJECT_NAME}_client)

add_executable(${CLIENT_NAME} ${SOURCES})

target_link_libraries(${CLIENT_NAME} PRIVATE
  SDL2
  SDL2_image
  SDL2_ttf
  ${SHARED_NAME}
  ${LIBS_WRAPPER_NAME}
  m # standard math library
)
