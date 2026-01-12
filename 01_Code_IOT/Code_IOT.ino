/*

IOT Smart Home Arduino Projekt Exponatwand 3 Discover Industry
Code by Marco Umstätter 2025


Versionenverzeichnis:
  Bei verwendung aktuellerer Versionen kann es beim Upload des Codes zu Problemen komme.

  esp32 von Espressif v 1.0.6


*/
//Implementierungen (Bibliotheken und zuordnung der Pins)

//Arduino Basis funktionen
#include <Arduino.h>

//Auto Reset Counter
int resetCounter = 500;
int originalResetCount = 1000;

//IR Sensor
#define pyroelectric 14  //Pin belegung

//Buzzer
#include <ESP32Tone.h>
#define buzzer_pin 25  //Pin belegung

//Button
#define btn1 16  //Lichtschalter //Pin belegung
bool bellLocker = false;
#define btn2 27  //Türschließer/Klingel Button //Pin belegung

//LED (Einfarbig)
#define led_y 12  //Pin belegung
bool ledOn = 0;   //LED Status anzeige (o=aus/1=an)

//Servo Fenster
#include <ESP32_Servo.h>
Servo myservo;  //erstellung eines Servo Objekts
//#define servoPin 2 //Pin belegung
//Servo nicht richtig über Winkel Steuerbar. Daher über Pulsweitenmodulation = PWM
int channel_PWM = 13;     //PWM Kanal
int freq_PWM = 50;        //Puls Frequenz
int resolution_PWM = 10;  //10 bit auflösung
const int PWM_Pin1 = 2;   // Pin belegung
bool opnW = 1;            //Status Fenster (1=Offen/0 = zu)

//Servo Tür
Servo myservoD;  //erstellung eines Servo Objekts
// #define servoPinD 13 //Pin belegung
bool opnD = 1;             //Status Tür (1=Offen/0 = zu)
int channel_PWM2 = 14;     //PWM Kanal
int freq_PWM2 = 50;        //Puls Frequenz
int resolution_PWM2 = 10;  //10 bit auflösung
const int PWM_Pin2 = 13;   // Pin belegung

//Regensensor
#define waterPin 34        //Pin belegung
int gwRegen = 3000;        //Grenzwert bei dem der Regensensor auslöst
int initialgwR = gwRegen;  // zum rurücksetzen siehe Reset Temp

//LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C mylcd(0x27, 16, 2);  //(0x27= I2C Kanal, 16 = Zeichen Plätze im LCD, 2= reihen am Display)

//Temperatur und Feuchte Sensor
#include "xht11.h"                      //Bibliothek
xht11 xht(17);                          //Pin belegung
unsigned char dht[4] = { 0, 0, 0, 0 };  //array mit Messdaten dht[0] = LF, dht[1] = LF(dezimal), dht[2] = temp, dht[3] = temp (dezimal)
bool tempOn = 0;                        //Variable legt fest ob Temperatur am Display angezeigt wird oder nicht (s. Startbildschirm)
int temp = 26;                          //Grenzwert für Aktivierung der Lüftung kann über drücken der beiden Knöpfe auf die aktuelle Temperatur gesetzt werden

//Ventilator
const int fanPWMPin = 18;   // z. B. GPIO18
const int fanChannel = 5;   // PWM-Kanal 5
const int freq = 5000;      // 5 kHz PWM-Frequenz
const int resolution = 10;  // 10 Bit: Wertebereich 0–1023
bool fanOn = 0;             // Ventilator Status (o=aus/1=an)



//Multicolor LED
#include <Adafruit_NeoPixel.h>
#define LED_PIN 26                                                  //Pin belegung
#define LED_COUNT 4                                                 //Anlazhl an LEDs
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);  //Initialisierung des LED Strips

//NFC Sensor
#include "MFRC522_I2C.h"
MFRC522 mfrc522(0x28);       //I2C Kanal belegung
String password_card = "";   // Variable die beim auslesen der Karte befüllt wird
String pwD = "1432207672";   //chip Haus 1
String pwD2 = "2622449181";  //chip Haus 2
String pwD3 = "2461106761";  //Karte Haus 2
String pwD4 = "164232118185"; // Karte Haus 1
//Password mit dem die tür aufgeht (muss dem der aktuellen karte entsprechen) Solte Code nicht bekannt sein, einfach Seriellen Monitor am PC bei angeschlossenem Haus auslesen. wird beim Dranhalten ausgespuckt



//WiFi
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
String item = "0";
const char* ssid = "SmartHome";          //Name des Hotspots
const char* password = "88888888";       //Password des Hotspots
const char* ssid2 = "iPhone von Marco";  //Name des Hotspots
const char* password2 = "12348765";      //Password des Hotspots
WiFiServer server(80);



//Initialisierung
void setup() {
  Serial.begin(115200);  //Datenübertragungsrate an Seriellen Monitor

  //Pins die auf input warten:
  pinMode(pyroelectric, INPUT);
  pinMode(btn1, INPUT);
  pinMode(btn2, INPUT);
  pinMode(waterPin, INPUT);

  //Pins die auf output warten:
  pinMode(led_y, OUTPUT);
  pinMode(buzzer_pin, OUTPUT);


  //Ventilator
  ledcSetup(fanChannel, freq, resolution);
  ledcAttachPin(fanPWMPin, fanChannel);


  //LCD
  mylcd.backlight();      //aktiviert das LCD Backlight (Dimmen nicht möglich)
  mylcd.init();           //Initialisiert des LCDs
  mylcd.setCursor(0, 0);  //Courser am LCD auf 1. Position in 1. Reihe

  //LED Aus
  digitalWrite(led_y, LOW);


  //Multicolor LED
  strip.begin();            // Initialisiert LED Strip
  strip.show();             // Alle LEDs aus
  strip.setBrightness(50);  // Helligkeit gesetzt auf 50 (max = 255)

  //NFC
  Wire.begin();        // initialisiere NFC I2C Kanal
  mfrc522.PCD_Init();  // initialisiere NFC Sensor


  //Servo Fenster
  ledcSetup(channel_PWM, freq_PWM, resolution_PWM);  //Werte der Variablen werden nun im Setup verwendet
  ledcAttachPin(PWM_Pin1, channel_PWM);              //verbindet den LEDC Kanal zum IO port (output)
  ledcWrite(channel_PWM, 26);                        //26 entspr. der Offenen fenster stellung und 65 entsprechen dem geschlossenen Fenster. Diese Werte sollte auch nicht verändert werden, sofern sich nicht baulich etwas am haus ändert

  //Servo Tür
  ledcSetup(channel_PWM2, freq_PWM2, resolution_PWM2);  //Werte der Variablen werden nun im Setup verwendet
  ledcAttachPin(PWM_Pin2, channel_PWM2);                //verbindet den LEDC Kanal zum IO port (output)
  ledcWrite(channel_PWM2, 26);                          //26 entspr. der Offenen tür stellung und 65 entsprechen dem geschlossenen tür. Diese Werte sollte auch nicht verändert werden, sofern sich nicht baulich etwas am haus ändert

  //Reset des Regensensors und des Temperatur Grenzwertes
  xht.receive(dht);
  mylcd.clear();
  mylcd.setCursor(0, 0);
  mylcd.print("ResetT= ");
  mylcd.print(dht[2]);
  mylcd.setCursor(0, 1);
  mylcd.print("GWRegen=");
  delay(2000);
  int regen_val = analogRead(waterPin);
  gwRegen = initialgwR + regen_val;
  mylcd.print(gwRegen);
  temp = dht[2];  //Variable wird mit aktueller Temperatur überschrieben
  delay(1000);
  mylcd.clear();
  mylcd.setCursor(0, 0);

  //Verbindung WiFi
  int timeout = 30;  // Timeout auf 30 zyklen à 500 mil.sek. = 15 Sekunden verbinungsaufau
  int dotCount = 0;  // Zähler für die Punkte

  int runNR = 1;  // Durchlauf zähler für 2. SSID

  //Wenn das WLAN nicht verbunden ist, wird in der oberen zeile Verbindung... angezeigt und in der unteren eine art ladebalken mit Punkten. dieser wird gelöscht, sobald 16 Punkte im Display angezeigt werden.
  while (WiFi.status() != WL_CONNECTED) {
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Suche:");
    mylcd.setCursor(0, 1);
    mylcd.print(ssid);
    delay(500);

    mylcd.clear();

    mylcd.print("Verbindung...");  // Anzeige: Verbindung wird hergestellt
    delay(200);                    //verzögerung. manchmal nötig um nicht sofort weiter zu machen.
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(500);             // halbe Sekunde warten
      dotCount++;             // Punkt hinzufügen
      mylcd.setCursor(0, 1);  // Cursor auf die zweite Zeile setzen
      for (int i = 0; i < dotCount; i++) {
        mylcd.print(".");  // Punkt anzeigen
        if (i % 17 == 0) {
          mylcd.clear();
          mylcd.setCursor(0, 0);
          mylcd.print("Verbindung...");  // Anzeige: Verbindung wird hergestellt
          mylcd.setCursor(0, 1);
        }
      }
      timeout--;  // Timeout herunterzählen
    }
    mylcd.clear();
    mylcd.setCursor(0, 0);



    // Wenn verbunden, zeige den Status auf dem LCD an
    if (WiFi.status() == WL_CONNECTED) {
      mylcd.clear();
      mylcd.setCursor(0, 0);
      mylcd.print("Verbunden - IP:");
      mylcd.setCursor(0, 1);
      mylcd.print(WiFi.localIP());  // Anzeige der lokalen IP-Adresse
      delay(3000);
    } else {
      if (runNR == 1) {  //2.SSID überschreibt ssid und wird im 2. versuch verwendet sollte es dann immer noch nicht verbunden sein, wier
        password = password2;
        ssid = ssid2;
        timeout = 30;
        dotCount = 0;
        runNR++;
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print("Suche:");
        mylcd.setCursor(0, 1);
        mylcd.print(ssid);
        delay(500);

      } else {
        while (true) {
          mylcd.clear();
          mylcd.setCursor(0, 0);
          mylcd.print("Verbindung");  // Anzeige: Verbindung fehlgeschlagen
          mylcd.setCursor(0, 1);
          mylcd.print("fehlgeschlagen");
          delay(60000);
          ESP.restart();
        }
      }
    }
  }

  server.begin();
  MDNS.addService("http", "tcp", 80);

  mylcd.clear();
  mylcd.setCursor(0, 0);
}




void loop() {

  if (resetCounter == 0) {
    ESP.restart();
  } else {
    resetCounter--;
  }

  xht.receive(dht);                    // Update der Temperatur
  bool btn1State = digitalRead(btn1);  //Statusabfrage der Button
  bool btn2State = digitalRead(btn2);  //Statusabfrage der Button

  // Start Bildschirm zeigt DI an
  mylcd.setCursor(0, 0);
  mylcd.print("DISCOVER    ");  //Leerzeichen dafür da, dass Temp und LF auf selber höhe sind, falls aktiviert
  // Temperaturanzeige, wenn tempOn = 1 wird ergänzt (Steuerung mit app)
  if (tempOn == 1) {
    mylcd.print(dht[2]);     // Temperatur anzeigen
    mylcd.print((char)223);  // entspricht dem ° Zeichen und war nicht anders möglich
    mylcd.print("C");        // Einheit Celsius
  }
  mylcd.setCursor(0, 1);
  mylcd.print("INDUSTRY    ");
  //Anzeige Luftfeuchtigkeit wenn tempOn = 1
  if (tempOn == 1) {
    mylcd.print(dht[0]);  // Feuchtigkeit anzeigen
    mylcd.print(" %");    // Prozentzeichen für Feuchtigkeit
  }

  WiFiClient client = server.available();  //Abfrage Objekt ob webseite aufgerufen wurde

  //wenn keine Internetseite (über die Smarthome App) aufgerufen wird soll folgendes passieren (Smart Home Physische sensoren werden Ausgelesen)
  if (!client) {

    // Alarmanlage: Aktivierung wenn bewegung erkannt wird und die Tür geschlossen ist
    bool pyroelectric_val = digitalRead(pyroelectric);
    if (opnD == 0) {
      if (pyroelectric_val == 1) {
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print(" !!! Alarm !!!");
        mylcd.setCursor(0, 1);
        mylcd.print(" Kein  Zutritt");
        delay(500);
        tone(buzzer_pin, 200, 50, 0);           //200 = Frequenz, 50 = dauer in Mil.sek., 0 = Audio Chanel (wichtig!)
        noTone(buzzer_pin, 0);                  //Schaltet buzzer aus. Nicht vergessen!
        colorWipe(strip.Color(255, 0, 0), 50);  //Strip.Color(R,G,B) mit normalen RGB werten von 0-255) 50 = anzeige dauer
        colorWipe(strip.Color(0, 0, 255), 50);
        tone(buzzer_pin, 200, 50, 0);
        noTone(buzzer_pin, 0);  //Schaltet buzzer aus. Nicht vergessen!
        colorWipe(strip.Color(255, 0, 0), 50);
        colorWipe(strip.Color(0, 0, 255), 50);
        tone(buzzer_pin, 200, 50, 0);
        noTone(buzzer_pin, 0);  //Schaltet buzzer aus. Nicht vergessen!
        colorWipe(strip.Color(255, 0, 0), 50);
        colorWipe(strip.Color(0, 0, 255), 50);
        colorWipe(strip.Color(0, 0, 0), 50);
        noTone(buzzer_pin, 0);  //Schaltet buzzer aus. Nicht vergessen! Reststrom kann ggf. fehler am Display verursachen!
        delay(500);
        mylcd.clear();          //nach jeder benutzung muss das LCD geleert werden
        mylcd.setCursor(0, 0);  // so wie der cursor auf die erste Zeile gesetzt werden
      }
    }

    //Lichtschalter für LED
    if (btn1State == 0 && btn2State == 1) {
      if (ledOn == 0) {
        digitalWrite(led_y, HIGH);  // High heißt LED ist an Low heißt aus
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print("    Licht  an");
        delay(500);
        mylcd.clear();
        mylcd.setCursor(0, 0);
        ledOn = 1;
        resetCounter = originalResetCount;
      } else {
        digitalWrite(led_y, LOW);
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print("    Licht aus");
        delay(500);
        mylcd.clear();
        mylcd.setCursor(0, 0);
        ledOn = 0;
        resetCounter = originalResetCount;
      }
    };

    //Klingel
    if (bellLocker == false) {
      if (btn1State == 1 && btn2State == 0) {
        delay(200);
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print("  Es  klingelt");
        tone(buzzer_pin, 500, 500, 1);  //200 = Frequenz, 50 = dauer in Mil.sek., 0 = Audio Chanel (wichtig!)
        delay(200);
        tone(buzzer_pin, 400, 500, 1);
        noTone(buzzer_pin, 0);  //Schaltet buzzer aus. Nicht vergessen! Reststrom kann ggf. fehler am Display verursachen!
        mylcd.clear();
        mylcd.setCursor(0, 0);
        resetCounter = originalResetCount;
        delay(200);
        bellLocker = true;
      }
    }


    //Regensensor der fenster bei feuchte (finger auf sensor reicht) schließt
    int water_val = analogRead(waterPin);


    if (water_val > gwRegen) {
      ledcWrite(channel_PWM, 65);  //65 ist geschlossen und sollte auch nicht verändert werden, sofern sich nicht baulich etwas am haus ändert
      delay(15);
      opnW = 0;
      mylcd.clear();
      mylcd.setCursor(0, 0);
      mylcd.print(" Regen  erkannt");
      mylcd.setCursor(0, 1);
      mylcd.print("   Fenster zu");
      delay(1000);
      mylcd.clear();
      mylcd.setCursor(0, 0);
      resetCounter = originalResetCount;
    } else {
      if (opnW == 0) {
        ledcWrite(channel_PWM, 26);  //Fenster auf
        opnW = 1;
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print("   Fenster auf");
        delay(1000);
        mylcd.clear();
        mylcd.setCursor(0, 0);
        resetCounter = originalResetCount;
      }
    }

    // Reset der Temperaturgrenze für Klimaautomatik und Regensensor wenn beide Buttons gleichzeitig gedrückt werden
    if (btn1State == 0 && btn2State == 0) {
      mylcd.clear();
      mylcd.setCursor(0, 0);
      mylcd.print("ResetT= ");
      mylcd.print(dht[2]);
      mylcd.setCursor(0, 1);
      mylcd.print("GWRegen=");
      delay(2000);
      int regen_val = analogRead(waterPin);
      gwRegen = initialgwR + regen_val;
      mylcd.print(gwRegen);
      temp = dht[2];  //Variable wird mit aktueller Temperatur überschrieben
      delay(1000);
      mylcd.clear();
      mylcd.setCursor(0, 0);
      resetCounter = originalResetCount;
    }


    //bei Temperaturen über der reset Temperatur +1°C wird der Ventilator aktiviert bis die Temperatur wieder unter der genannten ist. alles über 1°C dauert zu lang mit dem finger. falls es nicht klappt könnte man eine mechhanik einbauen die den Temperaturanstieg (Dezimal) über ein paar loop zyklen beobachtet und wenn der zu groß ist, dann klima auslösen. oder über LF faken.
    if (dht[2] > temp + 1) {
      if (fanOn == 0) {
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print(" Klimaanlage an");
        mylcd.setCursor(0, 1);
        mylcd.print("      ");
        mylcd.print(dht[2]);
        mylcd.print((char)223);
        mylcd.print("C");  // Einheit Celsius
        fanOn = 1;
        for (int speed = 0; speed <= 700; speed += 20) {
          ledcWrite(fanChannel, speed);
          delay(100);
        }
        delay(2000);
        resetCounter = originalResetCount;
        mylcd.clear();
        mylcd.setCursor(0, 0);
      }
    } else {
      if (fanOn == 1) {
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print(" Klimaanlage aus");
        mylcd.setCursor(0, 1);
        mylcd.print("      ");
        mylcd.print(dht[2]);
        mylcd.print((char)223);
        mylcd.print("C");  // Einheit Celsius
        fanOn = 0;
        ledcWrite(fanChannel, 0);
        delay(2000);
        mylcd.clear();
        mylcd.setCursor(0, 0);
        resetCounter = originalResetCount;
      }
    }




    //Auslesen des RFID Chips. Wenn der richtige Chip dran ist, wird das Haus aufgesperrt und die Alarmanlage deaktiviert. RFID Block muss am schluss plaziert werden. Handling anders nicht möglich wegen Returns.

    //Bremse dass NFC Sensor nicht auf dauer feuer ist und überhitzt.
    //Wenn keine karte präsentiert wird wird einfach returned
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      delay(50);
      client.stop();  //schließt den Client, dass er nicht dauernd offen ist. wichtig!
      return;
    }

    //Wenn eine Karte erkanntwurde gehts hier weiter
    // UID der Karte wird ausgelesen
    Serial.print(F("Card UID:"));
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      //Serial.print(mfrc522.uid.uidByte[i], HEX);
      //Serial.print(mfrc522.uid.uidByte[i]);
      password_card = password_card + String(mfrc522.uid.uidByte[i]);
    }
    Serial.println(password_card);  // UID wird im Seriellen Monitor ausgegeben, damit man sie auslesen kann, falls mal eine neue da hin kommt.


    //Wenn die kartennummer stimmt geht die tür auf
    //Sollte mal ein neuer chip oder eine neue Karte verwendet werden, dann muss in der Variable pwD die NUMMER angepasst werden (s. Initialisierung). Auslesen kann man es über den Seriellen Monitor
    if (password_card == pwD || password_card == pwD2 || password_card == pwD3 || password_card == pwD4) {
      if (opnD == 0) {
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print(" Aufgeschlossen");
        ledcWrite(channel_PWM2, 26);
        password_card = "";  // Passwort muss für nächsten Zyklus wieder auf null gesetzt werden
        opnD = 1;
        delay(1000);
        mylcd.clear();
        mylcd.setCursor(0, 0);
        resetCounter = originalResetCount;
      } else {
        mylcd.clear();
        mylcd.setCursor(0, 0);
        mylcd.print(" Abgeschlossen");
        ledcWrite(channel_PWM2, 65);
        password_card = "";  // Passwort muss für nächsten Zyklus wieder auf null gesetzt werden
        opnD = 0;
        delay(1000);
        mylcd.clear();
        mylcd.setCursor(0, 0);
        resetCounter = originalResetCount;
      }
    } else {
      //ist die nummer falsch, wird der fehler angezeigt
      password_card = "";  // Passwort muss für nächsten Zyklus wieder auf null gesetzt werden
      mylcd.setCursor(0, 0);
      mylcd.print(" Kein  Zutritt!");
      mylcd.setCursor(0, 1);
      mylcd.print("NFC  Tag  falsch");
      delay(1000);
      mylcd.clear();
      mylcd.setCursor(0, 0);
      resetCounter = originalResetCount;
    }
    client.stop();  //schließt den Client, dass er nicht dauernd offen ist. wichtig!
    return;
  }
  // Alle weiteren Befehle sollten immer vor dem RFID Teil eingesetzt werden, da dieser ein return enthält und anders nicht funktioniert und somit der code danach unwirksam wäre.





  // Ab hier wird alles erledigt, wenn zu beginn der schleife eine Abfrage des Clients registriert wurde. Also es werden Befehel ausgeführt, welche durch abruf der jeweiligen internetadresse getriggert werden. (SnartHome APP)
  while (client.connected() && !client.available()) {
    delay(15);
  }

  //zusammensetzung der adresse
  String req = client.readStringUntil('\r');
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return;
  }
  req = req.substring(addr_start + 1, addr_end);
  item = req;
  Serial.println(item);
  String s;


  if (req == "/")  //Main Seite mit Output
  {
    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    // s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP32 at ";
    // s += ipStr;
    // s += "</html>\r\n\r\n";
    // Serial.println("Sending 200");
    // client.println(s);  //Send the string S, you can read the information when visiting the address of E smart home using a browser.
    //                     //client.print(s);
  }

  //wird ausgeführt wenn man <ip-adresse>/led/on auf einem Endgerät im selben WLAN aufruft.
  if (req == "/led/on") {
    client.println("Licht An");
    digitalWrite(led_y, HIGH);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("   Licht   an");
    delay(500);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    ledOn = 1;
    resetCounter = originalResetCount;
  }

  if (req == "/led/off") {
    client.println("licht aus");
    digitalWrite(led_y, LOW);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("   Licht   aus");
    delay(500);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    ledOn = 0;
    resetCounter = originalResetCount;
  }

  if (req == "/fan/on") {
    client.println(" Ventilator an");
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print(" Klimaanlage an");
    mylcd.setCursor(0, 1);
    mylcd.print("      ");
    mylcd.print(dht[2]);
    mylcd.print((char)223);
    mylcd.print("C");  // Einheit Celsius
    fanOn = 1;
    for (int speed = 0; speed <= 700; speed += 20) {  // 700 ist perfekt. Höhere werte könnten zu problemen der Stromversorgung führen →z.B. Display Probleme
      ledcWrite(fanChannel, speed);
      delay(100);
    }
    delay(2000);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    resetCounter = originalResetCount;
  }

  if (req == "/fan/off") {
    client.println("Ventilator Aus");
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print(" Klimaanlage aus");
    mylcd.setCursor(0, 1);
    mylcd.print("      ");
    mylcd.print(dht[2]);
    mylcd.print((char)223);
    mylcd.print("C");  // Einheit Celsius
    fanOn = 0;
    ledcWrite(fanChannel, 0);
    delay(1000);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    resetCounter = originalResetCount;
  }

  if (req == "/temp/on") {
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Temperatur  ");
    mylcd.print(dht[2]);
    mylcd.print((char)223);
    mylcd.print("C");
    mylcd.setCursor(0, 1);
    mylcd.print("Luftfeuchte ");
    mylcd.print(dht[0]);
    mylcd.print(" %");
    delay(1000);
    tempOn = 1;
    mylcd.clear();
    mylcd.setCursor(0, 0);
    resetCounter = originalResetCount;
  }

  if (req == "/temp/off") {

    tempOn = 0;
    mylcd.clear();
    mylcd.setCursor(0, 0);
    resetCounter = originalResetCount;
  }

  if (req == "/door/close") {
    Serial.println(" Abgeschlossen");
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("  Abgeschlossen");
    ledcWrite(channel_PWM2, 65);
    delay(15);
    opnD = 0;
    delay(1000);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    resetCounter = originalResetCount;
  }

  if (req == "/door/open") {
    ledcWrite(channel_PWM2, 26);
    delay(15);
    mylcd.clear();
    Serial.println("  Aufgeschlossen ");
    mylcd.setCursor(0, 0);
    mylcd.clear();
    mylcd.print("Aufgeschlossen");
    opnD = 1;
    delay(1000);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    resetCounter = originalResetCount;
  }

  if (req == "/window/open") {

    delay(15);
    mylcd.clear();
    Serial.println("   Fenst  auf   ");
    mylcd.setCursor(0, 0);
    mylcd.clear();
    mylcd.print("   Fenst  auf");
    ledcWrite(channel_PWM, 26);
    delay(15);
    opnW = 0;
    delay(1000);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    resetCounter = originalResetCount;
  }

  if (req == "/window/close") {

    delay(15);
    mylcd.clear();
    Serial.println("Fenst Zu");
    mylcd.setCursor(0, 0);
    mylcd.clear();
    mylcd.print("   Fenster  zu");
    ledcWrite(channel_PWM, 65);
    opnW = 1;
    delay(1000);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    resetCounter = originalResetCount;
  }

  if (req == "/restart") {
    ESP.restart();
  }

  if (req == "/sleep") {
    mylcd.noBacklight();
    esp_deep_sleep_start();
  }

  if (req == "/reset") {
      mylcd.clear();
      mylcd.setCursor(0, 0);
      mylcd.print("ResetT= ");
      mylcd.print(dht[2]);
      mylcd.setCursor(0, 1);
      mylcd.print("GWRegen=");
      delay(2000);
      int regen_val = analogRead(waterPin);
      gwRegen = initialgwR + regen_val;
      mylcd.print(gwRegen);
      temp = dht[2];  //Variable wird mit aktueller Temperatur überschrieben
      delay(1000);
      mylcd.clear();
      mylcd.setCursor(0, 0);
      resetCounter = originalResetCount;
  }

  client.stop();  //schließt den Client, dass er nicht dauernd offen ist. wichtig!
}


//Void Schleife für das 2x2 LED Panel
void colorWipe(uint32_t color, int wait) {
  for (int i2 = 0; i2 < strip.numPixels(); i2++) {  // für jeden pixel im strip...
    strip.setPixelColor(i2, color);                 //  Set pixel's color (in RAM)
    strip.show();                                   //  Update strip to match
    delay(wait);                                    //  Pause for a moment (wert aus übergabe)
  }
}
