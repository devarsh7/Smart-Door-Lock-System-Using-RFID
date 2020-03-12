

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <FirebaseArduino.h>

// PIN Numbers : RESET + SDAs
#define RST_PIN         0
#define SS_1_PIN        2
//#define SS_2_PIN        8
//#define SS_3_PIN        7
//#define SS_4_PIN        6
#define FIREBASE_HOST "rfid-scanner-9e653.firebaseio.com"
#define FIREBASE_AUTH "PAtyQkRNg6xpQn93AzMwdl1lxPcb6JqziWvb2VS6"
#define WIFI_SSID "NotSecured"
#define WIFI_PASSWORD "30032812"

// Led and Relay PINS
/*#define GreenLed        2
#define relayIN         3
#define RedLed          4*/

const String card1 = "45 7F 4A C5";
const String card2 = "3C CA DA 73";
// List of Tags UIDs that are allowed to open the puzzle
byte tagarray[][4] = {
  {0x4B, 0x17, 0xBC, 0x79},
  {0x8A, 0x2B, 0xBC, 0x79}, 
  {0x81, 0x29, 0xBC, 0x79},
  {0x45, 0x7F, 0x4A, 0xC5},
};

// Inlocking status :
int tagcount = 0;
bool access = false;

#define NR_OF_READERS   1

byte ssPins[] = {SS_1_PIN};

// Create an MFRC522 instance :
MFRC522 mfrc522[NR_OF_READERS];

/**
   Initialize.
*/
void setup() {
   String str = "WELCOME to the Advanced Door Lock System";
  Serial.println(str);

  Serial.begin(115200);           // Initialize serial communications with the PC
 // Serial.println("WELCOME to the Advanced Door Lock System");
 // Serial.println("Now place your card to enter:");
  
  while (!Serial);              // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  SPI.begin();                  // Init SPI bus

  /* Initializing Inputs and Outputs */
/*  pinMode(GreenLed, OUTPUT);
  digitalWrite(GreenLed, LOW);
  pinMode(relayIN, OUTPUT);
  digitalWrite(relayIN, HIGH);
  pinMode(RedLed, OUTPUT);
  digitalWrite(RedLed, LOW);*/

  /* looking for MFRC522 readers */
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
    //mfrc522[reader].PCD_SetAntennaGain(mfrc522[reader].RxGain_max);
   
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  }
}

/*
   Main loop.
*/

void loop() {

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {

    // Looking for new cards
    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
      Serial.print(F("Reader "));
      Serial.print(reader);

      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F(": Card ID:"));
      unsigned char* b1= mfrc522[reader].uid.uidByte;
      dump_byte_array(b1, mfrc522[reader].uid.size);
      Serial.println();

      for (int x = 0; x < sizeof(tagarray); x++)                  // tagarray's row
      {
        for (int i = 0; i < mfrc522[reader].uid.size; i++)        //tagarray's columns
        {
          if ( mfrc522[reader].uid.uidByte[i] != tagarray[x][i])  //Comparing the UID in the buffer to the UID in the tag array.
          {
            DenyingTag();
            break;
          }
          else
          {
            if (i == mfrc522[reader].uid.size - 1)                // Test if we browesed the whole UID.
            {
              AllowTag();
            }
            else
            {
              continue;                                           // We still didn't reach the last cell/column : continue testing!
            }
          }
        }
        if (access) break;                                        // If the Tag is allowed, quit the test.
      }


      if (access)
      {
        if (tagcount == NR_OF_READERS)
        {
          OpenDoor();
             // set value
            
            Firebase.pushString("Card ID",card1);
             // handle error
              if (Firebase.failed()) {
              Serial.print("setting /number failed:");
              Serial.println(Firebase.error());  
              return;
              }
  delay(1000);
        }
        else
        {
          MoreTagsNeeded();
        }
      }
      else
      {
        UnknownTag();
        // set value
            Firebase.pushString("Card ID", card2 );
             // handle error
              if (Firebase.failed()) {
              Serial.print("setting /number failed:");
              Serial.println(Firebase.error());  
              return;
              }
      }
      /*Serial.print(F("PICC type: "));
        MFRC522::PICC_Type piccType = mfrc522[reader].PICC_GetType(mfrc522[reader].uid.sak);
        Serial.println(mfrc522[reader].PICC_GetTypeName(piccType));*/
      // Halt PICC
      mfrc522[reader].PICC_HaltA();
      // Stop encryption on PCD
      mfrc522[reader].PCD_StopCrypto1();
    } //if (mfrc522[reader].PICC_IsNewC..
  } //for(uint8_t reader..
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printTagcount() {
  Serial.print("Tag no. ");
  Serial.println(tagcount);
}

void DenyingTag()
{
  tagcount = tagcount ;
  access = false;
}

void AllowTag()
{
  tagcount = tagcount + 1;
  access = true;
}

void Initialize()
{
  tagcount = 0;
  access = false;
}

void OpenDoor()
{
  Serial.println("Welcome! the door is now open");
  Initialize();
 // digitalWrite(relayIN, LOW);
//  digitalWrite(GreenLed, HIGH);
//  delay(2000);
//  digitalWrite(relayIN, HIGH);
//  delay(500);
//  digitalWrite(GreenLed, LOW);
Firebase.pushString("Message", "Welcome! The door is now open");
  // handle error
  if (Firebase.failed()) {
      Serial.print("setting /message failed:");
      Serial.println(Firebase.error());  
      return;
  }
}

void MoreTagsNeeded()
{
//  printTagcount();
  Serial.println("System needs more cards");
//  digitalWrite(RedLed, HIGH);
//  delay(1000);
//  digitalWrite(RedLed, LOW);
//  access = false;
}

void UnknownTag()
{
  Serial.println("This Tag isn't allowed!");
//  printTagcount();
  for (int flash = 0; flash < 5; flash++)
  {/*
    digitalWrite(RedLed, HIGH);
    delay(100);
    digitalWrite(RedLed, LOW);
    delay(100);*/
  }
  Firebase.pushString("Message", "This Tag isn't allowed");
  // handle error
  if (Firebase.failed()) {
      Serial.print("setting /message failed:");
      Serial.println(Firebase.error());  
      return;
  }
}
