cmake_minimum_required(VERSION 3.10)
project(WaterEffect)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем флаги компилятора для Windows
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Исходные файлы
set(SOURCE_FILES
    src/main.cpp
    src/WaterEffect.cpp
)

# Заголовочные файлы
set(HEADER_FILES
    src/WaterEffect.h
)

# Создание исполняемого файла
add_executable(WaterEffect ${SOURCE_FILES} ${HEADER_FILES})

# Добавление библиотек Windows
target_link_libraries(WaterEffect
    d2d1
    dwrite
    windowscodecs
    dwmapi
)

# Установка выходного каталога
set_target_properties(WaterEffect PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
) 