#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int TRIG_PIN   = 12;
const int ECHO_PIN   = 11;
const int BUZZER_PIN = 2;

unsigned long lastBeep = 0;
bool buzzerOn          = false;
long prevDistance      = 999;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin();
  Wire.setClock(100000);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED OK at 0x3C");
  } else if (display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println("OLED OK at 0x3D");
  } else {
    Serial.println("OLED not found — running without display");
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(20, 25);
  display.print("READY");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();

  Serial.println("Ready");
}

long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 15000);
  if (duration == 0) return 400;
  return duration / 58;
}

void updateOLED(long distance, long delta, int beepInterval) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // -- Distance big text ---
  display.setTextSize(3);
  display.setCursor(0, 0);
  if (distance >= 400) display.print("---");
  else {
    display.print(distance);
    display.setTextSize(1);
    display.setCursor(display.getCursorX(), 16);
    display.print("cm");
  }

  // --- Delta (approach speed) ---
  display.setTextSize(1);
  display.setCursor(0, 36);
  display.print("Delta: ");
  display.print(delta);
  display.print(" cm");

  // --- Interval ---
  display.setCursor(0, 46);
  display.print("Interval: ");
  if (beepInterval == 0) display.print("silent");
  else { display.print(beepInterval); display.print("ms"); }

  // --- Status ---
  display.setTextSize(1);
  display.setCursor(0, 56);
  if      (distance > 50)  display.print("CLEAR");
  else if (distance <= 10) display.print("!! DANGER !!");
  else if (distance <= 20) display.print(">> WARNING");
  else if (distance <= 35) display.print("CAUTION");
  else                     display.print("NEAR");

  display.display();
}

void loop() {
  unsigned long now = millis();

  long distance = getDistance();

  if (distance >= 400) {
    Serial.println("No object detected");
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
    prevDistance = 999;

    // OLED no object
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.print("NO OBJECT");
    display.display();

    delay(200);
    return;
  }

  // Approach speed
  long delta = prevDistance - distance;
  prevDistance = distance;

  // Beep interval
  int beepInterval;
  if (distance > 50) {
    beepInterval = 0;
  } else {
    beepInterval = map(distance, 1, 50, 60, 900);
  }

  // Boost on rapid approach
  if (delta > 0 && beepInterval > 0) {
    int boost = map(constrain(delta, 0, 20), 0, 20, 0, 50);
    beepInterval = beepInterval - (beepInterval * boost / 100);
    beepInterval = constrain(beepInterval, 40, 900);
  }

  // Serial output
  Serial.print("Dist: ");
  Serial.print(distance);
  Serial.print("cm | Delta: ");
  Serial.print(delta);
  Serial.print("cm | Interval: ");
  if (beepInterval == 0) Serial.print("silent");
  else { Serial.print(beepInterval); Serial.print("ms"); }
  Serial.print(" | ");
  if      (distance > 50)  Serial.println("CLEAR");
  else if (distance <= 10) Serial.println("!! DANGER !!");
  else if (distance <= 20) Serial.println(">> WARNING");
  else if (distance <= 35) Serial.println("CAUTION");
  else                     Serial.println("NEAR");

  // OLED output — same info as serial
  updateOLED(distance, delta, beepInterval);

  
  if (beepInterval == 0) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
    return;
  }

  if (!buzzerOn && (now - lastBeep) >= (unsigned long)beepInterval) {
    lastBeep = now;
    buzzerOn = true;
    digitalWrite(BUZZER_PIN, HIGH);
  }

  if (buzzerOn && (now - lastBeep) >= 40) {
    buzzerOn = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
}
