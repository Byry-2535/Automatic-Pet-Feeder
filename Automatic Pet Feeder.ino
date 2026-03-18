#include "arduino_secrets.h"
#include <ESP32Servo.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <time.h>
#include "thingProperties.h"

#define STOP_ANGLE      91
#define FORWARD_ANGLE   60
#define REVOLUTION_TIME 1600
#define MIN_FEED_INTERVAL 30000

Servo myservo;
int servoPin = 13;
const int ledPin = 2;

unsigned long lastFeedTime = 0;
int lastFedHour = -1;
int lastFedMinute = -1;
unsigned long lastScheduleCheck = 0;
unsigned long lastTimePrint = 0;
bool servoRunning = false;
int revolutionsDone = 0;
unsigned long lastRevolutionTime = 0;

struct tm timeinfo;
bool timeSynced = false;

unsigned long millisAtStart;
time_t epochStart;

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    myservo.attach(servoPin);
    myservo.write(STOP_ANGLE);

    initProperties();

    Serial.println("Connecting to WiFi...");
    WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    
    Serial.println("\nWiFi connected!");
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    epochStart = time(nullptr);
    millisAtStart = millis();
    Serial.print("Waiting for NTP...");

    if (getLocalTime(&timeinfo)) {
        timeSynced = true;
        Serial.printf("\nNTP OK: %02d:%02d PH\n", timeinfo.tm_hour, timeinfo.tm_min);
    } else {
        Serial.println("\nNTP failed");
        timeSynced = false;
    }

    Serial.println("Connecting to Arduino IoT Cloud...");
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    setDebugMessageLevel(2);
    ArduinoCloud.printDebugInfo();
    unsigned long startAttempt = millis();

    while (!ArduinoCloud.connected()) {
        Serial.print(".");
        delay(500);
        if (millis() - startAttempt > 20000) {
            Serial.println("\nCloud connection timeout, continuing anyway...");
            break;
        }
    }

    Serial.println("\nCloud connected or proceeding without cloud.");
    digitalWrite(ledPin, HIGH);
}

void loop() {
    ArduinoCloud.update();
    time_t now;
    
    if (getLocalTime(&timeinfo)) {
        timeinfo.tm_hour = (timeinfo.tm_hour + 8) % 24;
        timeSynced = true;
    } else {
        unsigned long elapsed = (millis() - millisAtStart) / 1000;
        now = epochStart + elapsed;
        localtime_r(&now, &timeinfo);
        timeinfo.tm_hour = (timeinfo.tm_hour + 8) % 24;
    }

    if (millis() - lastTimePrint > 10000) {
        Serial.printf("PH time: %02d:%02d:%02d %s\n",
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                      timeSynced ? "(NTP)" : "(manual PH)");
        lastTimePrint = millis();
    }

    if (servoSwitch && millis() - lastScheduleCheck > 1000) {
        checkSchedule();
        lastScheduleCheck = millis();
    }

    if (servoRunning) {
        digitalWrite(ledPin, millis() % 500 < 250);
        unsigned long nowMillis = millis();

        if (nowMillis - lastRevolutionTime >= REVOLUTION_TIME) {
            revolutionsDone++;
            lastRevolutionTime = nowMillis;
        }

        if (revolutionsDone >= userRevolutions) {
            myservo.write(STOP_ANGLE);
            servoRunning = false;
            digitalWrite(ledPin, HIGH);
            Serial.printf("Spin complete: %d revolutions\n", userRevolutions);
        }
    }
}

void startSpinCycle(int revolutions, bool manual) {
    if (!manual && millis() - lastFeedTime < MIN_FEED_INTERVAL) return;
    lastFeedTime = millis();
    userRevolutions = constrain(revolutions, 2, 5);
    revolutionsDone = 0;
    lastRevolutionTime = millis();
    myservo.write(FORWARD_ANGLE);
    servoRunning = true;
    Serial.printf("Starting spin: %d revolutions\n", userRevolutions);
}

void checkSchedule() {
    int h = timeinfo.tm_hour;
    int m = timeinfo.tm_min;
    int s = timeinfo.tm_sec;

    if (h == feedHour && m == feedMinute && s < 5) {
        if (!servoRunning && servoSwitch &&
            (lastFedHour != h || lastFedMinute != m)) {
            startSpinCycle(userRevolutions, false);
            lastFedHour = h;
            lastFedMinute = m;
            Serial.println("Scheduled feed executed");
        }
    }
}

void onFeedNowChange() {
    if (feedNow && !servoRunning) {
        startSpinCycle(userRevolutions, true);
        feedNow = false;
    }
}

void onRevolutionsChange() { Serial.printf("User set revolutions: %d\n", userRevolutions); }
void onFeedHourChange()     { Serial.printf("Feed hour updated: %d\n", feedHour); }
void onFeedMinuteChange()   { Serial.printf("Feed minute updated: %d\n", feedMinute); }
void onServoSwitchChange()  { Serial.printf("Servo switch updated: %s\n", servoSwitch ? "ON" : "OFF"); }