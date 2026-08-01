#include "file_logger.h"
#include "raylib.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>

#if defined(_WIN32)
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #define MKDIR(path) mkdir(path, 0755)
#endif

namespace {
    constexpr long MAX_LOG_SIZE_BYTES = 1 * 1024 * 1024; // 1 MB - vuot nguong nay thi xoay vong
    constexpr int MAX_BACKUPS = 5;

    std::string logDir = "logs";
    std::string currentLogPath;
    FILE* logFile = nullptr;

    const char* LevelName(int msgType) {
        switch (msgType) {
            case LOG_TRACE:   return "TRACE";
            case LOG_DEBUG:   return "DEBUG";
            case LOG_INFO:    return "INFO";
            case LOG_WARNING: return "WARN";
            case LOG_ERROR:   return "ERROR";
            case LOG_FATAL:   return "FATAL";
            default:          return "LOG";
        }
    }

    std::string BackupPath(int n) {
        return logDir + "/game." + std::to_string(n) + ".log";
    }

    void WriteTimestampHeader(const char* label) {
        if (!logFile) return;
        time_t now = time(nullptr);
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(logFile, "\n===== %s luc %s =====\n", label, timeBuf);
        fflush(logFile);
    }

    // XOAY VONG: game.log hien tai vuot MAX_LOG_SIZE_BYTES -> day lui game.1.log thanh
    // game.2.log, ..., game.(N-1).log thanh game.N.log (ban thu MAX_BACKUPS+1 tro len bi
    // mat), roi doi ten game.log -> game.1.log, mo lai game.log rong de ghi tiep.
    void RotateIfNeeded() {
        if (!logFile) return;
        long size = ftell(logFile);
        if (size < 0 || size < MAX_LOG_SIZE_BYTES) return;

        fclose(logFile);
        logFile = nullptr;

        std::remove(BackupPath(MAX_BACKUPS).c_str()); // Ban cu nhat, bi loai khoi vong xoay
        for (int i = MAX_BACKUPS - 1; i >= 1; i--) {
            std::rename(BackupPath(i).c_str(), BackupPath(i + 1).c_str());
        }
        std::rename(currentLogPath.c_str(), BackupPath(1).c_str());

        logFile = fopen(currentLogPath.c_str(), "a");
        WriteTimestampHeader("Xoay vong log - file moi bat dau");
    }

    void TraceLogCallbackImpl(int msgType, const char* text, va_list args) {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), text, args);

        // Van in ra console/stdout GIONG HANH VI MAC DINH cua raylib (huu ich luc dev
        // chay truc tiep tu terminal) - FileLogger CHI THEM ghi ra file, khong thay the.
        printf("%s: %s\n", LevelName(msgType), buf);

        if (!logFile) return;

        time_t now = time(nullptr);
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(logFile, "[%s] [%s] %s\n", timeBuf, LevelName(msgType), buf);
        fflush(logFile); // Flush NGAY - can dau vet neu game crash ngay sau dong log nay

        RotateIfNeeded();
    }
}

void FileLogger::Init(const std::string& logDirectory) {
    logDir = logDirectory;
    MKDIR(logDir.c_str()); // Bo qua ket qua tra ve - da ton tai san la binh thuong, khong phai loi

    currentLogPath = logDir + "/game.log";
    logFile = fopen(currentLogPath.c_str(), "a");
    if (!logFile) {
        // Khong the ghi log ra dia (vd thu muc chi-doc) - game van phai chay tiep binh
        // thuong, chi mat kha nang ghi file (TraceLogCallbackImpl van in console).
        printf("WARN: FileLogger: khong the mo '%s' de ghi - chi in ra console\n", currentLogPath.c_str());
    } else {
        WriteTimestampHeader("Phien choi moi bat dau");
    }

    SetTraceLogCallback(TraceLogCallbackImpl);
}

void FileLogger::Shutdown() {
    if (logFile) {
        WriteTimestampHeader("Phien choi ket thuc binh thuong");
        fclose(logFile);
        logFile = nullptr;
    }
}
