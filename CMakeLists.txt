cmake_minimum_required(VERSION 3.15)
project(ScreenshotServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки для Windows
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
    set(WINDOWS_LIBS gdiplus windowscodecs ole32 ws2_32 uuid shell32 comctl32)
    
    # Скрываем консоль и указываем WinMain
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup")
endif()

# Исходные файлы
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/screenshot.cpp
)

# Линковка библиотек (Windows)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${WINDOWS_LIBS})
endif()