// NodeMCU ESP8266 Library
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// SPI and MFRC522
#include <SPI.h>
#include <MFRC522.h>

#define D0 D0
#define RST_PIN D1                    //--> RST is connected to pinout D1
#define SS_PIN D2                     //--> SDA / SS is connected to pinout D2
#define BOARD_LED 2
#define WIFI_CONNECTING D3
#define WIFI_CONNECTED D4
#define D5 D5
#define D6 D6
#define D7 D7
#define D8 D8

MFRC522 mfrc522(SS_PIN, RST_PIN);     //--> Create MFRC522 instance.
ESP8266WebServer server(8080);        //--> Server on port 8080
WiFiClient client;

int readsuccess;
byte readcard[4];
char str[32] = "";
String StrUID;

const char *ssid = "Coding";
const char *password = "Password123@#";
const char *host = "http://192.168.100.7";
const int port = 8000;

unsigned long epochTime; 

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  delay(500);

  pinMode(WIFI_CONNECTED, OUTPUT);
  pinMode(WIFI_CONNECTING, OUTPUT);

  delay(5000);
  WiFi.mode(WIFI_OFF); 
  delay(5000);
  WiFi.mode(WIFI_STA); //WiFi Station Mode default mode is both station and acccess point modes

  WiFi.begin(ssid, password); //Connecting to router
  Serial.println("");
  Serial.print("Connecting");
  // Check connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(WIFI_CONNECTING, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);               // wait for a second
    digitalWrite(WIFI_CONNECTING, LOW);    // turn the LED off by making the voltage LOW
    delay(200);               // wait for a second
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Please tag a card or keychain to see the UID !");
  Serial.println("");

  digitalWrite(WIFI_CONNECTED, HIGH);
}

/**
* Arduino / NodeMCU Loop Function
*/
void loop() {
  // put your main code here, to run repeatedly
  readsuccess = getid();

  if(readsuccess) {
    digitalWrite(BOARD_LED, LOW);

    HTTPClient http;
    String UIDresultSend;
    UIDresultSend = StrUID;

    http.begin(client, "http://192.168.100.7:8000/api/nodemcu");  //Specify request destination
    http.addHeader("Content-Type", "application/json"); //Specify content-type header

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["uid"] = UIDresultSend;
    root["type"] = "ATTEND";
    root["unixtime"] = getTime();
    root.printTo(Serial);
    char json_str[100];
    root.prettyPrintTo(json_str, sizeof(json_str));

    int httpResponseCode = http.POST(json_str);   //Send the request

    if (httpResponseCode > 0) {
      String response = http.getString();    //Get the response payload

      // Bad Requests
      if (httpResponseCode >= 400 && httpResponseCode <= 499) {
        Serial.print("Bad Request: ");
        Serial.println(httpResponseCode);
      }

      // OK
      if (httpResponseCode >= 200 && httpResponseCode <= 299) {
        Serial.println(UIDresultSend);
        Serial.println(httpResponseCode);   //Print HTTP return code
        Serial.print("OK: ");
        Serial.println(response);
      }

    } else {
      Serial.println("unknown error!");
    }

    http.end();  //Close connection
    delay(1000);
    digitalWrite(BOARD_LED, HIGH);
  }
}

/**
* Procedure for reading and obtaining a UID from a card or keychain
*/
int getid() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }

  Serial.print("THE UID OF THE SCANNED CARD IS : ");

  for (int i = 0; i < 4; i++) {
    readcard[i] = mfrc522.uid.uidByte[i]; //storing the UID of the tag in readcard
    array_to_string(readcard, 4, str);
    StrUID = str;
  }
  mfrc522.PICC_HaltA();
  return 1;
}

/**
* Procedure to change the result of reading an array UID into a string
*/
void array_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}
