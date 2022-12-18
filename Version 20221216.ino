/*

Switch Antenna
F4IRX - 20/11/222
Version 0.1a - Beta
Version 20221203 - With Comment
Version 20221204 - If Debug + Press_Time
Version 20221205 - EEPROM
Version 20221206 - Default Radio in Memory ( EEPROM )
Version 20221206 - Default Sound in Memory ( EEPROM )
Version 20221212 - Modif EEPROM
Version 20221212 - Add Togle to see Active EXIT BNC
Version 20221215 - Add AP Mode 
Version 20221216 - WebServer -> Memory Setting

TODO 
====


Add WebServer for SmartPhone Remote
Tempo before switch Relay ?!? if error and store it to eeprom
Use CATS for Multitask




DESCRIPTION Fontionnality
=========================

Button_1
	Use for ON/OFF Screen, if Screen OFF then the LED_1 is Active , if Screen ON the  LED_1 is OFF
		Short Press => Screen OFF
		Long Press => Screen ON

Button_2
	Use for selecting the Radio (or Antenna) and Activate/desactivate the output
		Short Press	=> Selecting the Radio ( turn +1 )
		Long Press	=> Activate or Desactivate the output
	If Screen OFF, nothing append only Screen ON

Buzzer
	Only use for IR Remote Receive correct Code or Button activation (Short BIP "250")
	Long BIP for Error IR Receive CODE
	
LED_1
	Information for Switch "ON"
		ON is Screen OFF / OFF if Screen ON
LED_2
	Information for Statut of OUTPUT	
		ON 	= Active
		OFF = Not Active
LED_3
	Not Used
IR
	Receive IR Code from IR_Remote 
		Code 4127850240 	-> Play/Pause
		Code 3927310080	-> Next (If 8 then Rotate begin)
		Code 4161273600	-> Prev ( If 1 then Rotate end)
		Code 3125149440	-> ON/OFF
		Code 3091726080	-> Mute
		Code 4077715200	-> 1
		Code 3877175040	-> 2
		Code 2707357440	-> 3
		Code 4144561920	-> 4
		Code 3810328320	-> 5
		Code 2774204160	-> 6
		Code 3175284480	-> 7
		Code 2907897600	-> 8
		Code 3860463360	-> Vol+ ( Activate Sound )
		Code 3910598400	-> Vol- ( Mute Sound )
		Code 3041591040	-> 9
		Code 4061003520	-> 0
		Code 3208707840	-> Toggle
		Code 3141861120	-> Mode
	
LCD 16*2 	-> I2C Add :  0x27
PCF8574A	-> I2C Add : 0X38	
	
	
	
ESP32 DOIT DEVKIT V1 - https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2018/08/ESP32-DOIT-DEVKIT-V1-Board-Pinout-36-GPIOs-updated.jpg?quality=100&strip=all&ssl=1

BUZZER 		-> GPIO25
Button_1 	-> GPIO27
Button_2	-> GPIO26
IR			  -> GPIO19
I2C SDA		-> GPIO21
I2C SCL		-> GPIO22
LED_1		  -> GPIO12
LED_2	  	-> GPIO13
LED_3	  	-> GPIO14



EEPROM MAP
0-15      - Text Radio 1
16-31     - Text Radio 2
32-47     - Text Radio 3
47-63     - Text Radio 4
64-79     - Text Radio 5
80-95     - Text Radio 6
96-111    - Text Radio 7
112-127   - Text Radio 8
128       - Default Radio
129       - Default Sound 



*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp> // version 3.9.0
#include <EEPROM.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
 
#define Buzzer 25
#define Button_1 26  										                      // ON/OFF/STANDBY
#define Button_2 27  									                      	// ANTENNA
#define IR_RECEIVE_PIN 19
#define LED_1 12  											                      // ON/OFF/STANDBY
#define LED_2 13  											                      // Actif
#define LED_3 14  										                      	// LAN

#define DEBUG                                                 // Comment if not DEBUG MODE

#define PRESS_TIME  1000    								                  // 1000 milliseconds
#define EEPROM_SIZE 140                                       // See EPPROM MAP

// Variable

const char* ssid     = "SwitchAntenna";
const char* password = "Pass=F4IRX";

IPAddress local_ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";
const char* PARAM_INPUT_4 = "input4";
const char* PARAM_INPUT_5 = "input5";
const char* PARAM_INPUT_6 = "input6";
const char* PARAM_INPUT_7 = "input7";
const char* PARAM_INPUT_8 = "input8";
const char* PARAM_INT = "inputInt";
const char* PARAM_D = "inputD";

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <p><b>ESP32 - Switch Antenna Setting </b></p>
  <p><b>F4IRX</b></p>
  <p>Radio = 15 Car MAX</p>
  <br>
  <form action="/get">
    Radio Mem 1 <input type="text" name="input1">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Radio Mem 2 <input type="text" name="input2">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Radio Mem 3 <input type="text" name="input3">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Radio Mem 4 <input type="text" name="input4">
    <input type="submit" value="Submit">
  </form><br>  
  <form action="/get">
    Radio Mem 5 <input type="text" name="input5">
    <input type="submit" value="Submit">
  </form><br>  
  <form action="/get">
    Radio Mem 6 <input type="text" name="input6">
    <input type="submit" value="Submit">
  </form><br>  
  <form action="/get">
    Radio Mem 7 <input type="text" name="input7">
    <input type="submit" value="Submit">
  </form><br>  
  <form action="/get">
    Radio Mem 8 <input type="text" name="input8">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get" target="hidden-form">
    Flag Sound ( 1 or 0 ): <input type="number " name="inputInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Defaut radio ( 1 to 8 ): <input type="number " name="inputD">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>

  
</body></html>)rawliteral";



bool ShowExit                       = false;                  // for Exit BNC ( lcd print )
byte selectedMenuItem               = 8;                      // the selected menu (MAX 8 -> 8 Relay )
bool bStandByMode                   = true;                   // StandBy Screen 
bool bSound                         = true;                   // Sound
bool bActiveRadio                   = false;                  // Active Relay
byte lastStateButton_1              = LOW;                    // the previous state for Button_1 
byte currentStateButton_1;                                    // the current reading for Button_1
byte lastStateButton_2              = LOW;                    // the previous state for Button_2
byte currentStateButton_2;                                    // the current reading for Button_2 
unsigned long PressedTimeButton_1   = 0;                      // store the timepress for Button_1 (use for Short/Long Press)
unsigned long ReleasedTimeButton_1  = 0;                      // store the timerelease for Button_1 (use for Short/Long Press)
unsigned long PressedTimeButton_2   = 0;                      // store the timepress for Button_2 (use for Short/Long Press)
unsigned long ReleasedTimeButton_2  = 0;                      // store the timerelease for Button_2 (use for Short/Long Press)
byte LogoSound[8] = {                                         // LOGO for Sound CARACTERE
  B00001,
  B00011,
  B11111,
  B11111,
  B11111,
  B00011,
  B00001,
};
bool                          Mode_RX;                    // Le signal indiquant qu'une Cmd reçue est en voie d'être complétée
String                        String_Receive = "";               // La chaîne de caractères reçue du UART de l'ordi.
unsigned long int             Start_Time_RX;                 // La valeur conservée de millis pour fins de délai de réception d'une Cmd
char                          Char_Receive;                              // Le caractère reçu du UART de l'ordi.
unsigned short int            index1;                                   // La position du premier paramètre de la Cmd reçue
unsigned short int            index2;                                   // La position du deuxième paramètre de la Cmd reçue
String                        Param;                                   // Le premier paramètre de la Cmd reçue
String                        Cmd;                                 // Une Cmd reçue de l'ordi
//byte value;                                                         // use for verif EEPROM 

char HAM_RADIO[8][16] = { "  Yaesu FT857 "," Kenwood TS50 ","  Yaesu FT102 ","  SDR-PC Labo "," QCX+ QRPlabs ","  Balise WSPR ","   ATLAS 210x ","  HACK RF One "};

LiquidCrystal_I2C lcd(0x27, 16, 2);                           // LCD Definition 16Car 2Lig 0x27 = I2C ADD
// **********************************************************************************************
// ************************************     Function BIP     ************************************
// **********************************************************************************************
void Bip(int Duration) {                                      // Duration in ms
  if (bSound == true) {                                       // if flag = True -> Sound
    digitalWrite(Buzzer, HIGH);
    delay(Duration);
    digitalWrite(Buzzer, LOW);
  } 
}
// **********************************************************************************************
// ************************************     SwitchRelay      ************************************
// **********************************************************************************************
void SwitchRelay() {
  if (bActiveRadio == true) {                                 // if Flag = True -> LED_2 ON + write to PCF8574 with Tempo 
    digitalWrite(LED_2, HIGH);
    Wire.beginTransmission(0x38); // adresse sur 7 bits
    switch (selectedMenuItem ) {
      case 1:                                                 // Active Relay 1
        Wire.write(B11111110);
        break;
      case 2:                                                 // Active Relay 2
        Wire.write(B11111101);
        break;
      case 3:                                                 // Active Relay 3
        Wire.write(B11111011);
        break;
      case 4:                                                 // Active Relay 4  
        Wire.write(B11110111);
        break;
      case 5:                                                 // Active Relay 5
        Wire.write(B11101111);
        break;
      case 6:                                                 // Active Relay 6
        Wire.write(B11011111);
        break;
      case 7:                                                 // Active Relay 7
        Wire.write(B10111111);
        break;
      case 8:                                                 // Active Relay 8      
        Wire.write(B01111111);
        break;
      default:                                                // By Default no Relay ALL OFF       
        Wire.write(B11111111);
        break;
    }
    Wire.endTransmission();
    delay(50);                                                // Tempo between 2 State

  } else {                                                    // If Flag = False Switch Off all relays and led_2 = OFF
    digitalWrite(LED_2, LOW);
    Wire.beginTransmission(0x38);
    Wire.write(B11111111);
    Wire.endTransmission();    
  }
}
// **********************************************************************************************
// ************************************     displayMenu      ************************************
// **********************************************************************************************
void displayMenu() {
  lcd.setCursor(0, 0);                                        // Set LCD 0,0
  lcd.print(HAM_RADIO[selectedMenuItem - 1]);                 // Wirte the Menu selected
  lcd.setCursor(0, 1);                                        // 2eme Ligne
  if (bActiveRadio == true) {                                 // If Actived
      lcd.print("   [ Actif ]");
    } else {
      lcd.print("               ");
    }
  if (ShowExit== true) { 
      lcd.setCursor(0, 1); 
      lcd.print("S:"); 
      lcd.print(selectedMenuItem);
    }
  lcd.setCursor(15, 1);                                       // If sound , the Sound Caract
  if (bSound==true) {lcd.write(byte(0));} else {lcd.print(" "); }
  SwitchRelay();                                              // Switch RELAY after the Screen
}




// **********************************************************************************************
// ************************************     ResetEEPROM      ************************************
// **********************************************************************************************
void ResetEEPROM() {                                          // Clear the EEPROM
   for (int i = 0; i <= EEPROM_SIZE; i++) {
     EEPROM.writeInt(i, 0);            
    }
   EEPROM.commit();
   Serial.println("");
   Serial.println("##### RESET OK ######");
   ReadEEPROM(); 
}

void ReadEEPROM() {
  int index=0;
for (int j = 0; j < 8; ++j) {
  for (int k = 0; k < 15; ++k) {
      HAM_RADIO[j][k]=EEPROM.readUChar(k+index);
    }
  index  +=15;
  
}


}

void DumpEEPROM() {
  Serial.println("##### START EEPROM ######");
  for (int k = 0; k <= EEPROM_SIZE; ++k) {
      Serial.print(EEPROM.readUChar(k));
  }
  Serial.println("");
  Serial.println("##### END EEPROM ######");
}







// **********************************************************************************************
// ************************************        setup         ************************************
// **********************************************************************************************
void setup() {
  pinMode(Buzzer, OUTPUT);                                    // Pin Definition for Buzzer
  pinMode(LED_1, OUTPUT);                                     // Pin definition for LED_1
  pinMode(LED_2, OUTPUT);                                     // Pin definition for LED_2
  pinMode(LED_3, OUTPUT);                                     // Pin definition for LED_3    
  digitalWrite(LED_1, LOW);                                   // Put LED_1 OFF
  digitalWrite(LED_2, LOW);                                   // Put LED_2 OFF
  digitalWrite(LED_3, LOW);                                   // Put LED_3 OFF  
  digitalWrite(Buzzer, LOW);                                  // Put Buzzer OFF  
  pinMode(Button_1, INPUT);                                   // Pin definition for Button_1 ( Capacitance for Debounce )  
  pinMode(Button_2, INPUT);                                   // Pin definition for Button_2 ( Capacitance for Debounce ) 
  Serial.begin(9600);                                         // 9600 Baud    
  Serial.println(F("Switch Antenna Vesrion : " __VERSION__)); // Debug - Serial
  Serial.println(F("Built on " __DATE__ " at " __TIME__));    // Debug - Serial
  Wire.begin();
  Wire.beginTransmission(0x38);                               // Switch OFF all Relays
  Wire.write(B11111111);
  Wire.endTransmission();
  #ifdef DEBUG
      Serial.println(F("Relay Switch OFF"));                      // Debug - Serial
  #endif
  digitalWrite(LED_1, HIGH);                                      // Activate ALL LED for TEST POWER ON  
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, HIGH);
  #ifdef DEBUG
      Serial.println("ALL Led ON");                               // Debug - Serial
  #endif
  lcd.begin();                                    // Definition for LCD ( See LiquidCrystal_I2C.H)  
  lcd.createChar(0, LogoSound);                                   // Create CARACT for SOUND
  #ifdef DEBUG
      Serial.println(F("Caractere SOUND OK"));                    // Debug - Serial
  #endif
  lcd.backlight();                                                // Config for LCD
  lcd.setCursor(0, 0);                                            // Start LCD 
  lcd.print("Switch Antenna");
  lcd.setCursor(1, 1);
  lcd.print("F4IRX - " __VERSION__);
  #ifdef DEBUG
      Serial.println(F("Start LCD + Version"));                   // Debug - Serial
  #endif
  digitalWrite(LED_1, LOW);                                       // Switch LED OFF
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  #ifdef DEBUG
      Serial.println(F("ALL LED OFF"));                           // Debug - Serial
  #endif
IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Start the receiver  
 // irrecv.enableIRIn();                                            // Init IR
  #ifdef DEBUG
      Serial.println(F("IR ENEBALE"));                            // Debug - Serial
  #endif  
  if (!EEPROM.begin(EEPROM_SIZE)) {                               // Init EEPROM
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (EEPROM.read(128)!=0) {
        selectedMenuItem=EEPROM.readInt(128);                        // Test and affect default radio if it's store in memory
         #ifdef DEBUG
            Serial.print("Default Radio load : ");
            Serial.println(HAM_RADIO[selectedMenuItem]);
            Serial.println(selectedMenuItem);
         #endif
    } 
  if (EEPROM.read(129)!=0) {bSound=true;} else {bSound=false;}    // restore sound if in memory                                            
  if (EEPROM.read(0)!=0) ReadEEPROM();                                     // if something in EEPROM read EEPROM and Affect HAM_RADIO[]
  #ifdef DEBUG
      Serial.println("End of INIT EEPROM");
      Serial.println(F("END OF SETUP"));                          // Debug - Serial
  #endif
  delay(2000);                                                    // Delay for User hihihi
  Bip(50);                                                        // Little BIP ( for POWER ON ou Reset)
  displayMenu();   
  #ifdef DEBUG
      Serial.println(F("DisplayMenu OK"));                        // Debug - Serial
  #endif   
// WIFI  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.print("[+] AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

 // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage.toCharArray( HAM_RADIO[0],16);
      EEPROM.writeString(0,HAM_RADIO[0]);
      EEPROM.commit();
      ReadEEPROM();
      Serial.println("New Param on R1");
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      inputMessage.toCharArray( HAM_RADIO[1],16);
      EEPROM.writeString(15,HAM_RADIO[1]);
      EEPROM.commit();
      ReadEEPROM();
      Serial.println("New Param on R2");
    }
     // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      inputMessage.toCharArray( HAM_RADIO[2],16);
      EEPROM.writeString(30,HAM_RADIO[2]);
      EEPROM.commit();
      ReadEEPROM();
      Serial.println("New Param on R3");
    }
     // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = PARAM_INPUT_4;
      inputMessage.toCharArray( HAM_RADIO[3],16);
      EEPROM.writeString(45,HAM_RADIO[3]);
      EEPROM.commit();
      ReadEEPROM();
      Serial.println("New Param on R4");
    } 
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_5)) {
      inputMessage = request->getParam(PARAM_INPUT_5)->value();
      inputParam = PARAM_INPUT_5;
      inputMessage.toCharArray( HAM_RADIO[4],16);
      EEPROM.writeString(60,HAM_RADIO[4]);
      EEPROM.commit();
      ReadEEPROM();
      Serial.println("New Param on R5");      
    } // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_6)) {
      inputMessage = request->getParam(PARAM_INPUT_6)->value();
      inputParam = PARAM_INPUT_6;
      inputMessage.toCharArray( HAM_RADIO[5],16);
      EEPROM.writeString(75,HAM_RADIO[5]);
      EEPROM.commit();
      ReadEEPROM();
      Serial.println("New Param on R6");
    } // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_7)) {
      inputMessage = request->getParam(PARAM_INPUT_7)->value();
      inputParam = PARAM_INPUT_7;
      inputMessage.toCharArray( HAM_RADIO[6],16);
      EEPROM.writeString(90,HAM_RADIO[6]);
      EEPROM.commit();
      ReadEEPROM();
      Serial.println("New Param on R7");
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_8)) {
      inputMessage = request->getParam(PARAM_INPUT_8)->value();
      inputParam = PARAM_INPUT_8;
      inputMessage.toCharArray( HAM_RADIO[7],16);
      EEPROM.writeString(105,HAM_RADIO[7]);
      EEPROM.commit();
      ReadEEPROM();
      Serial.println("New Param on R8");
    }
    else if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      if (inputMessage=="1") {
                  EEPROM.writeInt(129, 1);
                  bSound=true;
                } else {
                  EEPROM.writeInt(129, 0);
                  bSound=false;
                }     
      EEPROM.commit();        
      ReadEEPROM();
      displayMenu();
    }
    else if (request->hasParam(PARAM_D)) {
      inputMessage = request->getParam(PARAM_D)->value();
      Serial.println("\nOK");
      EEPROM.writeInt(128, inputMessage.toInt());                              
      EEPROM.commit();
      ReadEEPROM();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to Switch Antenna MEMORY <br><a href=\"/\">Return to Home Page</a>");
  });

  server.onNotFound(notFound);
  server.begin();
 
}


// **********************************************************************************************
// ************************************        notFound      ************************************
// **********************************************************************************************
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// **********************************************************************************************
// ************************************         loop         ************************************
// **********************************************************************************************
void loop() {
  IR_Receive();                                                                                                       // Scan if IR code Receive
  ScanButton_1();                                                                                                     // Scan Button_1
  ScanButton_2();                                                                                                     // Scan Button_2
  if (bStandByMode==false) { digitalWrite(LED_3, !digitalRead(LED_3)); }                                               // Blink LED_3 if StandbyMode
  if (Serial.available() > 0)                                                                                         // Some Char on Serial PORT ?
    {
       Mode_RX = true;                                                                                                 // Start Listening a CMD and PARAM
      Start_Time_RX = millis();                                                                                       // Start Timing if too long CMD Canceled
      Char_Receive = Serial.read();                                                                                   // Read Char
      if (String_Receive == "") Serial.println();                                                                     // if First Char then CR/LF
      Serial.print(Char_Receive);                                               
      if ((Char_Receive == 8) || (Char_Receive == 127))  String_Receive.remove(String_Receive.length()-1);            // DEL or BACKSPACE
      else if ((Char_Receive != '\n') && (Char_Receive != '\r')) String_Receive += Char_Receive;                      // if not the END Add Char to String
      else                                                                                                            // CMD OK !
      {
        index1 = String_Receive.indexOf(' ');                                                                         //find location of first CMD
        Cmd = String_Receive.substring(0, index1);                                                                    // the CMD
        Cmd.toUpperCase();
        index2 = String_Receive.indexOf('.', index1+1 );                                                              //finds location of Param ,
        Param = String_Receive.substring(index1+1, index2);                                                           // Next PARAM 
        Param.toUpperCase();
        if (Cmd == "R1")                                                                                              // Command for Writing HAM_RADIO[0]
        { 
          Serial.println("\nOK");
          Param.toCharArray( HAM_RADIO[0],16);
          EEPROM.writeString(0,HAM_RADIO[0]);
          EEPROM.commit();
          ReadEEPROM();
        }
        else if (Cmd == "R2")                                                                                         // Command for Writing HAM_RADIO[1]
        { 
          Serial.println("\nOK");
          Param.toCharArray( HAM_RADIO[1],16);
          EEPROM.writeString(15,HAM_RADIO[1]);
          EEPROM.commit();
          ReadEEPROM();
        } 
        else if (Cmd == "R3")                                                                                         // Command for Writing HAM_RADIO[2]
        { 
          Serial.println("\nOK");
          Param.toCharArray( HAM_RADIO[2],16);
          EEPROM.writeString(30,HAM_RADIO[2]);
          EEPROM.commit();
          ReadEEPROM();
          }
       else if (Cmd == "R4")                                                                                          // Command for Writing HAM_RADIO[3]
        { 
          Serial.println("\nOK");
          Param.toCharArray( HAM_RADIO[3],16);
          EEPROM.writeString(45,HAM_RADIO[3]);
          EEPROM.commit();
          ReadEEPROM();
        } 
       else if (Cmd == "R5")                                                                                          // Command for Writing HAM_RADIO[4]
        { 
          Serial.println("\nOK");
          Param.toCharArray( HAM_RADIO[4],16);
          EEPROM.writeString(60,HAM_RADIO[4]);
          EEPROM.commit();
          ReadEEPROM();
        }         
       else if (Cmd == "R6")                                                                                          // Command for Writing HAM_RADIO[5]
        { 
          Serial.println("\nOK");
          Param.toCharArray( HAM_RADIO[5],16);
          EEPROM.writeString(75,HAM_RADIO[5]);
          EEPROM.commit();
          ReadEEPROM();
        } 
       else if (Cmd == "R7")                                                                                          // Command for Writing HAM_RADIO[6]
        { 
          Serial.println("\nOK");
          Param.toCharArray( HAM_RADIO[6],16);
          EEPROM.writeString(90,HAM_RADIO[6]);
          EEPROM.commit();
          ReadEEPROM();
        } 
       else if (Cmd == "R8")                                                                                          // Command for Writing HAM_RADIO[7]
        { 
          Serial.println("\nOK");
          Param.toCharArray( HAM_RADIO[7],16);
          EEPROM.writeString(105,HAM_RADIO[7]);
          EEPROM.commit();
          ReadEEPROM();
        }   
        else if (Cmd == "D")                                                                                          // Command for wiriting the default Radio in memory
        { 
          Serial.println("\nOK");
          EEPROM.writeInt(128, Param.toInt());                              
          EEPROM.commit();
          ReadEEPROM();
          
        }    
        else if (Cmd == "S")                                                                                          // Command for wiriting the default Sound in memory
        { 
          Serial.println("\nOK");
          if (Param=="1") {
                  EEPROM.writeInt(129, 1);
                  bSound=true;
                } else {
                  EEPROM.writeInt(129, 0);
                  bSound=false;
                }     
          EEPROM.commit();        
          ReadEEPROM();
          displayMenu();
        }              
        else if (Cmd == "LIST")                                                                                       // Command for listing the EEPROM
        { 
          Serial.println("\nOK");
          DumpEEPROM();
        }    
        else if (Cmd == "RESET")                                                                                      // Command for Cleari,g the EEPROM
        { 
          Serial.println("\nOK");                                                                                     // Reset the EEPROM and need a restart for default HAM_RADIO
          ResetEEPROM();
        }            
       else if ((Cmd == "HELP") || (Cmd == "?"))                                                                      // HELP
        {
          Serial.println("HELP");          
          Serial.println("Command: MUST BE TERMINATED BY . (dot) ");
          Serial.println("----------------------------------");
          Serial.println("R1 <Ham radio 1> (Max 16 car)");
          Serial.println("R2 <Ham radio 2> (Max 16 car)");
          Serial.println("R3 <Ham radio 3> (Max 16 car)");
          Serial.println("R4 <Ham radio 4> (Max 16 car)");
          Serial.println("R5 <Ham radio 5> (Max 16 car)");
          Serial.println("R6 <Ham radio 6> (Max 16 car)");
          Serial.println("R7 <Ham radio 7> (Max 16 car)");
          Serial.println("R8 <Ham radio 8> (Max 16 car)");
          Serial.println("LIST - Liste EEPROM ");
          Serial.println("RESET - Reset the EEPROM, Need Restart for loading the default PARAM");   
          Serial.println("D <N° Ham radio> (integer <= 8)   Exemple : D 1 for first Radio store in memory");   
          Serial.println("S <1 or 0> Save Sound in memory ");  
          Serial.println("----------------------------------");
        } 
       else Serial.println("\nERREUR: commande non reconnue");         
       String_Receive = "";
       Mode_RX = false;
       while (Serial.available()) Serial.read();
      }

    }
    if (Mode_RX && ((millis() - Start_Time_RX) >= 10000))                                                             // Time Expired, CMD Canceled !
    {
      Serial.println("\r\nCommande Annulée");                                         
      Mode_RX = false;                                                                                                // Change MODE RX to FALSE
    }

}
// **********************************************************************************************
// ************************************     ScanButton_2     ************************************
// **********************************************************************************************
void ScanButton_2() {
  currentStateButton_2 = digitalRead(Button_2);                       // read the state of Button_2
  if(lastStateButton_2 == HIGH && currentStateButton_2 == LOW)        // Button_2 is pressed
    { 
      PressedTimeButton_2 = millis();                                 // Store the pressed time  
    }
  else if(lastStateButton_2 == LOW && currentStateButton_2 == HIGH) { // Button_2 is released
    ReleasedTimeButton_2 = millis();                                  // Store the release time
    long pressDuration2 = ReleasedTimeButton_2 - PressedTimeButton_2; // Short or Long ?
    if( pressDuration2 <= PRESS_TIME ) {                              // short ! 
      #ifdef DEBUG
            Serial.println(F("BUTTON 2 A short press is detected"));        // Debug - Serial
      #endif
      Bip(250);                                                       // BIP for 250ms
      if (bActiveRadio==false) {                                      // if relay NOT activated then turn to next menu 
            selectedMenuItem++;
            if (selectedMenuItem > 8) selectedMenuItem = 1;           // Rotate Menu
           }
      bActiveRadio = false;                                           // if Relay Activated -> Switch RELAY OFF in next displayMenu 
    }
    if( pressDuration2 > PRESS_TIME ) {                               // Long !
      #ifdef DEBUG
            Serial.println(F("BUTTON 2 A long press is detected"));         // Debug - Serial
	    #endif
      Bip(250);                                                       // Bip pour 250ms
      if(bStandByMode==false ) {Bip(250);} else {bActiveRadio = !bActiveRadio;}  // if StandbyMode : Bip NOT allow if NOT the TOGGLE
    }
    StandByOFF();                                                     // Turn ON Screen
    displayMenu();                                                    // DisplayMenu and Switch Relay
  }
  delay(50);
  lastStateButton_2 = currentStateButton_2;                           // save the the last state for Button_2
}
// **********************************************************************************************
// ************************************     ScanButton_1     ************************************
// **********************************************************************************************
void ScanButton_1() {                                                 // StandBy = Screen OFF but Relay Activation continue
  currentStateButton_1 = digitalRead(Button_1);
  if(lastStateButton_1 == HIGH && currentStateButton_1 == LOW)        // button is pressed
    PressedTimeButton_1 = millis();                                   // Store the pressed time
  else if(lastStateButton_1 == LOW && currentStateButton_1 == HIGH) { // button is released
    ReleasedTimeButton_1 = millis();                                  // Store the release time
    long pressDuration1 = ReleasedTimeButton_1 - PressedTimeButton_1; // Short Long ?
    if( pressDuration1 <= PRESS_TIME ) {                              // Short !
	    Bip(250);
      StandByOFF();
      #ifdef DEBUG
          Serial.println(F("B1_A short press is detected"));
      #endif
    }
    if( pressDuration1 > PRESS_TIME ) {                                // Long !
    #ifdef DEBUG
      Serial.println(F("B1_A long press is detected"));
    #endif
	    Bip(250);
      StandByON();
    }
  }
  delay(50);
  lastStateButton_1 = currentStateButton_1;                           // save the the last state for Button_1
}
// **********************************************************************************************
// ************************************      StandByOFF      ************************************
// **********************************************************************************************
void StandByOFF() {                                                   // Put the Screen ON
      lcd.display();
      lcd.backlight(); 
      digitalWrite(LED_3,LOW);                                        // on fixe a eteint la led veille 
}
// **********************************************************************************************
// ************************************      StandByON       ************************************
// **********************************************************************************************
void StandByON() {                                                    // Put the Screen OFF 
       lcd.noDisplay();
       lcd.noBacklight();
}
// **********************************************************************************************
// ************************************      IR_Receive      ************************************
// **********************************************************************************************
void IR_Receive() {   
 /* 
   if (IrReceiver.decode()) {
      Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
      IrReceiver.printIRResultShort(&Serial); // optional use new print version
      ...
      IrReceiver.resume(); // Enable receiving of the next value
  */
  
                                                  // Receive IR Code
  bool BadIRCode= false;                                              // Use for Bad IR Code
  if (IrReceiver.decode()) {                                      // Decode IR Code if receive ( see IRRemote.h )
    lcd.setCursor(15, 0);                                             // '*' on screen if receive IR Code ( Good or Bad)
    lcd.print("*");
    #ifdef DEBUG
        Serial.print(F("Code IR:"));                                      // Debug - Serial
        Serial.println(IrReceiver.decodedIRData.decodedRawData, DEC);
    #endif
    switch (IrReceiver.decodedIRData.decodedRawData) {                                          // IR Code Value 
      case 4127850240:
          if(bStandByMode==false ) {
          Bip(250);} else {
          bActiveRadio = !bActiveRadio; }
          break;
      case 3927310080:                                                  // Next
          bActiveRadio = false;
          selectedMenuItem++;
          if (selectedMenuItem > 8) selectedMenuItem = 1;
          break;
 
      case 4161273600:                                                 // Prev
          bActiveRadio = false;
          selectedMenuItem--;
          if (selectedMenuItem < 1) selectedMenuItem = 8;
          break;
      case 3125149440:                                                  // StandBY/Power
          bStandByMode=!bStandByMode;
           if (bStandByMode==false){StandByON();} else{ StandByOFF(); }
          break;
      case 3091726080:                                                  // Mute
          bSound = false;
          break;
      case 4077715200:                                                  // code 1
           selectedMenuItem=2;                                        // use 2 in displaymenu selectedMenuItem=selectedMenuItem-1
          break;
      case 3877175040:                                                  // code 2
           selectedMenuItem=3;      
          break;
      case 2707357440:                                                  // code 3
          selectedMenuItem=4;
          break;
      case 4144561920:                                                  // code 4
          selectedMenuItem=5;
          break;
      case 3810328320:                                                  // code 5
          selectedMenuItem=6;
          break;
      case 2774204160:                                                  // code 6
          selectedMenuItem=7;
          break;
      case 3175284480:                                                  // code 7
          selectedMenuItem=8;
          break;
      case 2907897600:                                                  // code 8
          selectedMenuItem=9;
          break;
      case 3860463360:                                                  // v+
          bSound = true;
          break;
      case 3910598400:                                                  // v-
          bSound = false;
          break;
      case 3041591040:                                                  // code 9
          Bip(250);
          break;
      case 4061003520:                                                  // code 0
          Bip(250);
          break;
      case 3208707840:                                                  // code toggle
          ShowExit=!ShowExit;    
          Bip(250);
          break;
      case 3141861120:                                                  // code mode
          Bip(250);
          break;
      default:
        BadIRCode=true;                                               // use for bad code
        break;
    }
   //                                                    // HAS TO BE HERE AFTER PLAY BUTTON AND  BEFORE STANDBYON
    if(BadIRCode==false) {
        Bip(100);
        displayMenu();
    }
    if (bSound==true) {delay(50);} else {delay(170);}                 // timing for "*" screen
    lcd.setCursor(15, 0);                                             // Clear '*'
    lcd.print(" ");
    IrReceiver.resume();
  }
}
