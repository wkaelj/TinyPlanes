
set(LIBS_WRAPPER_NAME ${PROJECT_NAME}_libs_wrap)

# cmake options
option(CGLM_SHARED "Shared build" OFF)
option(CGLM_STATIC "Static build" OFF)
option(CGLM_USE_TEST "Enable Tests" OFF) # for make check - make test



add_library(${LIBS_WRAPPER_NAME} STATIC
  $<TARGET_OBJECTS:noise1234>
  $<TARGET_OBJECTS:cutils>
)

target_link_libraries(${LIBS_WRAPPER_NAME} PUBLIC
  noise1234
)

#include cglm headers
target_include_directories(${LIBS_WRAPPER_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/cglm/include/
    ${CMAKE_CURRENT_LIST_DIR}/cutils/
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/noise1234/)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/cutils/)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/cglm/ EXCLUDE_FROM_ALL)

