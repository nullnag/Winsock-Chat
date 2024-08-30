# CMakeList.txt: проект CMake для Server; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#

# Добавьте источник в исполняемый файл этого проекта.
add_executable (Server "Server.cpp" "Server.h" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Server PROPERTY CXX_STANDARD 20)
endif()

  
set(SQLITE3_SRC_DIR "C:/Users/boiko/source/repos/Server/Server")
set(SQLITE3_SRCS
    ${SQLITE3_SRC_DIR}/sqlite3.c
)
add_library(sqlite3 STATIC ${SQLITE3_SRCS})
target_include_directories(sqlite3 PUBLIC ${SQLITE3_SRC_DIR})
target_link_libraries(Server PRIVATE sqlite3)
include_directories("C:/Users/boiko/source/repos/Server/json/single_include/nlohmann")
