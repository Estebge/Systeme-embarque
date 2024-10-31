//Appel des bibliothèque pour les capteurs, la carte SD, le GPS, ...
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <RTClib.h>
#include <SD.h>
#include <ChainableLED.h>
#include <SoftwareSerial.h>

//Initialisation des boutons
#define boutonRougePin 3
#define boutonVertPin  2

//Initailisation de la LED
ChainableLED leds(5, 6, 1);

//Initialisation données pour les capteurs
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

//Initialisation variables globales.
uint8_t error;
uint8_t mode;
uint8_t mode_prec;

unsigned long startTime = millis();

uint8_t etatBoutonR = HIGH;
uint8_t etatBoutonV = HIGH;
long t1Vert, t1Rouge;

SoftwareSerial gpsSerial(8, 9);  // Créer un port série logiciel pour le GPS

Adafruit_BME280 bme;
RTC_DS3231 rtc;
File dataFile;

//Fonction LED
//Permet de changer la couleur de la LED en fonction des erreur ou du mode
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

//Fonction du bouton vert
//Permet de changer d'un mode à l'autre grâce à la pression du bouton vert
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

//Pareil que le bouton vert mais avec le rouge
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

//Fonction permettant de récupérer les données
//Permet de vérifier qu'un capteur ou autre soit branchés
//Permet de récupérer et stocker les données s'il n'y a aucun problème
void Recup_data(){
  error = NO_ERROR;
  //Vérification présence horloge
  if (!rtc.begin()) {
    error = RTC_ERROR;
  }
  
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Réglez à l'heure de compilation
  }

 // Vérification de la présence de trames GPS
  if (gpsSerial.available()) {
    String nmea = "";
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      nmea += c;
      
      if (c == '\n') { // Si on détecte une fin de trame (nouvelle ligne)
        Serial.println("Trame GPS détectée : " + nmea);  // Affiche la trame pour vérification
        nmea = "";  // Réinitialise la chaîne pour la prochaine lecture
      }
    }
  } else {
    error = GPS_ERROR;  // Définit une erreur si aucune trame n'est disponible
  }

  //Vérification présence capteur
  if (!bme.begin(0x76)){
    error = CAP_ERROR;
  }
  
  // Les lignes ci dessous sont à tester lorsque nous sommes dans des conditions parfaites pour utiliser le GPS (dehors, avec des satellites disponible de sûr)

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

    // Initialisation de la carte SD
  if (!SD.begin(SD_CS_PIN)) {
    error = DATA_ERROR;
  }
  Serial.println("Carte SD initialisée avec succès.");

  // Vérifie d'abord si le fichier existe déjà
  if (!SD.exists("data.csv")) {
    dataFile = SD.open("data.csv", FILE_WRITE); // Crée le fichier s'il n'existe pas
    if (dataFile) {
      Serial.println("Fichier créé avec succès.");
      dataFile.println(F("Date;Heure;Temp;Press;Hum;Lum;GPS")); // Écrit les en-têtes
      dataFile.close();
    } else {
      error = WRITE_ERROR;  // Indique une erreur d'écriture
    }
  } else {
    Serial.println("Fichier déjà présent, aucun besoin de le recréer.");
  }

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
  delay(1000);
}

//Déclaration des différents mode et de leur action
//Par exemple ce mode appel la fonction LED pour avoir la couleur du mode puis nous stockons les données
void standard(){
  led();
  Recup_data();
}

void economique(){
  led();
  Recup_data();
}

void configuration(){
  led();
  delay(10000);
  mode = STD;
  standard();
}

void maintenance(){
  led();
}

//Fonction qui se lit une fois et permet d'initialiser le programme
//Per exemple c'est ici que nous initialisons la communication avec l'ordi ou que nous initialisons le mode des boutons
void setup() {
  Serial.begin(9600);   //Communication avec l'interface
  pinMode(boutonRougePin, INPUT_PULLUP);
  pinMode(boutonVertPin, INPUT_PULLUP);
  gpsSerial.begin(9600);
  Wire.begin();

  mode = INIT;

  leds.setColorRGB(0, 0, 0, 0);

  startTime = millis();

  attachInterrupt(digitalPinToInterrupt(boutonRougePin), BoutonRouge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(boutonVertPin), Boutonvert, CHANGE);
  delay(5000);
}

//Enfin ce code se fait en permanence
//Il permet de passer d'un mode à l'autre via les fonctions vu précédemment
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
