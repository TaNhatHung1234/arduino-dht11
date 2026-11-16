# Dự đoán nhiệt độ trong nhà bằng LSTM

Hệ thống thu thập dữ liệu môi trường trong nhà từ ESP32 (nhiệt độ, độ ẩm, khí gas, bụi mịn) và dùng mạng LSTM để dự đoán nhiệt độ trong 30 phút tới.

## Phần cứng

| Linh kiện | Chân ESP32 | Đo được |
|---|---|---|
| DHT22 | GPIO 4 | Nhiệt độ (°C), độ ẩm (%) |
| MQ-2 | GPIO 34 (ADC) | Giá trị ADC thô của khí gas (0–4095) |
| GP2Y1010AU0F | LED: GPIO 26, Output: GPIO 35 (ADC) | Nồng độ bụi (µg/m³) |

Vi điều khiển: ESP32. Cần mạch chia áp nếu điện áp đầu ra của cảm biến bụi vượt 3,3 V.

## Cấu trúc thư mục

```
.
├── arduino/sensor_node/sensor_node.ino   # Firmware ESP32 (FreeRTOS)
├── data/sensor_data.csv                  # Dữ liệu thu thập được
├── notebooks/temperature_lstm_final.ipynb # Tiền xử lý, huấn luyện, đánh giá
├── requirements.txt
└── README.md
```

## Dữ liệu

File `data/sensor_data.csv`:

| Cột | Ý nghĩa |
|---|---|
| `timestamp` | Thời gian (UTC+7), đồng bộ qua NTP |
| `temperature` | Nhiệt độ (°C) |
| `humidity` | Độ ẩm (%) |
| `gas_raw` | Giá trị ADC thô của MQ-2 (chưa quy đổi ppm) |
| `dust_ugm3` | Nồng độ bụi (µg/m³, chưa hiệu chuẩn) |

- Tần suất lấy mẫu: 2 giây/mẫu, được gộp trung bình thành 3 phút/mẫu khi huấn luyện.
- Thời gian thu thập: [số ngày], từ [ngày bắt đầu] đến [ngày kết thúc].
- Vị trí: [phòng/vị trí].

## Phương pháp

1. **Tiền xử lý**: chuyển `timestamp` sang datetime, gộp trung bình theo 3 phút, mã hóa thời gian trong ngày bằng sin/cos.
2. **Chuẩn hóa**: `StandardScaler` fit trên tập train, áp dụng cho tập test để tránh rò rỉ dữ liệu.
3. **Cửa sổ trượt**: dùng 30 mẫu lịch sử (90 phút) để dự đoán nhiệt độ 10 mẫu sau (30 phút).
4. **Chia dữ liệu theo thời gian**: 70% train, phần còn lại test, không xáo trộn.
5. **Mô hình**: LSTM 2 lớp (64, 32 đơn vị), Dense 32 và Dense 1, loss MSE, optimizer Adam, EarlyStopping.
6. **Baseline**: dự đoán nhiệt độ tương lai bằng nhiệt độ hiện tại.

## Kết quả

| Mô hình | MAE (°C) | RMSE (°C) |
|---|---|---|
| Baseline (giữ nguyên nhiệt độ) | [...] | [...] |
| LSTM | [...] | [...] |

[Nhận xét: LSTM có vượt baseline không, và ở horizon nào.]

## Hạn chế

- DHT22 có sai số khoảng ±0,5 °C, giới hạn độ chính xác của mục tiêu dự đoán.
- Dữ liệu chỉ thu trong [số ngày] ngày, chưa đủ để mô hình học biến động theo mùa.
- Khí gas và bụi chưa hiệu chuẩn sang đơn vị chuẩn.
- Dữ liệu từ một vị trí duy nhất, chưa kiểm chứng khả năng tổng quát hóa.

## Cách chạy

### 1. Nạp firmware

1. Mở `arduino/sensor_node/sensor_node.ino` bằng Arduino IDE hoặc PlatformIO.
2. Sửa SSID và mật khẩu Wi-Fi trong code.
3. Chọn board ESP32, nạp code, mở Serial Monitor ở 115200 baud.
4. Cài thư viện `DHT sensor library` (Adafruit).

### 2. Huấn luyện và đánh giá

```bash
git clone <url-repo>
cd <ten-repo>
pip install -r requirements.txt
jupyter notebook notebooks/temperature_lstm_final.ipynb
```

Chạy các cell từ trên xuống dưới. Notebook đọc dữ liệu từ `../data/sensor_data.csv`.

## Yêu cầu

- ESP32 với Arduino core, thư viện DHT sensor library
- Python 3.10+ (đã kiểm tra với TensorFlow 2.19)

## Giấy phép

[MIT / loại giấy phép bạn chọn]