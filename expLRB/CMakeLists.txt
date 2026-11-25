cmake_minimum_required(VERSION 3.16...3.27)
project(expLRB)

option(WITH_GEANT4_UIVIS "Build with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include ${Geant4_INCLUDE_DIR})
file(GLOB sources CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(expLRB main.cc ${sources} ${headers})
target_link_libraries(expLRB ${Geant4_LIBRARIES})

if(WITH_GEANT4_UIVIS)
  target_compile_definitions(expLRB PRIVATE EXP_LRB_WITH_UIVIS)
endif()

# Копируем визуализационные макросы в build/, чтобы запускать бинарник из build/
set(expLRB_SCRIPTS
    animate_flight.mac animate_rotate.mac rotate_frame.mac time_frame.mac vis.mac)

foreach(_script ${expLRB_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()

install(TARGETS expLRB DESTINATION bin)
