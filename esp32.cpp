#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

#define MQ2_PIN 34

#define DUST_LED_PIN 26
#define DUST_SENSOR_PIN 35

DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);

    dht.begin();

    pinMode(DUST_LED_PIN, OUTPUT);
    digitalWrite(DUST_LED_PIN, HIGH);

    Serial.println("Environmental Monitoring System");
}

float readDustDensity() {

    digitalWrite(DUST_LED_PIN, LOW);
    delayMicroseconds(280);

    int adcValue = analogRead(DUST_SENSOR_PIN);

    delayMicroseconds(40);
    digitalWrite(DUST_LED_PIN, HIGH);

    delayMicroseconds(9680);

    float voltage = adcValue * (3.3 / 4095.0);

    float dustDensity = (voltage - 0.6) * 1000 / 0.5;

    if (dustDensity < 0)
        dustDensity = 0;

    return dustDensity;
}

void loop() {

    // DHT22
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // MQ2
    int gasRaw = analogRead(MQ2_PIN);

    // Dust
    float dust = readDustDensity();

    Serial.println("====================");

    if (!isnan(temperature) && !isnan(humidity)) {
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" C");

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");
    }
    else {
        Serial.println("DHT22 Read Error");
    }

    Serial.print("Gas Sensor Raw: ");
    Serial.println(gasRaw);

    Serial.print("Dust Density: ");
    Serial.print(dust);
    Serial.println(" ug/m3");

    delay(2000);
}