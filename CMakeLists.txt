cmake_minimum_required(VERSION 3.16)
project(HardcoreSpaceInvaders CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mac dinh build Release neu khong chi dinh - trong CMake, khong set CMAKE_BUILD_TYPE
# nghia la KHONG co flag toi uu hoa nao ca (tuong duong -O0), pha vo toan bo cong suc
# toi uu thuat toan/cache-friendliness da lam trong code (SpatialGrid, EnemyPool...).
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(raylib REQUIRED)

file(GLOB SOURCES "src/*.cpp")

add_executable(space_invaders ${SOURCES})
target_link_libraries(space_invaders raylib)

# Trên Linux, raylib thường cần thêm các thư viện hệ thống sau nếu link static
if (UNIX AND NOT APPLE)
    target_link_libraries(space_invaders m pthread dl rt X11)
endif()
