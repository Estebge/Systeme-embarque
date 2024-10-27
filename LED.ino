#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <RTClib.h>
#include <SD.h>
#include <ChainableLED.h>
#include <SoftwareSerial.h>

#define boutonRougePin 3
#define boutonVertPin  2

ChainableLED leds(5, 6, 1);

#define SEALEVELPRESSURE_HPA (1013.25)  // Pression au niveau de la mer standard
#define lightSensorPin A0
#define SD_CS_PIN 4  // Pin CS pour la carte SD

//Initialisation variable capteur erreur
#define NO_ERROR 0
#define RTC_ERROR 1
#define GPS_ERROR 2
#define CAP_ERROR 3
#define VAL_ERROR 4
#define DATA_ERROR 5
#define WRITE_ERROR 6

//Initialisation variable mode
#define INIT 0
#define STD 1
#define ECO 2
#define CFG 3
#define MNT 4

uint8_t error;
uint8_t mode;
uint8_t mode_prec;

unsigned long startTime = millis();

uint8_t etatBoutonR = HIGH;
uint8_t etatBoutonV = HIGH;
long t1Vert, t1Rouge;

SoftwareSerial gpsSerial(3, 4);  // Créer un port série logiciel pour le GPS

Adafruit_BME280 bme;
RTC_DS3231 rtc;
File dataFile;

void led(){
  switch (error){
    case RTC_ERROR : leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 0, 0, 255);
    delay(500);
    break;
    case GPS_ERROR : leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 255, 255, 0);
    delay(500);
    break;
    case CAP_ERROR : leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 255, 255, 0);
    delay(500);
    break;
    case VAL_ERROR : leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 255, 255, 0);
    delay(500);
    break;
    case DATA_ERROR : leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 255, 255, 255);
    delay(500);
    break;
    case WRITE_ERROR : leds.setColorRGB(0, 255, 0, 0);
    delay(500);
    leds.setColorRGB(0, 255, 255, 255);
    delay(1000);
    break;
    default : 
      switch(mode)
      {
        case STD : leds.setColorRGB(0, 0, 255, 0); 
        break;
        case ECO : leds.setColorRGB(0, 0, 0, 255); 
        break;
        case CFG : leds.setColorRGB(0, 255, 255, 0); 
        break;
        case MNT : leds.setColorRGB(0, 255, 120, 0); 
        break;        
      }
  }
}

void Boutonvert()
{
  if (etatBoutonV == HIGH) {
    t1Vert = millis();
    etatBoutonV = LOW;
  }
  else {
    long t2 = millis();
    long dureePression = t2-t1Vert;
    if (dureePression > 5000){
      if (mode == ECO){
        mode = STD;
      }
      else{
        mode = ECO;
      }
    }
    etatBoutonV = HIGH;
    t1Vert = 0;
  }
}

void BoutonRouge(){
  if (etatBoutonR == HIGH) {
    t1Rouge = millis();
    etatBoutonR = LOW;
  }
  else {
    long t2 = millis();
    long dureePression = t2 - t1Rouge;
    if (mode == INIT && dureePression > 50) {  // Passe directement en CFG si on presse le bouton en INIT
      mode = CFG;
    }
    else if (dureePression > 5000) {  // Passe en MNT ou retourne au mode précédent si appui long en dehors de INIT
      if (mode == STD) {
        mode_prec = STD;
        mode = MNT;  // Passe au mode MNT si appui long en mode STD
      }
      else if (mode == ECO) {
        mode_prec = ECO;
        mode = MNT;  // Passe également au mode MNT depuis ECO avec appui long
      }
      else if (mode == MNT) {
        mode = mode_prec;  // Retourne au mode précédent après MNT
        mode_prec = INIT;  // Réinitialise pour éviter les conflits futurs
      }
    }
    etatBoutonR = HIGH;
    t1Rouge = 0;
  }
}

void Recup_data(){
  error = NO_ERROR;
  //Vérification présence horloge
  if (!rtc.begin()) {
    error = RTC_ERROR;
  }
  
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Réglez à l'heure de compilation
  }

  // Vérification GPS, attente de données GPS sur 5 cycles avant signalement d'erreur
  bool gpsTrouve = false;
  for (int i = 0; i < 5; i++) {
    if (gpsSerial.available()) {
      String nmea = "";
      
      while (gpsSerial.available()) {
        char c = gpsSerial.read();
        nmea += c;

        if (nmea.startsWith("$GPGGA")) {  // Filtre les trames GPS valides
          gpsTrouve = true;
          break;
        }
      }
      if (gpsTrouve) break;
    }
    delay(100);  // Pause pour laisser le GPS répondre
  }
  if (!gpsTrouve) {
    error = GPS_ERROR;  // Pas de trame GPS détectée après 5 cycles
  }

  //Vérification présence capteur
  if (!bme.begin(0x76)){
    error = CAP_ERROR;
  }
  
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
    error = DATA_ERROR;
  }
}

void standard(){
  Recup_data();
  led();
  delay(1000);
}

void economique(){
  Recup_data();
  led();
  delay(2000);
}

void configuration(){
  led();
  delay(10000);
  standard();
}

void maintenance(){
  led();
}

void setup() {
  Serial.begin(9600);
  pinMode(boutonRougePin, INPUT_PULLUP);
  pinMode(boutonVertPin, INPUT_PULLUP);
  gpsSerial.begin(9600);
  Wire.begin();

  // Initialisation de la carte SD
  if (!SD.begin(SD_CS_PIN)) {
    error = DATA_ERROR;
  }
  Serial.println("Carte SD initialisée avec succès.");

  // Création du fichier CSV s'il n'existe pas
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    Serial.println("Fichier ouvert avec succès.");
    dataFile.println(F("Date;Heure;Temp;Press;Hum;Lum;GPS"));
    dataFile.close();
  } else {
    error = WRITE_ERROR;
  }

  mode = INIT;

  leds.setColorRGB(0, 0, 0, 0);

  startTime = millis();

  attachInterrupt(digitalPinToInterrupt(boutonRougePin), BoutonRouge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(boutonVertPin), Boutonvert, CHANGE);
  delay(5000);
}

void loop() {
  // Vérifie si le mode est toujours INIT et si 5 secondes se sont écoulées sans appui du bouton
  if (mode == INIT && millis() - startTime > 5000) {
    mode = STD;  // Passe en mode STD après 5 secondes si le bouton rouge n'est pas pressé
  }

  switch (mode){
    case STD : standard();
    break;
    case ECO : economique();
    break;
    case CFG : configuration();
    break;
    case MNT : maintenance();
    break;
  }
}
