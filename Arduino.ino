#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Keypad.h>
#include <EEPROM.h>

// ============ LCD Configuration (2 x 16x2) ============
LiquidCrystal_I2C lcd1(0x27, 16, 2);  // LCD1 - Hiển thị kết quả
LiquidCrystal_I2C lcd2(0x26, 16, 2);  // LCD2 - Hiển thị input

// ============ RFID Configuration ============
#define RST_PIN 22
#define SS_PIN 5
MFRC522 rfid(SS_PIN, RST_PIN);

// ============ Keypad Configuration ============
const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {32, 33, 25, 26};      // Connect to row pins
byte colPins[COLS] = {27, 14, 12, 13};      // Connect to column pins

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// ============ Pin Configuration ============
#define BUZZER_PIN 19
#define DOOR_LOCK_PIN 15

// ============ EEPROM Configuration ============
#define EEPROM_SIZE 512
#define PASSWORD_ADDR 0
#define RFID_CARD_ADDR 32

// ============ Password & RFID Storage ============
String currentPassword = "1234";
String authorizedRFID = "12 34 56 78";
String currentInput = "";
String currentRFIDInput = "";

// ============ State Variables ============
enum Mode {
  NORMAL,
  CHANGE_PASSWORD_STEP1,
  CHANGE_PASSWORD_STEP2,
  CHANGE_PASSWORD_STEP3,
  CHANGE_RFID_STEP1,
  CHANGE_RFID_STEP2
};

Mode currentMode = NORMAL;
unsigned long lastLCDUpdate = 0;
unsigned long messageTimeout = 0;
String currentMessage = "";

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32 - Password & RFID Authentication System");
  
  // Initialize I2C and LCD displays
  Wire.begin();
  delay(100);
  
  lcd1.init();
  lcd1.backlight();
  lcd1.setCursor(0, 0);
  lcd1.print("System Init...");
  
  lcd2.init();
  lcd2.backlight();
  lcd2.setCursor(0, 0);
  lcd2.print("System Init...");
  
  delay(500);
  
  // Initialize SPI and RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID Reader initialized");
  
  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(DOOR_LOCK_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(DOOR_LOCK_PIN, HIGH);
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadPasswordFromEEPROM();
  loadRFIDFromEEPROM();
  
  Serial.print("Current Password: ");
  Serial.println(currentPassword);
  Serial.print("Authorized RFID: ");
  Serial.println(authorizedRFID);
  
  delay(500);
  
  lcd1.clear();
  lcd1.setCursor(0, 0);
  lcd1.print("System Ready!");
  lcd1.setCursor(0, 1);
  lcd1.print("Enter Password");
  
  lcd2.clear();
  lcd2.setCursor(0, 0);
  lcd2.print("Or Scan RFID");
  
  Serial.println("Setup Complete!");
}

// ============ MAIN LOOP ============
void loop() {
  // Check RFID card
  checkRFID();
  
  // Handle keypad input
  handleKeypad();
  
  // Update LCD Display
  unsigned long currentTime = millis();
  if (currentTime - lastLCDUpdate >= 500) {
    updateLCDDisplays();
    lastLCDUpdate = currentTime;
  }
  
  // Handle message timeout
  if (currentTime > messageTimeout && messageTimeout != 0) {
    if (currentMode == NORMAL) {
      resetDisplay();
    }
  }
  
  delay(10);
}

// ============ Check RFID Card ============
void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // Get card UID
  String cardID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardID += (rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    cardID += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) {
      cardID += " ";
    }
  }
  cardID.toUpperCase();
  
  Serial.print("Card UID: ");
  Serial.println(cardID);
  
  // Handle based on current mode
  if (currentMode == NORMAL) {
    // Check authentication
    if (cardID == authorizedRFID) {
      showResult("RFID OK!", "Access Granted");
      triggerBuzzer(200);
      digitalWrite(DOOR_LOCK_PIN, LOW);
      delay(3000);
      digitalWrite(DOOR_LOCK_PIN, HIGH);
      resetDisplay();
    } else {
      showResult("RFID FAIL!", "Card Not Auth");
      triggerBuzzer(1000);
      delay(2000);
      resetDisplay();
    }
  } 
  else if (currentMode == CHANGE_RFID_STEP2) {
    // New RFID card scanned for change
    authorizedRFID = cardID;
    saveRFIDToEEPROM();
    
    currentMode = NORMAL;
    showResult("RFID Changed!", "New Card Set");
    triggerBuzzer(300);
    delay(2000);
    resetDisplay();
    
    Serial.print("New RFID saved: ");
    Serial.println(authorizedRFID);
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ============ Handle Keypad Input ============
void handleKeypad() {
  char key = customKeypad.getKey();
  
  if (!key) {
    return;
  }
  
  Serial.println(key);
  
  // NORMAL MODE - Enter password or change mode
  if (currentMode == NORMAL) {
    if (key == 'A') {
      // Start change password mode
      currentMode = CHANGE_PASSWORD_STEP1;
      currentInput = "";
      currentMessage = "Enter Old Pwd:";
      messageTimeout = millis() + 30000;
      Serial.println("Change Password Mode");
    } 
    else if (key == 'B') {
      // Start change RFID mode
      currentMode = CHANGE_RFID_STEP1;
      currentInput = "";
      currentMessage = "Enter Pwd:";
      messageTimeout = millis() + 30000;
      Serial.println("Change RFID Mode");
    }
    else if (key == '#') {
      // Check password
      if (currentInput == currentPassword) {
        showResult("Password OK!", "Access Granted");
        triggerBuzzer(200);
        digitalWrite(DOOR_LOCK_PIN, LOW);
        delay(3000);
        digitalWrite(DOOR_LOCK_PIN, HIGH);
        currentInput = "";
        messageTimeout = millis() + 2000;
      } else {
        showResult("Password FAIL!", "Incorrect Pwd");
        triggerBuzzer(1000);
        currentInput = "";
        messageTimeout = millis() + 2000;
      }
    }
    else if (key == '*') {
      // Clear input
      currentInput = "";
    }
    else if (key >= '0' && key <= '9') {
      // Add digit to input
      if (currentInput.length() < 8) {
        currentInput += key;
      }
    }
  }
  
  // CHANGE PASSWORD - Step 1 (Verify old password)
  else if (currentMode == CHANGE_PASSWORD_STEP1) {
    if (key == '#') {
      if (currentInput == currentPassword) {
        currentMode = CHANGE_PASSWORD_STEP2;
        currentInput = "";
        currentMessage = "Enter New Pwd:";
        messageTimeout = millis() + 30000;
        Serial.println("Old password correct");
      } else {
        currentMode = NORMAL;
        showResult("Old Pwd FAIL!", "Incorrect");
        triggerBuzzer(1000);
        messageTimeout = millis() + 2000;
      }
    }
    else if (key == '*') {
      currentInput = "";
    }
    else if (key >= '0' && key <= '9') {
      if (currentInput.length() < 8) {
        currentInput += key;
      }
    }
  }
  
  // CHANGE PASSWORD - Step 2 (Enter new password)
  else if (currentMode == CHANGE_PASSWORD_STEP2) {
    if (key == '#') {
      if (currentInput.length() >= 4) {
        currentMode = CHANGE_PASSWORD_STEP3;
        String newPassword = currentInput;
        currentInput = "";
        currentMessage = "Confirm New Pwd:";
        messageTimeout = millis() + 30000;
        
        // Store temporarily
        EEPROM.writeString(PASSWORD_ADDR + 50, newPassword);
        
        Serial.print("New password entered: ");
        Serial.println(newPassword);
      } else {
        currentMode = NORMAL;
        showResult("Invalid Pwd!", "Min 4 chars");
        triggerBuzzer(1000);
        messageTimeout = millis() + 2000;
      }
    }
    else if (key == '*') {
      currentInput = "";
    }
    else if (key >= '0' && key <= '9') {
      if (currentInput.length() < 8) {
        currentInput += key;
      }
    }
  }
  
  // CHANGE PASSWORD - Step 3 (Confirm new password)
  else if (currentMode == CHANGE_PASSWORD_STEP3) {
    if (key == '#') {
      String tempPassword = EEPROM.readString(PASSWORD_ADDR + 50);
      if (currentInput == tempPassword) {
        currentPassword = tempPassword;
        savePasswordToEEPROM();
        
        currentMode = NORMAL;
        showResult("Pwd Changed!", "Success!");
        triggerBuzzer(300);
        currentInput = "";
        messageTimeout = millis() + 2000;
        
        Serial.print("Password changed to: ");
        Serial.println(currentPassword);
      } else {
        currentMode = NORMAL;
        showResult("Pwd NOT Match!", "Try Again");
        triggerBuzzer(1000);
        currentInput = "";
        messageTimeout = millis() + 2000;
      }
    }
    else if (key == '*') {
      currentInput = "";
    }
    else if (key >= '0' && key <= '9') {
      if (currentInput.length() < 8) {
        currentInput += key;
      }
    }
  }
  
  // CHANGE RFID - Step 1 (Verify password)
  else if (currentMode == CHANGE_RFID_STEP1) {
    if (key == '#') {
      if (currentInput == currentPassword) {
        currentMode = CHANGE_RFID_STEP2;
        currentInput = "";
        currentMessage = "Scan New Card:";
        messageTimeout = millis() + 30000;
        Serial.println("Password correct, waiting for RFID");
      } else {
        currentMode = NORMAL;
        showResult("Pwd FAIL!", "Incorrect");
        triggerBuzzer(1000);
        messageTimeout = millis() + 2000;
      }
    }
    else if (key == '*') {
      currentInput = "";
    }
    else if (key >= '0' && key <= '9') {
      if (currentInput.length() < 8) {
        currentInput += key;
      }
    }
  }
}

// ============ Show Result on LCD1 ============
void showResult(String line1, String line2) {
  lcd1.clear();
  lcd1.setCursor(0, 0);
  lcd1.print(line1);
  lcd1.setCursor(0, 1);
  lcd1.print(line2);
}

// ============ Reset Display ============
void resetDisplay() {
  currentMode = NORMAL;
  currentInput = "";
  currentMessage = "";
  messageTimeout = 0;
  
  lcd1.clear();
  lcd1.setCursor(0, 0);
  lcd1.print("System Ready");
  lcd1.setCursor(0, 1);
  lcd1.print("Enter Pwd or");
  
  lcd2.clear();
  lcd2.setCursor(0, 0);
  lcd2.print("Scan RFID");
  lcd2.setCursor(0, 1);
  lcd2.print("A=ChgPwd B=ChgRFID");
}

// ============ Update LCD Displays ============
void updateLCDDisplays() {
  // LCD2 - Show input
  lcd2.setCursor(0, 0);
  lcd2.print("                ");  // Clear
  lcd2.setCursor(0, 0);
  
  if (currentMessage.length() > 0) {
    lcd2.print(currentMessage);
  } else {
    lcd2.print("Enter Password:");
  }
  
  lcd2.setCursor(0, 1);
  lcd2.print("                ");  // Clear
  lcd2.setCursor(0, 1);
  
  // Show password as dots
  for (int i = 0; i < currentInput.length(); i++) {
    lcd2.print("*");
  }
}

// ============ Buzzer Function ============
void triggerBuzzer(unsigned long duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

// ============ Save Password to EEPROM ============
void savePasswordToEEPROM() {
  EEPROM.writeString(PASSWORD_ADDR, currentPassword);
  EEPROM.commit();
  Serial.println("Password saved to EEPROM");
}

// ============ Load Password from EEPROM ============
void loadPasswordFromEEPROM() {
  String pwd = EEPROM.readString(PASSWORD_ADDR);
  if (pwd.length() > 0 && pwd.length() <= 8) {
    currentPassword = pwd;
  }
}

// ============ Save RFID to EEPROM ============
void saveRFIDToEEPROM() {
  EEPROM.writeString(RFID_CARD_ADDR, authorizedRFID);
  EEPROM.commit();
  Serial.println("RFID saved to EEPROM");
}

// ============ Load RFID from EEPROM ============
void loadRFIDFromEEPROM() {
  String rfid = EEPROM.readString(RFID_CARD_ADDR);
  if (rfid.length() > 0) {
    authorizedRFID = rfid;
  }
}
