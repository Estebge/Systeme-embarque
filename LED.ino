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

int etatBoutonRouge = HIGH;
int etatBoutonVert = HIGH;

uint8_t modeprec = 0; // 0 = standard, 1 = eco 
unsigned long startTime = 0;

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
        case MNT : leds.setColorRGB(0, 255, 125, 25); 
        break;        
      }
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
    error = GPS_ERROR;
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

void standart(){
  mode = STD;
  Recup_data();
  led();
  delay(1000);
}

void economique(){
  mode = ECO;
  Recup_data();
  led();
  delay(2000);
}

void configuration(){
  mode = CFG;
  led();
}

void maintenance(){
  mode = MNT;
  led();
}

void setup() {
  Serial.begin(9600);
  pinMode(boutonRougePin, INPUT_PULLUP);
  pinMode(boutonVertPin, INPUT_PULLUP);
  gpsSerial.begin(9600);
  Wire.begin();

  startTime = millis();
  while (millis() - startTime < 5000) {
    etatBoutonRouge = digitalRead(boutonRougePin);
    if (etatBoutonRouge == LOW) {
      configuration();
      dernierTempsAction = millis(); // Mettre à jour le dernier temps d'action
    }
  }

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
}

void loop() {
  etatBoutonVert = digitalRead(boutonVertPin);
  etatBoutonRouge = digitalRead(boutonRougePin);

  // Gestion du mode "stand" par défaut
  if (etatBoutonRouge == HIGH && etatBoutonVert == HIGH && mode == STD) {
    standart();
    mode = 0;
  }

  // Changement vers mode économique
  if (mode == STD && etatBoutonVert == LOW) {
    if (debutPressionVert == 0) {
      debutPressionVert = millis();
    } else if (millis() - debutPressionVert >= 5000) {
      mode = CFG;
      economique();
      debutPressionVert = 0;
      dernierTempsAction = millis(); // Mise à jour du temps d'action
    }
  }

  // Retour en mode "stand" depuis mode économique
  if (mode == CFG && etatBoutonVert == LOW) {
    if (debutPressionVert0 == 0) {
      debutPressionVert0 = millis();
    } else if (millis() - debutPressionVert0 >= 5000) {
      mode = 0;
      standart();
      debutPressionVert0 = 0;
      dernierTempsAction = millis(); // Mise à jour du temps d'action
    }
  }

  if (etatBoutonVert == HIGH) {
    debutPressionVert = 0;
    debutPressionVert0 = 0;
  }

  // Changement vers mode maintenance
  if (mode == STD && etatBoutonRouge == LOW) {
    if (debutPressionRouge0 == 0) {
      debutPressionRouge0 = millis();
    } else if (millis() - debutPressionRouge0 >= 5000) {
      mode = MNT;
      modeprec = STD;
      maintenance();
      debutPressionRouge0 = 0;
      dernierTempsAction = millis(); // Mise à jour du temps d'action
    }
  }

  // Retour depuis mode maintenance
  if (mode == MNT && etatBoutonRouge == LOW) {
    if (debutPressionRouge2 == 0) {
      debutPressionRouge2 = millis();
    } else if (millis() - debutPressionRouge2 >= 5000) {
      if (modeprec == STD) {
        mode = STD;
        standart();
        debutPressionRouge2 = 0;
      } else if (modeprec == ECO) {
        mode = ECO;
        economique();
        debutPressionRouge2 = 0;
      }
      dernierTempsAction = millis(); // Mise à jour du temps d'action
    }
  }

  if (etatBoutonRouge == HIGH) {
    debutPressionRouge0 = 0;
    debutPressionRouge2 = 0;
    debutPressionRouge3 = 0;
  }

  // Gestion du délai d'inactivité de 30 minute pour le mode configuration
  if (mode == CFG && (millis() - dernierTempsAction > 5000)) {
    mode = STD;
    standart();
  }
}
