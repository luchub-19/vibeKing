#pragma once
#include "raylib.h"

// ==========================================
// VOICE POOL - chống "voice stealing"
// PlaySound() gọi liên tiếp trên CÙNG 1 Sound sẽ CẮT CỤT bản đang phát trước đó (raylib
// chỉ giữ đúng 1 trạng thái phát cho mỗi Sound). Khi nhiều sự kiện âm thanh xảy ra gần
// nhau (combo hạ nhiều địch cùng lúc, bắn liên tục lúc có Rapid Fire...) tiếng nổ/tiếng
// bắn trước đó bị ngắt giữa chừng, nghe rất giật cục.
//
// `PlaySoundMulti`/`StopSoundMulti` đã bị raylib loại bỏ khỏi bản 5.x (kiểm tra source
// raylib 5.5 đang link - không còn tồn tại). Thay thế chính thức là LoadSoundAlias():
// tạo N "giọng" (voice) dùng CHUNG dữ liệu mẫu gốc (không tốn thêm RAM cho waveform,
// chỉ tốn thêm N con trỏ AudioBuffer nhỏ) nhưng MỖI alias có trạng thái phát RIÊNG -
// round-robin qua các alias này cho phép tối đa N bản phát chồng lấn nhau mà không bản
// nào bị cắt cụt sớm.
// ==========================================
template <int Voices>
class VoicePool {
private:
    Sound base{};        // Sở hữu dữ liệu mẫu thật sự (từ GenerateTone/LoadSoundFromWave)
    Sound voices[Voices]{};
    int nextVoice = 0;
    bool loaded = false;

public:
    // `source`: Sound gốc đã có sẵn dữ liệu mẫu - VoicePool nhận quyền sở hữu (Unload()
    // sẽ giải phóng luôn `source`), không cần Unload() nó ở nơi gọi.
    void Init(Sound source) {
        base = source;
        loaded = true;

        // BUG THẬT SỰ TRONG RAYLIB 5.5: LoadSoundAlias() deref thẳng
        // `source.stream.buffer->data` mà KHÔNG kiểm tra buffer null trước (xem raudio.c
        // dòng 971). Khi không có audio device thật (container/CI không có sound card,
        // hoặc driver lỗi), LoadSoundFromWave() phía trên trả về Sound với
        // stream.buffer == NULL - gọi LoadSoundAlias() trên đó segfault ngay lập tức.
        // IsSoundValid() kiểm tra đúng điều kiện đó (frameCount>0 && buffer!=NULL &&...)
        // trước khi dám tạo alias - nếu base không hợp lệ, voices[] giữ nguyên {0} và
        // Play()/SetVolume() bên dưới sẽ tự bỏ qua an toàn (PlaySound/SetSoundVolume của
        // raylib tự no-op trên Sound rỗng, không crash).
        if (!IsSoundValid(base)) return;

        for (int i = 0; i < Voices; i++) voices[i] = LoadSoundAlias(base);
    }

    void Play() {
        PlaySound(voices[nextVoice]);
        nextVoice = (nextVoice + 1) % Voices;
    }

    void SetVolume(float v) {
        SetSoundVolume(base, v);
        for (int i = 0; i < Voices; i++) SetSoundVolume(voices[i], v);
    }

    void Unload() {
        if (!loaded) return;
        for (int i = 0; i < Voices; i++) UnloadSoundAlias(voices[i]);
        UnloadSound(base);
        loaded = false;
    }
};
