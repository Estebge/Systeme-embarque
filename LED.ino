#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <RTClib.h>
#include <SD.h>
#include <ChainableLED.h>
#include <SoftwareSerial.h>

ChainableLED leds(5, 6, 1);

#define SEALEVELPRESSURE_HPA (1013.25)  // Pression au niveau de la mer standard
#define lightSensorPin A0
#define SD_CS_PIN 4  // Pin CS pour la carte SD

bool SDecrit = true;
bool SDpresente = true;

SoftwareSerial gpsSerial(3, 4);  // Créer un port série logiciel pour le GPS

Adafruit_BME280 bme;
RTC_DS3231 rtc;
File dataFile;

void erreur(){
  // Impossible de trouver l'horloge
  while(!rtc.begin()){
    leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 0, 0, 255);
    delay(500);
  }

  // Impossible de trouver le GPS
  while(!gpsSerial.available()){
    leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 255, 255, 0);
    delay(500);
  }

  // Impossible de trouver le capteur BME280
  while(!bme.begin(0x76)){
    leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 0, 255, 0);
    delay(500);
  }

  // Incohérence des capteurs

  // Carte SD pleine
  while(!SDecrit){
    leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 255, 255, 255);
    delay(500);
  }

  // Impossible de trouver la carte SD
  while(!SDpresente){
    leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 255, 255, 255);
    delay(1000);
  }
  
  leds.setColorRGB(0, 0, 0, 0);
}

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  Wire.begin();

  // Initialisation du module RTC
  if (!rtc.begin()) {
    Serial.println(F("Erreur : RTC introuvable !"));
    while (1);
  }
  
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Réglez à l'heure de compilation
  }

  // Initialisation de la carte SD
  if (!SD.begin(SD_CS_PIN)) {
    SDpresente = false;
  }
  Serial.println("Carte SD initialisée avec succès.");

  // Création du fichier CSV s'il n'existe pas
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    Serial.println("Fichier ouvert avec succès.");
    dataFile.println(F("Date;Heure;Temp;Press;Hum;Lum;GPS"));
    dataFile.close();
  } else {
    SDpresente = false;
  }
}

void loop() {
  if (gpsSerial.available()) {
    String nmea = "";
    
    // Lire les données disponibles du GPS
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      nmea += c;
      
      // Vérifier si une trame NMEA valide commence par '$'
      if (nmea.startsWith("$")) {
        Serial.println("Trame NMEA détectée : " + nmea);
        
        // Si la trame commence par GPGGA, alors c'est valide
        if (nmea.startsWith("$GPGGA")) {
          Serial.println("GPS présent et fonctionnel");
        } else {
          Serial.println("GPS présent mais pas encore de trame GPGGA");
        }
        nmea = "";  // Réinitialiser la chaîne pour la prochaine lecture
      }
    }
  } else {
    Serial.println("Erreur d'accès au GPS ou GPS non détecté");
  }

  erreur();
  
  // Lire une ligne complète de NMEA
  // while (Serial.available()) {
  //   char c = Serial.read();
  //   nmea += c;
  //   if (c == '\n') {
  //     // Si on a une ligne complète
  //     if (nmea.startsWith("$GPGGA")) {  // Filtrer les trames GPGGA
  //       Serial.println("Trame GPGGA reçue : " + nmea);
        
  //       // Découper les champs séparés par des virgules
  //       String fields[15];
  //       int index = 0;
  //       for (int i = 0; i < nmea.length(); i++) {
  //         if (nmea[i] == ',' || nmea[i] == '*') {
  //           index++;
  //         } else {
  //           fields[index] += nmea[i];
  //         }
  //       }
        
  //       // Afficher la latitude et la longitude
  //       Serial.print("Latitude : ");
  //       Serial.print(fields[2]);
  //       Serial.print(fields[3]);
  //       Serial.print(", Longitude : ");
  //       Serial.print(fields[4]);
  //       Serial.println(fields[5]);
  //     }
  //     // Réinitialiser la trame pour la prochaine lecture
  //     nmea = "";
  //   }
  // }

  // Récupérer la date et l'heure actuelles du RTC
  DateTime now = rtc.now();

  // Écrire les données dans le fichier CSV
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    Serial.println("Écriture des données dans le fichier...");
    
    // Écriture de la date et de l'heure
    dataFile.print(now.month(), DEC); dataFile.print('/');
    dataFile.print(now.day(), DEC); dataFile.print('/');
    dataFile.print(now.year(), DEC); dataFile.print("; ");
    dataFile.print(now.hour(), DEC); dataFile.print(':');
    dataFile.print(now.minute(), DEC); dataFile.print(':');
    dataFile.print(now.second(), DEC); dataFile.print("; ");

    // Écriture des données capteurs
    dataFile.print(bme.readTemperature()); dataFile.print("C;");
    dataFile.print(bme.readPressure() / 100.0F); dataFile.print(" hPa;");
    dataFile.print(bme.readHumidity()); dataFile.print("%;");
    dataFile.print((analogRead(lightSensorPin) / 1023.0) * 100); dataFile.println("%");

    dataFile.close();  // Fermer le fichier pour sauvegarder les données
  } else {
    SDecrit = false;
  }

  delay(2000);  // Pause de 2 secondes
}
