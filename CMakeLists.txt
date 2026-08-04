cmake_minimum_required(VERSION 3.16)
project(HardcoreSpaceInvaders CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# std::from_chars() cho float/double (level_config.cpp, settings.cpp) chi duoc libstdc++
# (GCC) ho tro DAY DU tu GCC 11 tro len - ban cu hon se bao loi compile kho hieu, tro thang
# vao <charconv> thay vi vao code cua chinh minh. Chan som o day, thong bao ro rang, thay vi
# de loi troi len tu 1 file .cpp cu the.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
    message(FATAL_ERROR
        "vibeKing can GCC >= 11 (std::from_chars<float> chi duoc libstdc++ ho tro day du tu "
        "ban nay). GCC hien tai: ${CMAKE_CXX_COMPILER_VERSION}. Dung Clang moi hoac cai GCC moi hon.")
endif()

# Mac dinh build Release neu khong chi dinh - trong CMake, khong set CMAKE_BUILD_TYPE
# nghia la KHONG co flag toi uu hoa nao ca (tuong duong -O0), pha vo toan bo cong suc
# toi uu thuat toan/cache-friendliness da lam trong code (SpatialGrid, EnemyPool...).
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
# QUAN TRONG: phai tu them lai -DNDEBUG - set() nay GHI DE toan bo gia tri mac dinh cua
# CMake cho bien nay (von la "-O3 -DNDEBUG" voi GNU), khong phai BO SUNG vao do. Thieu
# -DNDEBUG nghia la moi assert() (spatial_grid.h, bullet_pool.h, bunker.cpp...) van ACTIVE
# trong ban "Release" - vua ton CPU kiem tra moi frame, vua co the abort() giua demo/gameplay
# thuc te neu 1 assert nao do lo trigger, thay vi duoc compile away nhu dung nghia "release".
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

find_package(raylib REQUIRED)

# Liet ke tuong minh thay vi file(GLOB "src/*.cpp") - GLOB khong lam CMake biet duoc
# khi co file .cpp MOI duoc them vao (theo dung khuyen cao cua CMake docs), nen build
# incremental sau khi them file moi se ÂM THẦM thiếu file do neu khong tu tay chay lai
# cmake configure. Them file .cpp moi -> nho them dong tuong ung vao day.
set(SOURCES
    src/audio_system.cpp
    src/bunker.cpp
    src/config.cpp
    src/file_logger.cpp
    src/game_manager.cpp
    src/leaderboard.cpp
    src/level_config.cpp
    src/main.cpp
    src/physics_system.cpp
    src/player.cpp
    src/render_system.cpp
    src/settings.cpp
    src/sprites.cpp
    src/meta_progress.cpp
)

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
        tests/test_bunker.cpp
        tests/test_player.cpp
        tests/test_settings.cpp
        tests/test_boss.cpp
        tests/test_meta_progress.cpp
        src/leaderboard.cpp
        src/config.cpp
        src/bunker.cpp
        src/player.cpp
        src/settings.cpp
        src/meta_progress.cpp
    )
    target_include_directories(unit_tests PRIVATE src tests)
    target_link_libraries(unit_tests raylib)
    if (UNIX AND NOT APPLE)
        target_link_libraries(unit_tests m pthread dl rt X11)
    endif()

    add_test(NAME unit_tests COMMAND unit_tests)
endif()
