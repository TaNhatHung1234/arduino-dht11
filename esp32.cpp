#include <Arduino.h>
#include <DHT.h>
#include <time.h>

// ===== Cấu hình chân =====
#define DHT_PIN          4
#define DHT_TYPE         DHT22
#define MQ2_PIN          34   // ADC1, chỉ đọc (input only)
#define DUST_LED_PIN     26
#define DUST_PIN         35   // ADC1, chỉ đọc (input only)

// ===== Cấu hình lấy mẫu =====
#define SAMPLE_PERIOD_MS 2000  // DHT22 cần tối thiểu 2 s giữa 2 lần đọc
#define DUST_AVG_SAMPLES 5
#define QUEUE_LEN        10

struct SensorData {
  char  timestamp[24];
  float temperature;   // °C
  float humidity;      // %
  int   gasRaw;        // 0-4095 (MQ2 chưa quy đổi ppm)
  float dustUgm3;      // µg/m³ (cần hiệu chuẩn)
};

static DHT dht(DHT_PIN, DHT_TYPE);
static QueueHandle_t dataQueue;

// Lấy timestamp: ISO nếu đã đồng bộ NTP, ngược lại dùng uptime
static void makeTimestamp(char* buf, size_t len) {
  struct tm t;
  if (getLocalTime(&t, 50)) {
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &t);
  } else {
    snprintf(buf, len, "uptime_%lu", (unsigned long)millis());
  }
}

// Đọc bụi: lấy trung bình nhiều xung LED để giảm nhiễu
static float readDustUgm3() {
  uint32_t sumMv = 0;
  for (int i = 0; i < DUST_AVG_SAMPLES; i++) {
    digitalWrite(DUST_LED_PIN, LOW);      // bật LED hồng ngoại
    delayMicroseconds(280);               // chờ đến thời điểm lấy mẫu
    sumMv += analogReadMilliVolts(DUST_PIN);
    delayMicroseconds(40);
    digitalWrite(DUST_LED_PIN, HIGH);     // tắt LED
    delayMicroseconds(9680);              // chu kỳ ~10 ms
  }
  float volts = (sumMv / (float)DUST_AVG_SAMPLES) / 1000.0f;

  // GP2Y1010: 0,5 V ~ 100 µg/m³, điện áp nền ~0,6 V
  float ugm3 = (volts - 0.6f) * 200.0f;
  return ugm3 < 0 ? 0 : ugm3;
}

// Task đọc cảm biến, chạy đều mỗi SAMPLE_PERIOD_MS
static void sensorTask(void* pv) {
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    SensorData d;
    makeTimestamp(d.timestamp, sizeof(d.timestamp));

    d.temperature = dht.readTemperature();
    d.humidity    = dht.readHumidity();
    if (isnan(d.temperature) || isnan(d.humidity)) {
      d.temperature = NAN;   // để Python nhận là giá trị thiếu
      d.humidity    = NAN;
    }

    d.gasRaw   = analogRead(MQ2_PIN);
    d.dustUgm3 = readDustUgm3();

    // Nếu queue đầy thì bỏ mẫu mới (có thể đổi thành bỏ mẫu cũ)
    xQueueSend(dataQueue, &d, 0);

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
  }
}

// Task xuất dữ liệu ra Serial dạng CSV
static void outputTask(void* pv) {
  SensorData d;
  for (;;) {
    if (xQueueReceive(dataQueue, &d, portMAX_DELAY) == pdTRUE) {
      Serial.printf("%s,%.2f,%.2f,%d,%.2f\n",
                    d.timestamp, d.temperature, d.humidity,
                    d.gasRaw, d.dustUgm3);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  dht.begin();

  pinMode(DUST_LED_PIN, OUTPUT);
  digitalWrite(DUST_LED_PIN, HIGH);

  // Dải đo ADC ~0–3,1 V; nếu cảm biến bụi xuất trên 3,3 V thì cần chia áp
  analogSetPinAttenuation(DUST_PIN, ADC_11db);

  dataQueue = xQueueCreate(QUEUE_LEN, sizeof(SensorData));

  Serial.println("timestamp,temperature,humidity,gas_raw,dust_ugm3");

  xTaskCreatePinnedToCore(sensorTask, "sensor", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(outputTask, "output", 4096, NULL, 1, NULL, 0);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));   // loop không làm gì, mọi việc do task xử lý
}
