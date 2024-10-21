#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <RTClib.h>
#include <SD.h>
#include <ChainableLED.h>

#define SEALEVELPRESSURE_HPA (1013.25)  // Pression au niveau de la mer standard

Adafruit_BME280 bme; 
RTC_DS3231 rtc;     
#define lightSensorPin A0

File dataFile;

void setup() {
  Serial.begin(9600);
  Wire.begin();


  if (!bme.begin(0x76)) {
    Serial.println("Erreur : Impossible de trouver le capteur BME280 !");
    while (1);
  }


  if (!rtc.begin()) {
    Serial.println("Erreur : Impossible de trouver le module RTC !");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));


  if (rtc.lostPower()) {
    // Réglez l'heure du RTC si nécessaire
    Serial.println("L'horloge RTC a perdu l'alimentation. Réglage de l'heure...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Réglez à l'heure de compilation
  }


  if (!SD.begin(4)) {
    Serial.println(F("Erreur : Impossible d'initialiser la carte SD !"));
    while (1);
  }


  dataFile = SD.open("Releve_2024.csv", FILE_WRITE);
  if (dataFile) {
    // Écrire l'en-tête du fichier CSV si le fichier est vide
    dataFile.println("Date;Heure;Temp;Press;Hum;Lum");
    dataFile.close();
  } else {
    Serial.println("Erreur : Impossible d'ouvrir le fichier datalog.csv !");
  }
}

void loop() {
  // Récupérer les données du RTC
  DateTime now = rtc.now();

  // Récupérer les données du BME280
  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0F;  // en hPa
  float humidity = bme.readHumidity();

  // Récupérer la luminosité
  int lightValue = analogRead(lightSensorPin);
  float lightPercentage = (lightValue / 1023.0) * 100;


  // Enregistrer les données dans le fichier CSV
  dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(now.month(), DEC); dataFile.print('/');
    dataFile.print(now.day(), DEC); dataFile.print('/');
    dataFile.print(now.year(), DEC); dataFile.print("; ");
    dataFile.print(now.hour(), DEC); dataFile.print(':');
    dataFile.print(now.minute(), DEC); dataFile.print(':');
    dataFile.print(now.second(), DEC); dataFile.print("; ");
    dataFile.print(temperature); dataFile.print("C;");
    dataFile.print(pressure); dataFile.print(" hPa;");
    dataFile.print(humidity); dataFile.print("%;");
    dataFile.print(lightPercentage); dataFile.println("%");
    dataFile.close();  // Fermer le fichier pour s'assurer que les données sont bien enregistrées
  } else {
    Serial.println("Erreur : Impossible d'écrire dans le fichier datalog.csv !");
  }

  delay(2000);  // Attendre 1 seconde avant de lire à nouveau
}
