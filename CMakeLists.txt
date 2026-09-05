add_executable(Dummy
    src/Dummy.cpp
    src/DummyData.cpp
    src/DummyGroup.cpp
    src/Player.cpp
    src/PlayerState.cpp
    src/Server.cpp
    ../packetParser/FVector.cpp
)

target_compile_definitions(Dummy PRIVATE PSH_CLIENT)
target_include_directories(Dummy PRIVATE src ../packetParser)
target_link_libraries(Dummy PRIVATE NetLib)
if(MSVC)
    target_compile_options(Dummy PRIVATE /utf-8)
endif()

configure_file(DummySetting.toml DummySetting.toml COPYONLY)
configure_file(chatData.txt chatData.txt COPYONLY)
