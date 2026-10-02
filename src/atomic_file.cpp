#include "atomic_file.h"

// KHONG include raylib.h trong file nay - xem ly do o atomic_file.h.
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX // libstdc++ cua MinGW tu dinh nghia san - define lai la warning (-Werror)
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <cstdio>
#endif

bool AtomicFile::Replace(const std::string& tmp, const std::string& dst) {
#if defined(_WIN32)
    // MOVEFILE_REPLACE_EXISTING: dung cai std::rename() cua Windows thieu. WRITE_THROUGH: chi
    // tra ve sau khi thao tac da xuong dia, gan nhat voi dam bao cua rename(2) tren POSIX.
    return MoveFileExA(tmp.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(tmp.c_str(), dst.c_str()) == 0;
#endif
}
