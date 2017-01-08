


#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices
#include <Servo.h>

#define COMMON_ANODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#endif

#define StatusLock redLed
#define StatusUnlock greenLed


#define redLed 5    // Set Led Pins
#define greenLed 6
#define blueLed 7

#define buzzer A0

String Tristan = "22811285236";
String Cynthia;
String Felix;
String Fred;

byte Status  = (StatusLock);
int isLocked = 1;

//#define wipeB 0     // Button pin for WipeMode


byte storedCard[4];    // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM


/*  =+=+=+=  Const. for stepper  =+=+=+=  */
const int IN1=2;
const int IN2=3;
const int IN3=4;
const int IN4=8;
const int completeLap = 510/4;
const int rotationSpeed = 1;
const char tab1[] =
{
  0x01, 0x03, 0x02, 0x06, 0x04, 0x0c, 0x08, 0x09
};
const char tab2[] =
{
  0x01, 0x09, 0x08, 0x0c, 0x04, 0x06, 0x02, 0x03
};
/*  =+=+=+= (end) for stepper  =+=+=+=  */



/*
  We need to define MFRC522's pins and create instance
  Pin layout should be as follows (on Arduino Uno):
  MOSI: Pin 11 / ICSP-4
  MISO: Pin 12 / ICSP-1
  SCK : Pin 13 / ICSP-3
  SS : Pin 10 (Configurable)
  RST : Pin 9 (Configurable)
  look MFRC522 Library for
  other Arduinos' pin configuration
*/

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.



void setup() {

  //led 
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  // pinMode(wipeB, INPUT_PULLUP);    // Enable pin's pull up resistor
  //Be careful how relay circuit behave on while resetting or power-cycling your Arduino
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off
  //Stepper motor
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(buzzer,OUTPUT);

  //Protocol Configuration
  Serial.begin(9600);   // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware

  //Set gain to increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  //cleaning EEPROM
  Serial.println(F("Starting Wiping EEPROM"));
  for (int x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
    if (EEPROM.read(x) == 0) {              //If EEPROM address 0
    // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
    }else {
      EEPROM.write(x, 0);       // if not write 0 to clear, it takes 3.3mS
     }
  }
  Serial.println(F("EEPROM Successfully Wiped"));
  digitalWrite(redLed, LED_OFF);  // visualize successful wipe
  delay(100);
  for(int i = 0; i < 4; i = i + 1){
      digitalWrite(redLed, LED_ON);
      delay(100);
      digitalWrite(redLed, LED_OFF);
      delay(100);
  }
  delay(700);
  
  //unlock & lock the door
  Serial.println("");
  Serial.println("Unlocking the door...");
  unlock();
  Serial.println("Locking the door...");
  lock();
  //ready
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  Serial.println(F("Waiting PICCs to be scanned"));
  
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off
  delay(700);

}

void loop() {
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off

  int temp;
  //light breating
  int fadeAmount = -2;
  int brightness = 250;
  delay(350);

  while (temp != 1){
    temp = getID();
    analogWrite(Status, brightness);

    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;
  
    // reverse the direction of the fading at the ends of the fade:
    if (brightness < 5 || brightness > 250) {
      fadeAmount = -fadeAmount;
    }
    // wait for 10 milliseconds to see the dimming effect
    delay(10);
  }

  if (Status == StatusLock){
    unlock();
  } else {
    lock();
  }
  tone(buzzer,1300,150);
  delay(150);
  tone(buzzer,1300,150);
  
  if (isLocked){
    Status = StatusLock;
  } else {
    Status = StatusUnlock;
  }
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off
  for(int i = 0; i < 4; i = i + 1){
      digitalWrite(Status, LED_ON);
      delay(100);
      digitalWrite(Status, LED_OFF);
      delay(100);
  }  
}

int getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  String cardRead;
  for (int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
    cardRead.concat(readCard[i]);
  }
  if ((cardRead) != Tristan){
    Serial.println(F("Access DENIED"));
    tone(buzzer,400,1000);
    return 0;
   } else {
    tone(buzzer,1200,150);

    Serial.println("");
    Serial.println(F("Hello Tristan"));
   }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void lock(){
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off
  if (!isLocked){
    isLocked = 1;
  }
  ctlStepMotor(completeLap, rotationSpeed);
  StepMotorStop();
  delay(100);
  
  return;
}

void unlock(){
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off
  if (isLocked){
    isLocked = 0;
  }
  ctlStepMotor(-completeLap, rotationSpeed);
  StepMotorStop();
  delay(100);
  return;
}

void ctlStepMotor(int angle, char speeds ){
  for (int j = 0; j < abs(angle) ; j++)
  {
    if (angle > 0)
    {
      for (int i = 0; i < 8; i++)
      {
        digitalWrite(IN1, ((tab1[i] & 0x01) == 0x01 ? true : false));
        digitalWrite(IN2, ((tab1[i] & 0x02) == 0x02 ? true : false));
        digitalWrite(IN3, ((tab1[i] & 0x04) == 0x04 ? true : false));
        digitalWrite(IN4, ((tab1[i] & 0x08) == 0x08 ? true : false));
        delay(1);
      }
    }
    else
    {
      for (int i = 0; i < 8 ; i++)
      {
        digitalWrite(IN1, ((tab2[i] & 0x01) == 0x01 ? true : false));
        digitalWrite(IN2, ((tab2[i] & 0x02) == 0x02 ? true : false));
        digitalWrite(IN3, ((tab2[i] & 0x04) == 0x04 ? true : false));
        digitalWrite(IN4, ((tab2[i] & 0x08) == 0x08 ? true : false));
        delay(1);
      }
    }
  }
}

void StepMotorStop(){
  digitalWrite(IN1, 0);
  digitalWrite(IN2, 0);
  digitalWrite(IN3, 0);
  digitalWrite(IN4, 0);
}
