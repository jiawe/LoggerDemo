# CMakeList.txt: CMakeProject1 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("commonlog"
    DESCRIPTION "log library"
    VERSION 0.9.1
    )
set(CMAKE_CXX_STANDARD 11)



#可执行程序生成目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Lib)
#lib生成目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Lib)

set(CMAKE_AUTOMOC ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_VERBOSE_MAKEFILEON ON)

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CURRENT_SYSTEM Linux)
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(CURRENT_SYSTEM Windows)
ENDIF()


#静态库
add_library (${PROJECT_NAME}           
            logsrc/logger.cpp
            logsrc/RC4.cpp
            )

add_executable(${PROJECT_NAME}_test
    main.cpp
)



target_include_directories( ${PROJECT_NAME}_test
    PRIVATE 
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries( ${PROJECT_NAME}_test
    PRIVATE 
    ${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.lib
)

# target_link_libraries( "test" 
#     PRIVATE 
#     ${PROJECT_SOURCE_DIR}/build/librhlog.a
# )


#动态库
# add_library (${PROJECT_NAME} SHARED         
#             logger.cpp
#             RC4.cpp
#             )
