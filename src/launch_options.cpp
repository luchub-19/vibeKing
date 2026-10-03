#include "launch_options.h"
#include <algorithm>
#include <charconv>
#include <cmath>

namespace {
    // Doc so nguyen DUONG tu toan bo chuoi - "45abc"/"-3"/"" deu bi tu choi, khong lang le
    // lay phan dau nhu atoi() (doc sai 1 tham so bench roi do 0 frame thi kho hieu hon bao loi).
    bool ParsePositiveInt(const std::string& s, int& out) {
        int v = 0;
        auto res = std::from_chars(s.data(), s.data() + s.size(), v);
        if (res.ec != std::errc() || res.ptr != s.data() + s.size() || v <= 0) return false;
        out = v;
        return true;
    }

    bool StartsWith(const std::string& s, const char* prefix, std::string& rest) {
        std::string p(prefix);
        if (s.compare(0, p.size(), p) != 0) return false;
        rest = s.substr(p.size());
        return true;
    }
}

LaunchOptions ParseLaunchOptions(const std::vector<std::string>& args) {
    LaunchOptions o;
    for (const std::string& a : args) {
        std::string v;
        if (StartsWith(a, "--scene=", v)) {
            if (v == "combat")    o.scene = ShowcaseScene::Combat;
            else if (v == "boss") o.scene = ShowcaseScene::Boss;
            else if (v == "gameover") o.scene = ShowcaseScene::GameOver;
            else if (v == "waveclear") o.scene = ShowcaseScene::WaveClear;
            else if (v == "menu") o.scene = ShowcaseScene::Menu;
            else if (v == "hangar") o.scene = ShowcaseScene::Hangar;
            else if (v == "settings") o.scene = ShowcaseScene::Settings;
            else if (v == "howto") o.scene = ShowcaseScene::HowTo;
            else if (v == "attract") o.scene = ShowcaseScene::Attract;
            else if (v == "leaderboard") o.scene = ShowcaseScene::Leaderboard;
            else if (v == "achievements") o.scene = ShowcaseScene::Achievements;
            else if (v == "pause") o.scene = ShowcaseScene::Pause;
            else { o.error = "scene khong hop le: " + v; return o; }
        } else if (StartsWith(a, "--capture=", v)) {
            if (v.empty()) { o.error = "--capture can ten file"; return o; }
            o.captureFile = v;
        } else if (StartsWith(a, "--capture-frame=", v)) {
            if (!ParsePositiveInt(v, o.captureFrame)) { o.error = "--capture-frame phai la so nguyen duong"; return o; }
        } else if (StartsWith(a, "--quality=", v)) {
            // So nguyen khop thu tu enum GraphicsQuality (graphics_settings.h) - file nay giu
            // khong include settings de van la ham thuan doc lap.
            if (v == "low")         o.qualityOverride = 0;
            else if (v == "medium") o.qualityOverride = 1;
            else if (v == "high")   o.qualityOverride = 2;
            else { o.error = "quality khong hop le: " + v; return o; }
        } else if (StartsWith(a, "--lang=", v)) {
            if (v == "en")      o.languageOverride = 0; // Khop thu tu enum Language (localization.h)
            else if (v == "vi") o.languageOverride = 1;
            else { o.error = "lang khong hop le: " + v; return o; }
        } else if (StartsWith(a, "--tab=", v)) {
            if (v.size() != 1 || v[0] < '0' || v[0] > '3') { o.error = "--tab phai la 0..3"; return o; }
            o.settingsTab = v[0] - '0';
        } else if (StartsWith(a, "--bench=", v)) {
            if (!ParsePositiveInt(v, o.benchFrames)) { o.error = "--bench phai la so nguyen duong"; return o; }
        } else {
            o.error = "tham so khong ro: " + a;
            return o;
        }
    }
    // Chup va do cung luc: --bench bo gioi han FPS va thoat o frame cua no, --capture thoat
    // o frame cua no - frame nao toi truoc se cat ngang cai con lai. Tu choi ro rang.
    if (!o.captureFile.empty() && o.benchFrames > 0) o.error = "khong dung --capture va --bench cung luc";
    return o;
}

FrameStats SummarizeFrameTimes(std::vector<double> samplesMs) {
    FrameStats s;
    if (samplesMs.empty()) return s;
    std::sort(samplesMs.begin(), samplesMs.end());
    s.count = (int)samplesMs.size();
    double sum = 0.0;
    for (double v : samplesMs) sum += v;
    s.avgMs = sum / (double)s.count;
    s.maxMs = samplesMs.back();
    // Nearest-rank: phan tu thu ceil(0.95*n) (1-based). n=20 -> phan tu thu 19, KHONG phai 20
    // (do la max) - de nham nhat o day nen co test rieng.
    int rank = (int)std::ceil(0.95 * (double)s.count);
    if (rank < 1) rank = 1;
    s.p95Ms = samplesMs[(size_t)(rank - 1)];
    return s;
}

const char* LaunchUsage() {
    return "Cach dung: space_invaders [--scene=combat|boss|gameover|waveclear|menu|hangar|settings|howto|attract|"
           "leaderboard|achievements|pause] [--capture=<file.png>] [--capture-frame=<n>] [--quality=low|medium|high] "
           "[--lang=en|vi] [--tab=0..3] [--bench=<n>]\n";
}
