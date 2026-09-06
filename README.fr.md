# 🌦️ Système embarqué : Worldwide Weather Watcher

🇬🇧 [English](README.md) | **🇫🇷 Français**

Projet réalisé au **CESI** en deuxième année de cycle préparatoire intégré (2024-2025), dans le cadre du bloc *Système embarqué*.
Ce dépôt contient le **programme complet de la carte** (Livrable 3 – Maquette) : le firmware Arduino d'un prototype de station météo embarquée destinée à équiper des navires, qui relève des mesures environnementales, les horodate et les enregistre sur une carte SD.

> Projet déjà rendu dans le cadre de la formation. **Le code est conservé exactement tel qu'il a été livré en novembre 2024**, commentaires en français compris. Aucun bug n'a été corrigé depuis : les écarts entre le cahier des charges et le comportement réel sont documentés en détail dans la section *Limitations connues*, plutôt que réparés en silence.

> **Note sur la langue :** le code, les commentaires et les messages de la console série sont en français, tels qu'ils ont été écrits à l'époque.

---

## 🎬 Le scénario : l'AIVM et les navires de surveillance

L'**Agence Internationale pour la Vigilance Météorologique (AIVM)** lance un programme ambitieux : déployer sur les océans des navires de surveillance équipés de stations météo embarquées, chargées de mesurer les paramètres qui influent sur la formation des cyclones et autres catastrophes naturelles. À terme, ces navires échangeront leurs données pour anticiper les phénomènes extrêmes.

De nombreuses sociétés de transport naval ont accepté d'équiper leurs bateaux. En contrepartie, les stations doivent être **simples, robustes et pilotables par n'importe quel membre d'équipage** — pas par un ingénieur. Un dirigeant de l'agence confie le prototype à la startup **THS**, dans laquelle notre équipe travaille.

**Problème posé :** comment concevoir, sur un microcontrôleur 8 bits disposant de 32 Ko de mémoire programme, un système qui acquiert en continu six grandeurs physiques, les horodate, les archive, signale ses pannes sans écran, et se laisse reconfigurer par un marin muni d'un simple terminal série ?

---

## 🧭 Conception

Le système a d'abord été modélisé en UML avant d'écrire la moindre ligne de code (Livrable 1 – Analyse du système, septembre 2024). Les diagrammes qui suivent sont ceux du rendu original.

> ⚠️ Ces diagrammes décrivent le système **tel qu'il avait été spécifié**. Le programme finalement livré s'en écarte sur plusieurs points — intervalles, code couleur d'erreur, affichage en maintenance. Les écarts sont listés dans *Limitations connues*.

### Cas d'utilisation

L'environnement est le véritable acteur du système : les capteurs relèvent en permanence, les données sont lues, stockées sur la carte SD, et tout incident se traduit par un changement d'état de la LED.

![Diagramme de cas d'utilisation : l'environnement déclenche le relevé permanent des capteurs, la lecture des données, leur stockage sur carte SD, chaque étape pouvant provoquer un changement d'état de la LED](pictures/uml-cas-utilisation-3.png)

*(Les deux autres cas d'utilisation — démarrage du système et choix du mode par l'équipage — sont dans [`pictures/`](pictures/).)*

### Diagramme de séquence

Vue d'ensemble des quatre modes et de la gestion des erreurs, du démarrage jusqu'aux clignotements de diagnostic.

![Diagramme de séquence couvrant le démarrage en mode standard, le mode configuration, le mode maintenance, le mode économique et les quatre combinaisons de clignotement de la LED en cas d'erreur](pictures/uml-sequence.png)

---

## 🛠️ Fonctionnement

### Les cinq états du système

Le programme est une machine à états pilotée par une variable globale `mode`. La boucle principale (`loop()`) ne fait qu'aiguiller vers la fonction du mode courant ; ce sont les **interruptions matérielles** des deux boutons qui changent d'état.

![Diagramme d'activité de la boucle principale : après l'allumage, mode est initialisé à INIT, la fonction Bouton est appelée, puis une cascade de conditions oriente vers Configuration, Maintenance, Economique ou Standard](pictures/uml-activite-boucle-principale.png)

| Mode | LED | Comment y entrer | Ce qu'il fait |
|---|---|---|---|
| **`INIT`** | éteinte | au démarrage | Fenêtre de quelques secondes pendant laquelle le bouton rouge donne accès à la configuration. Bascule ensuite en `STD` automatiquement. |
| **`STD`** *(standard)* | 🟢 vert continu | par défaut à la fin de `INIT` | Cycle nominal : lecture de tous les capteurs, horodatage, écriture d'une ligne sur la carte SD, puis attente. |
| **`ECO`** *(économique)* | 🔵 bleu continu | bouton **vert** ≥ 5 s | Identique à `STD` mais l'intervalle entre deux acquisitions est **doublé**, pour économiser la batterie. |
| **`CFG`** *(configuration)* | 🟡 jaune continu | bouton **rouge** pendant `INIT` | Ouvre une console sur le port série : l'utilisateur tape des commandes pour changer les seuils et les intervalles, stockés en EEPROM. Sort sur inactivité et repasse en `STD`. |
| **`MNT`** *(maintenance)* | 🟠 orange continu | bouton **rouge** ≥ 5 s depuis `STD` ou `ECO` | Suspend toute écriture sur la carte SD, afin que la carte puisse être **retirée sans risque de corruption**. Un second appui long ramène au mode précédent. |

Chaque mode se réduit à quelques appels. La maintenance n'allume que la LED — c'est précisément le fait de **ne pas** appeler `Recup_data()` qui garantit qu'aucune écriture n'a lieu et que la carte SD peut être extraite.

![Diagramme d'activité des quatre fonctions de mode : Maintenance appelle seulement LED, Economique et Standard appellent LED puis Recup_data, Configuration boucle sur la saisie série jusqu'à 30 minutes d'inactivité avant de repasser en mode standard](pictures/uml-activite-modes.png)

### Les deux boutons

Les boutons sont câblés en `INPUT_PULLUP` et lus par **interruption matérielle** sur front changeant (`attachInterrupt(..., CHANGE)`). À chaque front, le programme mémorise `millis()` puis, au relâchement, calcule la durée d'appui : c'est ainsi qu'un appui court se distingue d'un appui long sans jamais bloquer la boucle principale.

| Bouton | Broche | Appui court | Appui long (≥ 5 s) |
|---|---|---|---|
| 🔴 **Rouge** | D3 (`INT1`) | `INIT` → `CFG` | `STD`/`ECO` ↔ `MNT` |
| 🟢 **Vert** | D2 (`INT0`) | — | `STD` ↔ `ECO` |

![Diagramme d'activité de la fonction Bouton : le bouton rouge en mode INIT bascule en CFG, un appui de 5 secondes sur le bouton rouge fait entrer ou sortir du mode MNT selon le mode précédent, et un appui de 5 secondes sur le bouton vert bascule entre STD et ECO](pictures/uml-activite-boutons.png)

> ℹ️ **Un écart assumé par rapport au sujet.** Le cahier des charges prévoyait le bouton rouge à la fois pour *entrer* en maintenance et pour *quitter* le mode économique. Le programme ne pouvait pas distinguer les deux intentions avec le même bouton et la même durée. L'équipe a donc réattribué la sortie du mode économique au bouton vert. Ce choix est justifié dans le Livrable 1.

### Codes couleur de la LED

La station n'a **aucun écran**. La LED RGB chaînable est le seul canal de diagnostic sur le pont : une couleur fixe indique le mode, une **alternance de deux couleurs** signale une panne. Tant qu'une erreur est présente, elle a priorité sur la couleur du mode.

| Signal | Signification |
|---|---|
| 🟢 vert fixe | Mode standard, tout va bien |
| 🔵 bleu fixe | Mode économique |
| 🟡 jaune fixe | Mode configuration |
| 🟠 orange fixe | Mode maintenance |
| 🔴 ↔ 🔵 | Erreur d'accès à l'horloge RTC |
| 🔴 ↔ 🟡 | Erreur d'accès au GPS |
| 🔴 ↔ 🟡 | Erreur d'accès aux capteurs — motif identique à l'erreur GPS, voir *Limitations connues* |
| 🔴 ↔ ⚪ | Erreur d'accès à la carte SD (absente, illisible ou pleine) |
| 🔴 ↔ ⚪ *(cadence lente)* | Erreur d'écriture sur la carte SD |

La fonction `led()` teste les erreurs en cascade, de la plus critique à la moins critique, avant de retomber sur la couleur du mode courant.

![Diagramme d'activité de la fonction LED : une cascade de conditions teste successivement RTC_ERROR, GPS_ERROR, CAP_ERROR, VAL_ERROR, DATA_ERROR et WRITE_ERROR, chacune associée à un clignotement bicolore, puis à défaut allume la LED verte, bleue, jaune ou orange selon le mode](pictures/uml-activite-led.png)

### Les grandeurs mesurées

| Grandeur | Composant | Interface | Broche |
|---|---|---|---|
| Température de l'air | BME280 | I²C (`0x76`) | A4 / A5 |
| Pression atmosphérique | BME280 | I²C (`0x76`) | A4 / A5 |
| Hygrométrie | BME280 | I²C (`0x76`) | A4 / A5 |
| Luminosité | Photorésistance | Analogique | A0 |
| Position | Module GPS | UART logiciel, 9600 bauds | D8 / D9 |
| Date et heure | DS3231 | I²C | A4 / A5 |

La fonction `Recup_data()` vérifie d'abord la présence de chaque périphérique — chaque échec renseigne la variable `error` exploitée par la LED — puis lit les capteurs, écrit la ligne sur la carte SD et applique le délai d'attente, doublé en mode économique.

![Diagramme d'activité de Recup_data : error est remis à NO_ERROR, puis la présence de l'horloge, du GPS, des capteurs, la cohérence des valeurs, la présence de la carte SD et la possibilité d'écrire sont testées successivement, avant la récupération des données, l'écriture sur la carte SD et le choix du délai selon le mode](pictures/uml-activite-recup-data.png)

### Le fichier de données

Chaque cycle ajoute une ligne au fichier `data.csv` à la racine de la carte SD. Les en-têtes sont écrits une seule fois, à la création du fichier.

```csv
Date;Heure;Temp;Press;Hum;Lum;Lat;Long
11/4/2024; 23:9:0; 21.53C;1013.42 hPa;48.20%;62.75%;43°28'52.4"N; 5°23'11.0"E;
```

Le point-virgule est utilisé comme séparateur pour que le fichier s'ouvre directement dans Excel en configuration française.

### Le mode configuration

Une fois en `CFG`, la station attend des commandes sur le port série à **9600 bauds**, au format `PARAMETRE=valeur`, une par ligne. Chaque commande est acquittée par un écho.

| Commande | Rôle | Valeur par défaut |
|---|---|---|
| `LOG_INTERVALL=` | Intervalle entre deux acquisitions | `1000` |
| `TIMEOUT=` | Délai d'inactivité avant sortie du mode `CFG` | `30` |
| `LUMIN=` | Active (1) ou désactive (0) le capteur de luminosité | `1` |
| `LUMIN_LOW=` / `LUMIN_HIGH=` | Seuils bas / haut de luminosité | `255` / `768` |
| `TEMP_AIR=` | Active (1) ou désactive (0) le capteur de température | `1` |
| `MIN_TEMP_AIR=` / `MAX_TEMP_AIR=` | Seuils de température de l'air (°C) | `-10` / `60` |
| `HYGR=` | Active (1) ou désactive (0) l'hygrométrie | `1` |
| `HYGR_MINT=` / `HYGR_MAXT=` | Plage de température de validité de l'hygrométrie (°C) | `0` / `50` |
| `PRESSURE=` | Active (1) ou désactive (0) le capteur de pression | `1` |
| `PRESSURE_MIN=` / `PRESSURE_MAX=` | Seuils de pression (hPa) | `850` / `1080` |
| `RESET` | Restaure toutes les valeurs par défaut | — |

> ⚠️ Ces commandes sont **acceptées et écrites en EEPROM, mais elles n'ont aucun effet sur le comportement de la station**. Voir *Limitations connues* — c'est la principale fonctionnalité inachevée du projet.

---

## 🔌 Matériel et câblage

La station — baptisée **NomadWeather** dans le manuel d'utilisation rédigé par l'équipe — est un empilement de trois cartes : un **Arduino Uno** (AVR ATmega328P) à la base, un **shield pour carte SD** au-dessus, et un **Grove Base Shield** au sommet, l'ensemble clipsé dans un support transparent. Chaque périphérique se branche ensuite sur le shield Grove par un câble détrompé à quatre fils, ce qui rend le montage entièrement sans soudure.

L'alimentation se fait en USB, depuis une batterie externe ou depuis un ordinateur.

### Contenu de la boîte

| # | Élément | | # | Élément |
|---|---|---|---|---|
| 1 | Shield pour connecteurs Grove | | 7 | Horloge RTC |
| 2 | Carte SD | | 8 | LED RGB |
| 3 | Shield Arduino pour carte SD | | 9 | Capteur de pression, humidité et température |
| 4 | Carte Arduino | | 10 | Boutons poussoirs |
| 5 | Support de carte Arduino | | 11 | GPS |
| 6 | Capteur de lumière | | | |

### Le prototype

![Photo du prototype démonté posé sur une table en bois : le Grove Base Shield au centre, avec des câbles Grove à quatre fils rayonnant vers la LED RGB chaînable, le module à deux boutons, les cartes capteurs et un connecteur laissé libre](pictures/montage.jpg)

La photo ci-dessus montre le prototype réellement utilisé pour la démonstration du Livrable 3, désolidarisé de la carte Arduino.

### Câblage

![Schéma de câblage : l'Arduino Uno au centre, relié à gauche au BME280 et au DS3231 par le bus I²C sur A4 et A5, à la photorésistance sur A0 et au module GPS sur D8 et D9 ; à droite au lecteur de carte SD sur D4 et le bus SPI, à la LED RGB sur D5 et D6, et aux deux boutons poussoirs sur D2 et D3](pictures/cablage.svg)

Six périphériques, six connecteurs Grove. Un connecteur Grove numérique porte **deux broches consécutives**, ce qui explique pourquoi le firmware les déclare par paires : brancher les boutons sur `D2` donne D2 *et* D3, la LED sur `D5` donne D5 *et* D6, le GPS sur `D8` donne D8 *et* D9.

| Périphérique | Connecteur Grove | Broches utilisées par le firmware | Interface |
|---|---|---|---|
| Boutons poussoirs (vert + rouge) | `D2` | D2 (vert), D3 (rouge) | Numérique, interruptions `INT0` / `INT1` |
| LED RGB chaînable | `D5` | D5 (données), D6 (horloge) | 2-wire |
| Module GPS | `D8` | D8 (RX), D9 (TX) | UART logiciel |
| Capteur de luminosité | `A0` | A0 | Analogique |
| BME280 | `I²C` | A4 (SDA), A5 (SCL) | I²C |
| Horloge RTC DS3231 | `I²C` | A4 (SDA), A5 (SCL) | I²C |
| Lecteur de carte SD | — *(shield dédié)* | D4 (CS), D11–D13 | SPI |

Le lecteur de carte SD est le seul périphérique qui ne soit pas un module Grove : il occupe son propre shield et monopolise le bus SPI matériel, avec D4 comme sélection de puce.

> ⚠️ Les deux shields s'emboîtent à force sur des broches qui se tordent facilement, et les six modules doivent être branchés exactement sur les bons connecteurs. Le manuel d'utilisation y consacre neuf étapes illustrées et deux avertissements distincts.

### Architecture des composants

Le diagramme de composants du Livrable 1 montre les dépendances entre les éléments : la carte Arduino est au centre, tout dépend d'elle, et elle-même ne dépend que de la batterie.

![Diagramme de composants : la carte Arduino au centre dépend de la batterie ; l'horloge RTC, le lecteur de carte SD, les boutons et la famille des capteurs s'y rattachent, la carte SD dépendant du lecteur et la LED étant pilotable par les boutons](pictures/uml-composants.png)

Le sujet prévoyait d'intégrer par la suite quatre modules tiers — température de l'eau, force du courant marin, force du vent, taux de particules fines. Ils apparaissent en rose sur le diagramme, à gauche : **ils n'ont pas été implémentés**, faute de mémoire disponible sur la carte.

---

## 🚀 Installation et utilisation

> **Prérequis :** [`arduino-cli`](https://arduino.github.io/arduino-cli/) ou l'Arduino IDE 2.x. Si vous avez déjà l'IDE, inutile d'installer quoi que ce soit de plus : `arduino-cli` y est embarqué, dans `resources\app\lib\backend\resources\`.

### 1. Cloner le dépôt

```bash
git clone https://github.com/Estebge/Systeme-embarque.git
cd Systeme-embarque
```

### 2. Installer les dépendances

```bash
arduino-cli core install arduino:avr
arduino-cli lib install "Adafruit BME280 Library" "RTClib" "Grove - Chainable RGB LED" "SD"
```

`Adafruit BusIO` et `Adafruit Unified Sensor` sont tirées automatiquement comme dépendances. `EEPROM`, `Wire`, `SPI` et `SoftwareSerial` sont fournies avec le cœur AVR.

### 3. Compiler

```bash
arduino-cli compile --fqbn arduino:avr:uno StationMeteo
```

### 4. Téléverser et observer

```bash
arduino-cli board list                                        # repérer le port, ex. COM3
arduino-cli upload -p COM3 --fqbn arduino:avr:uno StationMeteo
arduino-cli monitor -p COM3 -c baudrate=9600
```

Le moniteur série à **9600 bauds** est le seul canal de dialogue avec la station : il affiche l'état de la carte SD (`Carte OK`, `Fichier créé`, `Écriture des données`), les trames GPS reçues, et c'est là que se tapent les commandes du mode configuration.

Depuis l'Arduino IDE, le chemin est le même : ouvrir `StationMeteo/StationMeteo.ino`, puis *Croquis → Vérifier*, *Croquis → Téléverser*, *Outils → Moniteur série*.

---

## 🧪 Tester sans le matériel

Autant l'annoncer franchement : **sans les composants câblés, on ne peut pas faire tourner cette station.** Le programme interroge le BME280, le DS3231 et le lecteur SD dès son premier cycle ; sans eux il lève des erreurs et fait clignoter la LED, sans rien mesurer ni enregistrer.

Ce qui reste possible sans aucun matériel :

| Ce que vous pouvez faire | Ce que ça prouve |
|---|---|
| **Compiler** (étapes 1 à 3 ci-dessus) | Que les dépendances se résolvent et que le firmware tient dans la mémoire de l'Uno |
| **Lire le rapport de taille** produit par la compilation | L'empreinte mémoire réelle — voir la section suivante |
| **Lire le code et cette documentation** | Le fonctionnement de la machine à états, sans avoir à l'exécuter |

Avec un Arduino Uno nu, sans aucun capteur, il est également possible de téléverser le firmware et d'observer sur le moniteur série la séquence de démarrage et les erreurs de détection des périphériques absents. C'est utile pour vérifier une chaîne de téléversement, pas pour valider la station.

> ℹ️ **Aucun `data.csv` d'exemple n'est fourni.** Les relevés sont restés sur la carte SD de la station et n'ont jamais été copiés avant la restitution du matériel. Le format du fichier est documenté plus haut, mais il n'existe pas de capture authentique à montrer.

---

## 💾 Empreinte mémoire

C'est **la** contrainte structurante du projet, et la raison directe de la plupart des limitations listées plus bas. L'ATmega328P de l'Uno dispose de 32 Ko de mémoire programme et de 2 Ko de RAM.

| Carte | Mémoire programme | RAM statique | Marge restante |
|---|---|---|---|
| **Arduino Uno** *(cible du projet)* | 30 312 / 32 256 o → **93 %** ⚠️ | 1 513 / 2 048 o → **73 %** | **535 o** pour la pile *et* le tas |
| Arduino Mega 2560 | 31 956 / 253 952 o → 12 % ✅ | 1 525 / 8 192 o → 18 % | 6 667 o |

*(Mesures relevées à la compilation ; l'occupation de la mémoire programme varie de quelques dizaines d'octets selon les versions de bibliothèques.)*

Sur l'Uno, il restait donc **environ 2 Ko de mémoire programme** pour implémenter tout ce qui manquait. Les 535 octets de RAM libres sont d'autant plus critiques que le programme manipule des objets `String`, qui allouent dynamiquement : c'est la configuration typique dans laquelle un Uno devient instable.

---

## 📦 Dépendances

| Bibliothèque | Version validée | Rôle |
|---|---|---|
| `Adafruit BME280 Library` | 2.3.0 | Température, pression et humidité |
| `Adafruit BusIO` | 1.17.1 | Couche d'abstraction I²C/SPI *(dépendance)* |
| `Adafruit Unified Sensor` | 1.1.15 | Interface commune aux capteurs *(dépendance)* |
| `RTClib` | 2.1.4 | Horloge temps réel DS3231 |
| `Grove - Chainable RGB LED` | 1.0.0 | LED RGB chaînable |
| `SD` | 1.3.0 | Système de fichiers FAT sur carte SD |
| `EEPROM`, `Wire`, `SPI`, `SoftwareSerial` | — | Fournies avec le cœur `arduino:avr` |

---

## ⚠️ Limitations connues

Cette section est volontairement détaillée. Le projet a été rendu et évalué dans cet état ; ce qui suit décrit honnêtement l'écart entre ce qui était demandé et ce que le programme fait réellement.

### La cause principale : la mémoire, et les pointeurs

Le cahier des charges demandait un système de configuration persistante : l'utilisateur règle des seuils depuis la console, ces valeurs sont écrites en EEPROM, relues au démarrage, et le programme s'appuie dessus pour déclencher des alertes et cadencer ses acquisitions.

**La moitié de cette chaîne n'a jamais été branchée.** L'écriture en EEPROM fonctionne ; la relecture, le stockage dans les structures de données et l'exploitation par le code métier n'ont pas été terminés. La raison est double, et l'équipe l'assume :

- **L'équipe n'a pas suffisamment maîtrisé les pointeurs et le passage de paramètres par référence** dans le temps imparti. Manipuler des structures imbriquées, les passer à des fonctions sans les recopier et les remplir depuis l'EEPROM demandait une aisance que nous n'avions pas encore acquise. C'était le premier semestre où nous abordions ces notions, et elles sont restées à consolider à la fin du bloc.
- **Il ne restait pratiquement plus de place.** À 93 % de mémoire programme occupée, le budget disponible pour ajouter cette couche était d'environ 2 Ko. Chaque tentative se heurtait à la taille du binaire.

Le résultat est un ensemble de fonctionnalités présentes en façade — les commandes existent, elles répondent, elles écrivent — mais sans effet réel.

### Écarts entre le cahier des charges et le programme livré

| Fonctionnalité attendue | État réel |
|---|---|
| Configuration relue depuis l'EEPROM et appliquée | ❌ **Non implémentée.** Aucun `EEPROM.read()` ni `EEPROM.get()` n'existe dans le programme : la configuration est en écriture seule. |
| Seuils min/max déclenchant une alerte | ❌ **Non implémentés.** Le contrôle de dépassement est présent mais commenté dans le code, et l'erreur correspondante n'est jamais levée. Une version intermédiaire du fichier portait le commentaire « *Ne fonctionne pas, compliqué* ». |
| Activation/désactivation individuelle des capteurs | ❌ **Non implémentée.** Tous les capteurs sont lus systématiquement, quels que soient les indicateurs configurés. |
| Décodage des trames GPS (latitude/longitude réelles) | ❌ **Non implémenté.** Le programme vérifie qu'il reçoit des trames NMEA et les affiche, mais ne les décode pas. **Les coordonnées écrites dans le CSV sont constantes**, codées en dur (celles du campus). Le code de décodage figure en commentaire : il n'a jamais pu être testé faute de réception satellite en salle, et il repose sur un tableau de 15 objets `String` qui ne tiendrait pas dans les 535 octets de RAM disponibles. |
| Intervalle d'acquisition de 10 minutes en mode standard | ⚠️ **Valeur non conforme.** L'intervalle réel est de 1 s en `STD` et 2 s en `ECO`, valeurs de mise au point restées en place. Le doublement en mode économique fonctionne bien, mais sur ces valeurs-là. |
| Mode économique : une acquisition GPS sur deux, capteurs alternés | ⚠️ **Partiel.** Seul le doublement de l'intervalle est implémenté ; aucune alternance de capteurs. |
| Mode maintenance : affichage des mesures en temps réel | ⚠️ **Partiel.** La suspension des écritures sur la carte SD — l'objectif principal du mode, qui permet de retirer la carte sans risque — fonctionne. L'affichage temps réel des mesures, non. |
| Délai d'inactivité de 30 minutes en mode configuration | ⚠️ **Non conforme et défectueux.** Le délai codé est de 30 secondes, et il est calculé depuis le démarrage de la carte et non depuis la dernière commande reçue : passé 30 s de fonctionnement, le mode configuration se referme immédiatement. Le paramètre `TIMEOUT` n'est jamais consulté. |
| Rotation des fichiers d'archivage | ❌ **Non implémentée.** Un unique fichier `data.csv` croît indéfiniment, sans limite de taille ni archivage par date. |
| Modules tiers (eau, courant, vent, particules) | ❌ **Non implémentés**, faute de mémoire. |

### Défauts du code livré

Relevés lors de la relecture pour cette publication. **Ils n'ont volontairement pas été corrigés**, afin que le dépôt reflète le rendu réel.

| Défaut | Conséquence |
|---|---|
| `rtc.adjust(...)` est appelé à **chaque cycle**, en dehors du test `if (rtc.lostPower())` qui le précède | L'horloge est remise à l'heure de compilation en permanence : **les horodatages du CSV n'avancent jamais**. |
| Le délai d'acquisition utilise l'**adresse** EEPROM (`ADDR_LOG_INTERVAL`, qui vaut 0) au lieu de la valeur stockée | La commande `LOG_INTERVALL=` n'a aucun effet ; le programme retombe systématiquement sur la valeur par défaut. |
| Les adresses EEPROM sont espacées d'un octet, alors que `EEPROM.put()` en écrit **deux** pour un `int` | Les paramètres se chevauchent et s'écrasent mutuellement en cascade. Aucune valeur stockée n'est fiable. |
| `DEFAULT_LOG_INTERVAL` vaut 1000 mais est stocké dans un `uint8_t` | Débordement silencieux : la valeur devient 232. |
| Les variables partagées entre les interruptions et la boucle principale ne sont pas déclarées `volatile` | Le compilateur peut les mettre en cache ; les lectures de variables `long` ne sont pas atomiques. |
| Aucun anti-rebond sur les boutons | Un rebond mécanique génère plusieurs interruptions et désynchronise la machine à états. |
| `bme.begin()`, `SD.begin()` et `rtc.begin()` sont appelés à chaque itération | Ces opérations d'initialisation sont rejouées en boucle, ce qui est coûteux et rend l'accès à la carte SD instable. |
| `led()` est appelée **avant** `Recup_data()` dans chaque mode | La LED affiche toujours l'état d'erreur du cycle précédent. |
| La variable `error` est écrasée à chaque test successif | Seule la dernière erreur rencontrée survit ; les autres sont masquées. |
| L'erreur GPS est levée dès que le tampon série est vide | Or un GPS n'émet qu'une rafale par seconde : l'erreur est presque toujours un faux positif. Le tampon de 64 octets de `SoftwareSerial` déborde par ailleurs pendant les `delay()`. |
| Six `delay(100)` sont intercalés au milieu de l'écriture du CSV | 600 ms de blocage par enregistrement, sans utilité. |
| Le code couleur d'erreur capteur est rouge ↔ jaune | Le cahier des charges spécifiait rouge ↔ vert ; il est donc indiscernable de l'erreur GPS. |
| Le mode maintenance n'appelle jamais `Recup_data()` | La variable `error` reste figée : la LED continue d'afficher une ancienne erreur au lieu de l'orange du mode. |
| Les unités sont accolées aux valeurs dans le CSV (`21.53C`, `1013.42 hPa`) | Le fichier s'ouvre dans Excel mais les colonnes ne sont pas directement exploitables numériquement. La date est au format américain, sans zéros de remplissage. |

---

## 📁 Structure du dépôt

```
Systeme-embarque/
├── README.md                  ← anglais
├── README.fr.md               ← français
├── StationMeteo/
│   └── StationMeteo.ino       ← le firmware complet
└── pictures/
    ├── montage.jpg            ← photo du prototype
    ├── cablage.svg            ← schéma de câblage
    ├── uml-cas-utilisation-1..3.png
    ├── uml-sequence.png
    ├── uml-activite-boucle-principale.png
    ├── uml-activite-boutons.png
    ├── uml-activite-modes.png
    ├── uml-activite-recup-data.png
    ├── uml-activite-led.png
    └── uml-composants.png
```

Les diagrammes UML proviennent du **Livrable 1 – Analyse du système** (septembre 2024). La photo provient du support de soutenance du Livrable 3. Le schéma de câblage a été reconstitué à partir des affectations de broches déclarées dans le firmware.

> Le fichier s'appelait `LED.ino` à l'origine, du nom du tout premier essai du projet. Arduino impose qu'un croquis porte le nom de son dossier parent : il a donc été déplacé dans `StationMeteo/` pour que le dépôt compile directement après un clone. **Le contenu du fichier n'a pas été modifié d'une seule ligne.**

---

## 👥 Auteurs

Projet réalisé au **CESI** (2024-2025) par :

- **Estéban** : https://github.com/Estebge
- **Trystan** : https://github.com/trystanbouzonrueda
- **Maxime** : https://github.com/haguetm
- **Yanis** : https://github.com/yyyanis
- **Rayene** : https://github.com/rayene00
