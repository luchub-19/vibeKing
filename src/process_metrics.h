#pragma once

// ==========================================
// PROCESS METRICS - RAM tieu thu THAT SU cua tien trinh (khong phai uoc luong ly
// thuyet). Doc truc tiep VmRSS ("Resident Set Size" - bo nho vat ly THAT SU dang chiem
// dung trong RAM, khac voi VmSize la dung luong dia chi ao co the lon hon rat nhieu) tu
// /proc/self/status - co san tren moi nhan Linux, khong can thu vien ngoai nao.
//
// KHONG co cai dat cho Windows/macOS o day (du an nay build chinh tren Linux - xem
// CMakeLists.txt chi link X11 cho UNIX AND NOT APPLE) - tra ve -1 tren cac nen tang
// khac thay vi bia so lieu gia, RenderSystem se hien "N/A" khi gap gia tri nay.
// ==========================================
#if defined(__linux__)
    #include <cstdio>
    #include <cstring>

    inline long GetProcessRssKb() {
        FILE* f = fopen("/proc/self/status", "r");
        if (!f) return -1;

        char line[256];
        long rssKb = -1;
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line + 6, "%ld", &rssKb);
                break;
            }
        }
        fclose(f);
        return rssKb;
    }
#else
    inline long GetProcessRssKb() { return -1; }
#endif
