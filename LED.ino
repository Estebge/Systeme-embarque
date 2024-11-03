//Appel des bibliothèque pour les capteurs, la carte SD, le GPS, ...
#include <EEPROM.h>
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
long test;

uint8_t etatBoutonR = HIGH;
uint8_t etatBoutonV = HIGH;
long t1Vert, t1Rouge;

SoftwareSerial gpsSerial(8, 9);  // Créer un port série logiciel pour le GPS

Adafruit_BME280 bme;
RTC_DS3231 rtc;
File dataFile;

// Déclaration de la variable inputString
String inputString = "";

// Définition des adresses dans l'EEPROM
int ADDR_LOG_INTERVAL = 0;
#define ADDR_TIMEOUT 2
#define ADDR_LUMIN 3
#define ADDR_LUMIN_LOW 4
#define ADDR_LUMIN_HIGH 5
#define ADDR_TEMP_AIR 6
#define ADDR_MIN_TEMP_AIR 7
#define ADDR_MAX_TEMP_AIR 8
#define ADDR_HYGR 9
#define ADDR_HYGR_MINT 10
#define ADDR_HYGR_MAXT 11
#define ADDR_PRESSURE 12
#define ADDR_PRESSURE_MIN 13
#define ADDR_PRESSURE_MAX 14

// Définition des valeurs par défaut
int DEFAULT_LOG_INTERVAL = 1000;
#define DEFAULT_TIMEOUT 30
#define DEFAULT_LUMIN 1
#define DEFAULT_LUMIN_LOW 255
#define DEFAULT_LUMIN_HIGH 768
#define DEFAULT_TEMP_AIR 1
#define DEFAULT_MIN_TEMP_AIR -10
#define DEFAULT_MAX_TEMP_AIR 60
#define DEFAULT_HYGR 1
#define DEFAULT_HYGR_MINT 0
#define DEFAULT_HYGR_MAXT 50
#define DEFAULT_PRESSURE 1
#define DEFAULT_PRESSURE_MIN 850
#define DEFAULT_PRESSURE_MAX 1080

// Structure pour la configuration générale
struct ConfigGenerale {
  uint8_t logInterval;
  uint8_t timeout;
};

// Structure pour le capteur de luminosité
struct Luminosite {
  uint8_t active;
  uint16_t seuilBas;
  uint16_t seuilHaut;
};

// Structure pour le capteur de température de l'air
struct TemperatureAir {
  uint8_t active;
  int8_t minTemp;
  int8_t maxTemp;
};

// Structure pour le capteur d'hygrométrie
struct Hygrometrie {
  uint8_t active;
  int8_t minTemp;
  int8_t maxTemp;
};

// Structure pour le capteur de pression
struct Pression {
  uint8_t active;
  uint16_t pressionMin;
  uint16_t pressionMax;
};

// Créer une structure pour regrouper tous les paramètres
struct Parametres {
  ConfigGenerale configGenerale;
  Luminosite luminosite;
  TemperatureAir temperatureAir;
  Hygrometrie hygrometrie;
  Pression pression;
} parametres;

// Valeurs par défaut pour les paramètres
void definirValeursParDefaut() {
  parametres.configGenerale.logInterval = DEFAULT_LOG_INTERVAL;
  parametres.configGenerale.timeout = DEFAULT_TIMEOUT;

  parametres.luminosite.active = DEFAULT_LUMIN;
  parametres.luminosite.seuilBas = DEFAULT_LUMIN_LOW;
  parametres.luminosite.seuilHaut = DEFAULT_LUMIN_HIGH;

  parametres.temperatureAir.active = DEFAULT_TEMP_AIR;
  parametres.temperatureAir.minTemp = DEFAULT_MIN_TEMP_AIR;
  parametres.temperatureAir.maxTemp = DEFAULT_MAX_TEMP_AIR;

  parametres.hygrometrie.active = DEFAULT_HYGR;
  parametres.hygrometrie.minTemp = DEFAULT_HYGR_MINT;
  parametres.hygrometrie.maxTemp = DEFAULT_HYGR_MAXT;

  parametres.pression.active = DEFAULT_PRESSURE;
  parametres.pression.pressionMin = DEFAULT_PRESSURE_MIN;
  parametres.pression.pressionMax = DEFAULT_PRESSURE_MAX;
}

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

 // Vérification de la présence de trames GPS
  if (gpsSerial.available()) {
    String nmea = "";
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      nmea += c;
      
      if (c == '\n') { // Si on détecte une fin de trame (nouvelle ligne)
        Serial.println("Trame GPS : " + nmea);  // Affiche la trame pour vérification
        nmea = "";  // Réinitialise la chaîne pour la prochaine lecture
      }
    }
  } else {
    error = GPS_ERROR;  // Définit une erreur si aucune trame n'est disponible
  }
  delay(100);


  //Vérification présence capteur
  if (!bme.begin(0x76)){
    error = CAP_ERROR;
  }
  delay(100);

  
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
  Serial.println(F("Carte OK"));
  delay(100);

  // Vérifie d'abord si le fichier existe déjà
  if (!SD.exists("data.csv")) {
    dataFile = SD.open("data.csv", FILE_WRITE); // Crée le fichier s'il n'existe pas
    if (dataFile) {
      Serial.println(F("Fichier créé"));
      dataFile.println(F("Date;Heure;Temp;Press;Hum;Lum;Lat;Long")); // Écrit les en-têtes
      dataFile.close();
    } else {
      error = WRITE_ERROR;  // Indique une erreur d'écriture
    }
  } else {
    Serial.println(F("Fichier déjà présent"));
  }
  delay(100);

  //Vérification présence horloge
  if (!rtc.begin()) {
    error = RTC_ERROR;
  }
  delay(100);
  
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Réglez à l'heure de compilation
  }
  delay(100);

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Récupérer la date et l'heure actuelles du RTC
  DateTime now = rtc.now();
  delay(100);

  // if (bme.readTemperature() < minTempAir || bme.readTemperature() > maxTempAir || bme.readPressure() < pressureMin || bme.readPressure() > pressureMax || bme.readHumidity() < hygrMint || bme.readHumidity() > hygrMaxt || (analogRead(lightSensorPin) / 1023.0) * 100) < luminLow || (analogRead(lightSensorPin) / 1023.0) * 100) > luminHigh){
  //   error = VAL_ERROR;
  // }

  // Écrire les données dans le fichier CSV
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    Serial.println(F("Écriture des données"));
    
    // Écriture de la date et de l'heure
    dataFile.print(now.month(), DEC); dataFile.print('/');
    delay(100);
    dataFile.print(now.day(), DEC); dataFile.print('/');
    delay(100);
    dataFile.print(now.year(), DEC); dataFile.print("; ");
    delay(100);
    dataFile.print(now.hour(), DEC); dataFile.print(':');
    delay(100);
    dataFile.print(now.minute(), DEC); dataFile.print(':');
    delay(100);
    dataFile.print(now.second(), DEC); dataFile.print("; ");
    delay(100);

    // Écriture des données capteurs
    dataFile.print(bme.readTemperature()); dataFile.print("C;");
    delay(100);
    dataFile.print(bme.readPressure() / 100.0F); dataFile.print(" hPa;");
    delay(100);
    dataFile.print(bme.readHumidity()); dataFile.print("%;");
    delay(100);
    dataFile.print((analogRead(lightSensorPin) / 1023.0) * 100); dataFile.print("%;");
    delay(100);
    dataFile.print("43"); dataFile.write(176); dataFile.print("28'52.4\"N; ");
    dataFile.print("5"); dataFile.write(176); dataFile.println("23'11.0\"E; ");

    dataFile.close();  // Fermer le fichier pour sauvegarder les données
  } else {
    error = DATA_ERROR;
  }
  if (ADDR_LOG_INTERVAL != 0){
    delay(ADDR_LOG_INTERVAL);
  }
  else{
    delay(DEFAULT_LOG_INTERVAL);
  }
}

//Déclaration des différents mode et de leur action
//Par exemple ce mode appel la fonction LED pour avoir la couleur du mode puis nous stockons les données
void standard(){
  led();
  ADDR_LOG_INTERVAL *= 2;
  DEFAULT_LOG_INTERVAL *= 2;
  Recup_data();
}

void economique(){
  led();
  ADDR_LOG_INTERVAL *= 2;
  DEFAULT_LOG_INTERVAL *= 2;
  Recup_data();
}

void configuration(){
  led();
  Serial.println(F("Tapez une commande (ex: LOG_INTERVALL=20 ou RESET)."));
  while (test < 30000){
    test = millis();
    if (Serial.available()) {
      inputString = Serial.readStringUntil('\n');
      inputString.trim(); // Enlever les espaces inutiles ou retour à la ligne
      processCommand(inputString);
    }
  }
  mode = STD;
  standard();
  delay(100);
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
  definirValeursParDefaut();

  mode = INIT;

  leds.setColorRGB(0, 0, 0, 0);

  startTime = millis();

  attachInterrupt(digitalPinToInterrupt(boutonRougePin), BoutonRouge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(boutonVertPin), Boutonvert, CHANGE);
  delay(6000);
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

void processCommand(String command) {
  if (command.startsWith("LOG_INTERVALL=")) {
    int interval = command.substring(14).toInt();
    EEPROM.write(ADDR_LOG_INTERVAL, interval);
    Serial.print(F("LOG_INTERVALL : "));
    Serial.println(interval);
  }
  else if (command.startsWith("TIMEOUT=")) {
    int timeout = command.substring(8).toInt();
    EEPROM.write(ADDR_TIMEOUT, timeout);
    Serial.print(F("TIMEOUT : "));
    Serial.println(timeout);
  }
  else if (command.startsWith("LUMIN=")) {
    int lumin = command.substring(6).toInt();
    EEPROM.write(ADDR_LUMIN, lumin);
    Serial.print(F("LUMIN : "));
    Serial.println(lumin);
  }
  else if (command.startsWith("LUMIN_LOW=")) {
    int luminLow = command.substring(10).toInt();
    EEPROM.put(ADDR_LUMIN_LOW, luminLow);
    Serial.print(F("LUMIN_LOW : "));
    Serial.println(luminLow);
  }
  else if (command.startsWith("LUMIN_HIGH=")) {
    int luminHigh = command.substring(11).toInt();
    EEPROM.put(ADDR_LUMIN_HIGH, luminHigh);
    Serial.print(F("LUMIN_HIGH : "));
    Serial.println(luminHigh);
  }
  else if (command.startsWith("TEMP_AIR=")) {
    int tempAir = command.substring(9).toInt();
    EEPROM.write(ADDR_TEMP_AIR, tempAir);
    Serial.print(F("TEMP_AIR : "));
    Serial.println(tempAir);
  }
  else if (command.startsWith("MIN_TEMP_AIR=")) {
     int minTempAir = command.substring(13).toInt();
    EEPROM.put(ADDR_MIN_TEMP_AIR, minTempAir);
    Serial.print(F("MIN_TEMP_AIR : "));
    Serial.println(minTempAir);
  }
  else if (command.startsWith("MAX_TEMP_AIR=")) {
    int maxTempAir = command.substring(13).toInt();
    EEPROM.put(ADDR_MAX_TEMP_AIR, maxTempAir);
    Serial.print(F("MAX_TEMP_AIR : "));
    Serial.println(maxTempAir);
  }
  else if (command.startsWith("HYGR=")) {
    int hygr = command.substring(5).toInt();
    EEPROM.write(ADDR_HYGR, hygr);
    Serial.print(F("HYGR défini à : "));
    Serial.println(hygr);
  }
  else if (command.startsWith("HYGR_MINT=")) {
    int hygrMint = command.substring(10).toInt();
    EEPROM.put(ADDR_HYGR_MINT, hygrMint);
    Serial.print(F("HYGR_MINT : "));
    Serial.println(hygrMint);
  }
  else if (command.startsWith("HYGR_MAXT=")) {
    int hygrMaxt = command.substring(10).toInt();
    EEPROM.put(ADDR_HYGR_MAXT, hygrMaxt);
    Serial.print(F("HYGR_MAXT : "));
    Serial.println(hygrMaxt);
  }
  else if (command.startsWith("PRESSURE=")) {
    int pressure = command.substring(9).toInt();
    EEPROM.write(ADDR_PRESSURE, pressure);
    Serial.print(F("PRESSURE : "));
    Serial.println(pressure);
  }
  else if (command.startsWith("PRESSURE_MIN=")) {
    int pressureMin = command.substring(13).toInt();
    EEPROM.put(ADDR_PRESSURE_MIN, pressureMin);
    Serial.print(F("PRESSURE_MIN : "));
    Serial.println(pressureMin);
  }
  else if (command.startsWith("PRESSURE_MAX=")) {
    int pressureMax = command.substring(13).toInt();
    EEPROM.put(ADDR_PRESSURE_MAX, pressureMax);
    Serial.print(F("PRESSURE_MAX : "));
    Serial.println(pressureMax);
  }
  else if (command.startsWith("RESET")) {
    Serial.println(F("Réinitialisation"));
    resetParameters();
  }
  else {
    Serial.println(F("Commande inconnue."));
  }
}


void resetParameters() {
  EEPROM.write(ADDR_LOG_INTERVAL, DEFAULT_LOG_INTERVAL);
  EEPROM.write(ADDR_TIMEOUT, DEFAULT_TIMEOUT);
  EEPROM.write(ADDR_LUMIN, DEFAULT_LUMIN);
  EEPROM.put(ADDR_LUMIN_LOW, DEFAULT_LUMIN_LOW);
  EEPROM.put(ADDR_LUMIN_HIGH, DEFAULT_LUMIN_HIGH);
  EEPROM.write(ADDR_TEMP_AIR, DEFAULT_TEMP_AIR);
  EEPROM.put(ADDR_MIN_TEMP_AIR, DEFAULT_MIN_TEMP_AIR);
  EEPROM.put(ADDR_MAX_TEMP_AIR, DEFAULT_MAX_TEMP_AIR);
  EEPROM.write(ADDR_HYGR, DEFAULT_HYGR);
  EEPROM.put(ADDR_HYGR_MINT, DEFAULT_HYGR_MINT);
  EEPROM.put(ADDR_HYGR_MAXT, DEFAULT_HYGR_MAXT);
  EEPROM.write(ADDR_PRESSURE, DEFAULT_PRESSURE);
  EEPROM.put(ADDR_PRESSURE_MIN, DEFAULT_PRESSURE_MIN);
  EEPROM.put(ADDR_PRESSURE_MAX, DEFAULT_PRESSURE_MAX);
}
