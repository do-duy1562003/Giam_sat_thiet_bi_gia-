#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// ============ OLED Display Configuration ============
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============ Firebase Configuration ============
#define FIREBASE_HOST "YOUR_PROJECT_ID.firebaseio.com"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET"
#define API_KEY "YOUR_API_KEY"
#define USER_EMAIL "your_email@gmail.com"
#define USER_PASSWORD "your_password"
#define FIREBASE_PROJECT_ID "your_project_id"

FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// ============ Pin Configuration ============
#define FAN_PIN 5              // GPIO5 - Relay quạt
#define WINDOW_PIN 18          // GPIO18 - Servo cửa sổ
#define BUZZER_PIN 19          // GPIO19 - Còi cảnh báo
#define LED_RED_PIN 21         // GPIO21 - LED đỏ
#define LED_YELLOW_PIN 22      // GPIO22 - LED vàng
#define DHT_PIN 4              // GPIO4 - DHT22
#define BUTTON_PIN 23          // GPIO23 - Nút nhấn

// ============ Servo Configuration ============
Servo windowServo;
int servoPosition = 0;
#define SERVO_OPEN_ANGLE 180
#define SERVO_CLOSE_ANGLE 0

// ============ Device States ============
bool fanState = false;
bool windowState = false;
bool buzzerState = false;
bool redLedState = false;
bool yellowLedState = false;
float temperature = 0;
float humidity = 0;

// ============ WiFi Configuration ============
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// ============ Timing Variables ============
unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long lastFirebaseUpdate = 0;
unsigned long buzzerStartTime = 0;
unsigned long buzzerDuration = 0;
bool buzzerBlinking = false;

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32 Smart Home System with Firebase Starting...");
  
  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  
  // Initialize Pins
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize Servo
  windowServo.attach(WINDOW_PIN, 500, 2400);
  windowServo.write(SERVO_CLOSE_ANGLE);
  
  // Set all devices to OFF initially
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);
  
  delay(500);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize Firebase
  initializeFirebase();
  
  Serial.println("Setup Complete!");
}

// ============ MAIN LOOP ============
void loop() {
  // Reconnect to WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  // Handle button press
  handleButtonPress();
  
  // Manage buzzer blinking
  manageBuzzer();
  
  // Update OLED Display
  unsigned long currentTime = millis();
  if (currentTime - lastDisplayUpdate >= 1000) {
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }
  
  // Read sensors periodically
  if (currentTime - lastSensorRead >= 5000) {
    readSensors();
    lastSensorRead = currentTime;
  }
  
  // Update Firebase periodically
  if (currentTime - lastFirebaseUpdate >= 10000) {  // 10 seconds
    updateFirebase();
    lastFirebaseUpdate = currentTime;
  }
  
  delay(10);
}

// ============ WiFi Connection ============
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection Failed!");
  }
}

// ============ Firebase Initialization ============
void initializeFirebase() {
  Serial.println("Initializing Firebase...");
  
  // Assign the api key (required)
  config.api_key = API_KEY;
  
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  // Assign the RTDB URL (required)
  config.database_url = FIREBASE_HOST;
  
  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;
  
  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;
  
  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);
  
  // Enable auto reconnect the WiFi and Firebase when connection is lost
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(2);
  
  // Waiting for adding a new device
  unsigned long ms = millis();
  while (!auth.token.uid && (millis() - ms) < 3000) {
    delay(25);
  }
  
  Serial.print("User UID: ");
  Serial.println(auth.token.uid);
}

// ============ Token Status Callback ============
void tokenStatusCallback(token_info_t info) {
  if (info.status == token_status_on_request) {
    Serial.println("Token request...");
  } else if (info.status == token_status_on_refresh) {
    Serial.println("Token refresh...");
  } else if (info.status == token_status_on_error) {
    Serial.print("Token error, ");
    Serial.println(String(info.error.message).c_str());
  } else if (info.status == token_status_ready) {
    Serial.println("Token ready!");
  } else if (info.status == token_status_expired) {
    Serial.println("Token expired!");
  }
}

// ============ Update Firebase ============
void updateFirebase() {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready!");
    return;
  }
  
  Serial.println("Updating Firebase...");
  
  // Update device states
  Firebase.setBool(firebaseData, "/devices/fan/state", fanState);
  Firebase.setBool(firebaseData, "/devices/window/state", windowState);
  Firebase.setBool(firebaseData, "/devices/buzzer/state", buzzerState);
  Firebase.setBool(firebaseData, "/devices/redLed/state", redLedState);
  Firebase.setBool(firebaseData, "/devices/yellowLed/state", yellowLedState);
  
  // Update sensor data
  Firebase.setFloat(firebaseData, "/sensors/temperature", temperature);
  Firebase.setFloat(firebaseData, "/sensors/humidity", humidity);
  
  // Update status with timestamp
  Firebase.setString(firebaseData, "/status/lastUpdate", getTimestamp());
  Firebase.setString(firebaseData, "/status/wifiStatus", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  
  // Create detailed sensor log
  String sensorLogPath = "/logs/sensors/" + getTimestamp();
  FirebaseJson json;
  json.set("temperature", temperature);
  json.set("humidity", humidity);
  json.set("fanState", fanState);
  json.set("windowState", windowState);
  json.set("timestamp", getTimestamp());
  
  Firebase.setJSON(firebaseData, sensorLogPath, json);
  
  Serial.println("Firebase update complete!");
}

// ============ Handle Firebase Control Commands ============
void handleFirebaseCommands() {
  if (!Firebase.ready()) {
    return;
  }
  
  // Check for fan control command
  if (Firebase.getBool(firebaseData, "/commands/fan")) {
    if (firebaseData.dataType() == "boolean") {
      bool command = firebaseData.boolData();
      setFan(command);
      // Clear command
      Firebase.deleteNode(firebaseData, "/commands/fan");
    }
  }
  
  // Check for window control command
  if (Firebase.getBool(firebaseData, "/commands/window")) {
    if (firebaseData.dataType() == "boolean") {
      bool command = firebaseData.boolData();
      setWindow(command);
      Firebase.deleteNode(firebaseData, "/commands/window");
    }
  }
  
  // Check for buzzer control command
  if (Firebase.getBool(firebaseData, "/commands/buzzer")) {
    if (firebaseData.dataType() == "boolean") {
      bool command = firebaseData.boolData();
      if (command) {
        triggerBuzzer(3000);
      } else {
        stopBuzzer();
      }
      Firebase.deleteNode(firebaseData, "/commands/buzzer");
    }
  }
  
  // Check for alert command
  if (Firebase.getBool(firebaseData, "/commands/alert")) {
    if (firebaseData.dataType() == "boolean") {
      bool command = firebaseData.boolData();
      if (command) {
        triggerAlert();
        Firebase.deleteNode(firebaseData, "/commands/alert");
      }
    }
  }
}

// ============ Fan Control ============
void setFan(bool state) {
  fanState = state;
  digitalWrite(FAN_PIN, state ? HIGH : LOW);
  
  Serial.print("Fan: ");
  Serial.println(state ? "ON" : "OFF");
  
  // Log to Firebase
  if (Firebase.ready()) {
    Firebase.setString(firebaseData, "/logs/events", "Fan turned " + String(state ? "ON" : "OFF"));
  }
}

// ============ Window Control (Servo) ============
void setWindow(bool state) {
  windowState = state;
  int angle = state ? SERVO_OPEN_ANGLE : SERVO_CLOSE_ANGLE;
  
  // Smooth servo movement
  if (state) {
    for (int i = 0; i <= SERVO_OPEN_ANGLE; i += 10) {
      windowServo.write(i);
      delay(30);
    }
    windowServo.write(SERVO_OPEN_ANGLE);
  } else {
    for (int i = SERVO_OPEN_ANGLE; i >= SERVO_CLOSE_ANGLE; i -= 10) {
      windowServo.write(i);
      delay(30);
    }
    windowServo.write(SERVO_CLOSE_ANGLE);
  }
  
  servoPosition = angle;
  Serial.print("Window: ");
  Serial.println(state ? "OPEN" : "CLOSE");
  
  if (Firebase.ready()) {
    Firebase.setString(firebaseData, "/logs/events", "Window " + String(state ? "OPENED" : "CLOSED"));
  }
}

// ============ Buzzer Control ============
void triggerBuzzer(unsigned long duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  buzzerState = true;
  buzzerStartTime = millis();
  buzzerDuration = duration;
  buzzerBlinking = true;
  
  Serial.println("Buzzer: ON");
}

void stopBuzzer() {
  digitalWrite(BUZZER_PIN, LOW);
  buzzerState = false;
  buzzerBlinking = false;
  
  Serial.println("Buzzer: OFF");
}

void manageBuzzer() {
  if (buzzerBlinking) {
    unsigned long elapsed = millis() - buzzerStartTime;
    
    if (elapsed >= buzzerDuration) {
      stopBuzzer();
    } else {
      // Buzzer blink pattern
      if ((elapsed / 200) % 2 == 0) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
      }
    }
  }
}

// ============ Alert Trigger ============
void triggerAlert() {
  Serial.println("ALERT TRIGGERED!");
  triggerBuzzer(5000);
  
  digitalWrite(LED_RED_PIN, HIGH);
  redLedState = true;
  
  digitalWrite(LED_YELLOW_PIN, HIGH);
  yellowLedState = true;
  
  if (temperature > 35) {
    setWindow(true);
  }
  
  // Log alert to Firebase
  if (Firebase.ready()) {
    String alertPath = "/alerts/" + getTimestamp();
    FirebaseJson alertJson;
    alertJson.set("type", "temperature_alert");
    alertJson.set("temperature", temperature);
    alertJson.set("humidity", humidity);
    alertJson.set("severity", "high");
    Firebase.setJSON(firebaseData, alertPath, alertJson);
  }
}

// ============ Read Sensors ============
void readSensors() {
  // Simulated temperature and humidity reading
  temperature = random(20, 35);
  humidity = random(30, 80);
  
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print("C, Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  
  // Check thresholds and trigger alerts
  if (temperature > 35) {
    Serial.println("Temperature Alert!");
    if (Firebase.ready()) {
      Firebase.setBool(firebaseData, "/alerts/highTemp", true);
    }
  }
  
  if (humidity > 80) {
    Serial.println("Humidity Alert!");
    if (Firebase.ready()) {
      Firebase.setBool(firebaseData, "/alerts/highHumidity", true);
    }
  }
}

// ============ Button Handler ============
void handleButtonPress() {
  static unsigned long lastButtonPress = 0;
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastButtonPress > 300) {
      Serial.println("Button Pressed!");
      setFan(!fanState);
      lastButtonPress = currentTime;
    }
  }
}

// ============ Update OLED Display ============
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // Title
  display.println("=== Smart Home ===");
  display.println();
  
  // WiFi Status
  display.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    display.println("OK");
  } else {
    display.println("No Connection");
  }
  
  // Firebase Status
  display.print("Firebase: ");
  display.println(Firebase.ready() ? "OK" : "No Connection");
  
  display.println();
  
  // Device States
  display.setTextSize(1);
  display.print("FAN: ");
  display.println(fanState ? "ON" : "OFF");
  
  display.print("WINDOW: ");
  display.println(windowState ? "OPEN" : "CLOSE");
  
  display.print("RED LED: ");
  display.println(redLedState ? "ON" : "OFF");
  
  display.println();
  
  // Sensor Data
  display.print("Temp: ");
  display.print(temperature, 1);
  display.println("C");
  
  display.print("Humidity: ");
  display.print(humidity, 1);
  display.println("%");
  
  display.display();
}

// ============ Get Timestamp ============
String getTimestamp() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[30];
  strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}
