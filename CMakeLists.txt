cmake_minimum_required(VERSION 3.16)
project(ReFlood C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
option(FLOOD_WITH_SDL2 "Build SDL2 host" ON)
add_library(flood_core src/game.c src/assets.c src/ending.c src/audio.c src/presentation.c src/editor_browser.c src/custom_map_sequence.c)
target_include_directories(flood_core PUBLIC include)
target_compile_options(flood_core PRIVATE -Wall -Wextra -Wpedantic)
add_executable(flood_tests tests/test_core.c)
target_link_libraries(flood_tests PRIVATE flood_core)
target_compile_definitions(flood_tests PRIVATE FLOOD_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
enable_testing()
add_test(NAME core COMMAND flood_tests)
add_executable(flood_multiplayer_tests tests/test_multiplayer.c)
target_link_libraries(flood_multiplayer_tests PRIVATE flood_core)
target_compile_definitions(flood_multiplayer_tests PRIVATE FLOOD_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(NAME multiplayer COMMAND flood_multiplayer_tests)
add_executable(flood_sdl_input_tests tests/test_sdl_input.c)
target_link_libraries(flood_sdl_input_tests PRIVATE flood_core)
target_compile_definitions(flood_sdl_input_tests PRIVATE FLOOD_USE_SDL=1
  FLOOD_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
  REFLOOD_CONFIG_DIR="${CMAKE_CURRENT_BINARY_DIR}/test-config")
target_include_directories(flood_sdl_input_tests PRIVATE tests/sdl_stub)
add_test(NAME sdl_input COMMAND flood_sdl_input_tests)
add_library(reflood_editor_level src/editor_level.c src/editor_browser.c src/editor_tile_help.c)
target_include_directories(reflood_editor_level PUBLIC include)
target_compile_options(reflood_editor_level PRIVATE -Wall -Wextra -Wpedantic)
add_executable(reflood_editor_tests tests/test_editor.c)
target_link_libraries(reflood_editor_tests PRIVATE reflood_editor_level)
add_test(NAME editor COMMAND reflood_editor_tests)
add_executable(reflood src/main.c)
target_link_libraries(reflood PRIVATE flood_core)
set_target_properties(reflood PROPERTIES
  OUTPUT_NAME reflood
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/reflood")
add_custom_command(TARGET reflood POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          "$<TARGET_FILE_DIR:reflood>/tools"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/extract_flood_data.py"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/flood_data_manifest.json"
          "$<TARGET_FILE_DIR:reflood>/tools"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/data"
          "$<TARGET_FILE_DIR:reflood>/data"
  COMMENT "Copying data-extraction tools and data directory")
install(TARGETS reflood RUNTIME DESTINATION reflood)
install(FILES
  tools/extract_flood_data.py
  tools/flood_data_manifest.json
  DESTINATION reflood/tools)
install(DIRECTORY data DESTINATION reflood)
if(FLOOD_WITH_SDL2)
  find_package(SDL2 QUIET)
  if(SDL2_FOUND)
    target_compile_definitions(reflood PRIVATE FLOOD_USE_SDL=1)
    target_link_libraries(reflood PRIVATE SDL2::SDL2)
    add_executable(reflood-editor src/editor_main.c)
    target_link_libraries(reflood-editor PRIVATE reflood_editor_level SDL2::SDL2)
    target_compile_options(reflood-editor PRIVATE -Wall -Wextra -Wpedantic)
    set_target_properties(reflood-editor PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/reflood")
    install(TARGETS reflood-editor RUNTIME DESTINATION reflood)
    add_executable(reflood_editor_ui_tests tests/test_editor_ui.c)
    target_link_libraries(reflood_editor_ui_tests PRIVATE reflood_editor_level SDL2::SDL2)
    target_compile_options(reflood_editor_ui_tests PRIVATE -Wall -Wextra -Wpedantic)
  else()
    message(STATUS "SDL2 not found: building headless host. Install SDL2 development files and reconfigure for the SDL window.")
  endif()
endif()
