/******************
 * ESP32 PIR + Temperature Armed/Disarmed System
 * Author : Milad Mohseni
 * Board  : ESP32 (30-pin)
 * 
 * Description:
 * - PIR sensor toggles system ARM / DISARM state
 * - DS18B20 monitors ambient temperature
 * - OLED displays system status
 * - LED1 stays ON after PIR detection until system is DISARMED
 * - LED2 lights if temperature is out of 23-25°C
 * - Buzzer alert remains the same
 ******************/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- PIN DEFINITIONS ----------
#define PIR_PIN        4      // PIR motion sensor
#define TEMP_PIN       16     // DS18B20 data pin
#define LED1_PIN       2      // PIR alert LED (persistent)
#define LED2_PIN       5      // Temperature alert LED
#define BUZZER_PIN     15     // Active buzzer

#define SDA_PIN        21     // OLED SDA
#define SCL_PIN        22     // OLED SCL

// ---------- TEMPERATURE LIMITS ----------
#define TEMP_LOW   23.0
#define TEMP_HIGH  25.0

// ---------- TIMING ----------
#define PIR_COOLDOWN 5000   // 5 seconds debounce for PIR

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- DS18B20 ----------
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// ---------- SYSTEM STATE ----------
bool systemArmed = false;
bool led1On = false;               // LED1 persistent state
unsigned long lastPirTrigger = 0;

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(PIR_PIN, INPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  // Initialize temperature sensor
  sensors.begin();

  // Initial display
  displayStatus("SYSTEM READY", "DISARMED");
}

void loop() {

  // --------- PIR ARM/DISARM TOGGLE ---------
  if (digitalRead(PIR_PIN) == HIGH &&
      millis() - lastPirTrigger > PIR_COOLDOWN) {

    systemArmed = !systemArmed;
    lastPirTrigger = millis();

    if (systemArmed) {
      displayStatus("SYSTEM ARMED", "Monitoring...");
    } else {
      displayStatus("SYSTEM SAFE", "DISARMED");
      digitalWrite(LED1_PIN, LOW);   // Turn off LED1 when disarmed
      led1On = false;
      digitalWrite(LED2_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
    }
  }

  // --------- LED1 Persistent PIR Alert ---------
  if(systemArmed && digitalRead(PIR_PIN) == HIGH) {
    led1On = true;                 // PIR detected → LED1 stays on
  }
  digitalWrite(LED1_PIN, led1On);

  // --------- TEMPERATURE MONITOR ---------
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // LED2 lights only when temp is out of range (23-25°C)
  if(tempC < TEMP_LOW || tempC > TEMP_HIGH) {
    digitalWrite(LED2_PIN, HIGH);
    // Louder buzzer
    if(systemArmed) {
      tone(BUZZER_PIN, 3000); // 3kHz tone
    }
  } else {
    digitalWrite(LED2_PIN, LOW);
    noTone(BUZZER_PIN);
  }

  // OLED display smaller font
  displayTemperature(tempC, systemArmed);
  
  delay(200);
}

// ---------- DISPLAY FUNCTIONS ----------

void displayStatus(String line1, String line2) {
  display.clearDisplay();
  display.setTextSize(1);  // smaller font
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(line1);
  display.setCursor(0, 30);
  display.println(line2);
  display.display();
}

void displayTemperature(float temp, bool armed) {
  display.clearDisplay();
  display.setTextSize(1); // smaller font
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Temp: ");
  display.print(temp, 1);
  display.println("C");

  display.setCursor(0, 20);
  display.print("System: ");
  display.println(armed ? "ARMED" : "DISARMED");

  display.setCursor(0, 40);
  if(temp < TEMP_LOW || temp > TEMP_HIGH) {
    display.println("Temp Status: ALERT");
  } else {
    display.println("Temp Status: NORMAL");
  }
  display.display();
}