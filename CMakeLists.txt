cmake_minimum_required(VERSION 3.20)

project(EidosEngine
    VERSION 0.1
    LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Настройка путей сборки
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )

    # Отчёт о падении бесполезен без имён функций: в Release тоже пишем
    # отладочные символы. На скорость это не влияет, /OPT:REF и /OPT:ICF
    # возвращают оптимизации, которые /DEBUG иначе отключает
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG>)
    add_link_options($<$<CONFIG:Release>:/OPT:REF>)
    add_link_options($<$<CONFIG:Release>:/OPT:ICF>)
endif()

add_subdirectory(external)
add_subdirectory(src)

# --- НОВЫЙ БЛОК ДЛЯ ОТЛАДКИ (F5) И ASSETS ---

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT EidosApp)

if (TARGET EidosApp)
    # === НАСТРОЙКА РАБОЧЕЙ ПАПКИ ===
    set_target_properties(EidosApp PROPERTIES 
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # === ДОБАВЛЕНИЕ ASSETS В РЕШЕНИЕ (ВИЗУАЛЬНО) ===
    # Находим все файлы
    file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
    
    # Добавляем в проект
    target_sources(EidosApp PRIVATE ${ASSET_FILES})
    
    # Группируем красиво по папкам
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/assets" PREFIX "assets" FILES ${ASSET_FILES})
endif()