add_executable(Dummy
    src/Dummy.cpp
    src/DummyData.cpp
    src/DummyGroup.cpp
    src/Player.cpp
    src/PlayerState.cpp
    src/Server.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/Common/FVector.cpp"
)

target_compile_features(Dummy PRIVATE cxx_std_20)
target_compile_definitions(Dummy PRIVATE PSH_CLIENT UNICODE _UNICODE)
target_include_directories(Dummy PRIVATE src "${CMAKE_CURRENT_SOURCE_DIR}/../src/Common")
target_link_libraries(Dummy PRIVATE NetLib)
if(MSVC)
    target_compile_options(Dummy PRIVATE /utf-8)
endif()

set(DUMMY_RUNTIME_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/DummySetting.toml"
    "${CMAKE_CURRENT_SOURCE_DIR}/chatData.txt"
)
target_sources(Dummy PRIVATE ${DUMMY_RUNTIME_FILES})
set_source_files_properties(${DUMMY_RUNTIME_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_property(TARGET Dummy APPEND PROPERTY LINK_DEPENDS ${DUMMY_RUNTIME_FILES})
add_custom_command(TARGET Dummy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DUMMY_RUNTIME_FILES} "$<TARGET_FILE_DIR:Dummy>"
    COMMAND_EXPAND_LISTS
    VERBATIM)
set_target_properties(Dummy PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Dummy>")
