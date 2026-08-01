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

# ==========================================
# UNIT TESTS (Catch2 v2, single-header, vendor sẵn tại tests/thirdparty/catch.hpp -
# không cần internet/package manager lúc build, không phụ thuộc gì ngoài raylib đã có
# sẵn). Chỉ test các khối logic THUẦN (Swept-AABB CCD, SpatialGrid, Leaderboard) - không
# cần InitWindow/GPU context nào, nên chạy được cả trong CI headless.
# ==========================================
option(BUILD_TESTS "Build unit test suite (Catch2)" ON)
if(BUILD_TESTS)
    enable_testing()

    add_executable(unit_tests
        tests/test_main.cpp
        tests/test_bullet_ccd.cpp
        tests/test_spatial_grid.cpp
        tests/test_leaderboard.cpp
        tests/test_balance_config.cpp
        src/leaderboard.cpp
        src/config.cpp
    )
    target_include_directories(unit_tests PRIVATE src tests)
    target_link_libraries(unit_tests raylib)
    if (UNIX AND NOT APPLE)
        target_link_libraries(unit_tests m pthread dl rt X11)
    endif()

    add_test(NAME unit_tests COMMAND unit_tests)
endif()
