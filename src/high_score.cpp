#include "high_score.h"
#include "raylib.h"
#include <fstream>

// Ghi chú: không gọi file.close() thủ công ở đây - std::ifstream/std::ofstream là RAII,
// destructor tự đóng file khi ra khỏi scope. Gọi close() tay chỉ là rác, dễ khiến người
// đọc tưởng nhầm là bắt buộc, và không giúp gì khi có lỗi ghi giữa chừng (file vẫn mở
// tới cuối scope). Xử lý lỗi đúng cách phải dựa vào trạng thái stream (fail()/is_open()),
// không phải vào việc có gọi close() hay không.

void HighScore::Load(const std::string& path) {
    filePath = path;
    lastWriteFailed = false;

    std::ifstream file(filePath);
    if (file.is_open()) {
        file >> value;
        if (file.fail()) value = 0; // File tồn tại nhưng nội dung hỏng -> coi như chưa có điểm
    } else {
        value = 0; // Chưa từng chơi -> chưa có file, không phải lỗi
    }
}

bool HighScore::TrySubmit(int score) {
    if (score <= value) return false;
    value = score; // Cập nhật trong RAM ngay - kỷ lục trong phiên chơi vẫn hợp lệ dù ghi file có thất bại

    std::ofstream file(filePath);
    if (!file.is_open()) {
        // Ổ cứng từ chối quyền ghi, hết dung lượng, hoặc thư mục không tồn tại -> không
        // crash, chỉ cảnh báo và để phiên chơi tiếp tục bình thường.
        lastWriteFailed = true;
        TraceLog(LOG_WARNING, "HighScore: khong the mo file '%s' de ghi diem cao", filePath.c_str());
        return true;
    }

    file << value;
    if (file.fail()) {
        // Mở file thành công nhưng ghi giữa chừng thất bại (vd đĩa đầy) -> vẫn báo lỗi
        // thay vì im lặng coi như đã lưu.
        lastWriteFailed = true;
        TraceLog(LOG_WARNING, "HighScore: ghi diem cao vao '%s' that bai", filePath.c_str());
    } else {
        lastWriteFailed = false;
    }

    return true;
}
