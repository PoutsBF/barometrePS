/******************************************************************************
    BarometrePS                                     Stéphane Lepoutère (c) 2020

    Fait
        Lecture de la pression
        Affichage sur écran paperScreen avec graphe si changement
    A faire
        Affichage de l'heure
        Affichage tendance
*/

// Sources : http://www.lilygo.cn/prod_view.aspx?TypeId=50031&Id=1149

#include <Arduino.h>
#include <config.h>

// include library, include base class, make path known
#include <wire.h>
#include "SPI.h"
#include <GxEPD.h>
#include "include/time.h"

// include library, gestion WIFI, fichiers systèmes et json
#include "WifiConfig.h"

#include <wifi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <esp32Ping.h>

// Include capteur de pression
#include <Adafruit_BMP085.h>

#include <GxGDEH0213B73/GxGDEH0213B73.h>        // 2.13" b/w newer panel

extern const unsigned char posAiguille[][2];

int bmpWidth = 250, bmpHeight = 122;
extern const unsigned char lilygo[];
// FreeFonts from Adafruit_GFX
//#include <Fonts/FreeMonoBold12pt7b.h>
//#include <Fonts/FreeMonoBold18pt7b.h>
//#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

#include <GxIO/GxIO.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>

GxIO_Class io(SPI, /*CS=5*/ ELINK_SS, /*DC=*/ELINK_DC, /*RST=*/ELINK_RESET);
GxEPD_Class display(io, /*RST=*/ELINK_RESET, /*BUSY=*/ELINK_BUSY);

SPIClass sdSPI(VSPI);

int startX = 0, startY = 0;

// définition du capteur de pression
Adafruit_BMP085 bmp;

// Définition des configuration du WIFI
// ----- création des objets serveurs et paramètres ---
const char *ssid = YOUR_WIFI_SSID;
const char *password = YOUR_WIFI_PASSWD;

// Configuration du serveur NTP
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char t_jour[7][5] =
    {"dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."};

const char t_mois[12][5] =
    {"jan.", "fev.", "mar.", "avr.", "mai ", "juin", "jui.", "aou.", "sep.", "oct.", "nov.", "dec."};

// Protypes des fonctions
void setup_init();
void initWiFi();

//-----------------------------------------------------------------------------
// SETUP ----------------------------------------------------------------------
void setup()
{
    // Lecture de l'origine du reset
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    // Configuration du port série, du port SPI, de l'écran
    setup_init();

    // affichage de l'origine du reset
    switch (wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
        default: 
        {
          Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
        } break;
    }

    // Configuration paramètres wake up
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, LOW);
    esp_sleep_enable_timer_wakeup((uint64_t) (TIME_TO_SLEEP * uS_TO_S_FACTOR));
    Serial.printf("Setup ESP32 to sleep for every %d Seconds\n", TIME_TO_SLEEP);

    timeClient.setTimeOffset(3600);
}

//-----------------------------------------------------------------------------
// LOOP -----------------------------------------------------------------------
void loop()
{
    static int32_t pression_pre = 0;
    static int32_t pressions[P_ECHANTILLONS] =
        {102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000,
         102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000,
         102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000, 102000};
    static int32_t pressions_historique[P_HISTO] =
        {104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
          94,  94,  94,  94,  94,  94,  94,  94,  94,  94,
          84,  84,  84,  84,  84,  84,  84,  84,  84,  84,
          74,  74,  74,  74,  74,  74,  74,  74,  74,  74,
          64,  64,  64,  64,  64,  64,  64,  64,  64,  64,
          74,  74,  74,  74,  74,  74,  74,  74,  74,  74,
          84,  84,  84,  84,  84,  84,  84,  84,  84,  84,
          94,  94,  94,  94,  94,  94,  94,  94,  94,  94,
          55,  55,  55,  55,  55,  55,  55,  55,  55,  55,
         104, 104, 104, 104, 104, 104};
    static int8_t pression_pos = 1; // première moyenne 

    Serial.println("lecture...");
    int32_t pression = bmp.readPressure();
    pression_pos--;
    pressions[pression_pos] = pression;

    pression /= 100;
    Serial.printf("pression : %d\n", pression);

    timeClient.begin();
    Serial.println("lecture de l'heure : ");

    bool success = Ping.ping("192.168.0.2", 3);

    if (!success)
    {
        Serial.println("Ping failed");
    }
    else
    {
        Serial.println("Ping succesful.");
    }

    while (!timeClient.update())
    {
//        timeClient.forceUpdate();
        Serial.print(".");
        delay(500);
    }

    Serial.printf("\n%s : \n", timeClient.getFormattedTime().c_str());

    if(pression != pression_pre)
    {
        pression_pre = pression;
        display.drawBitmap(lilygo, startX, startY, bmpWidth, bmpHeight, GxEPD_BLACK);

        //    display.setTextColor(GxEPD_BLACK);
        display.setCursor(155, 22);
        display.print("pression : ");
        Serial.printf("Pression : %dhPa\n", pression);
        display.setCursor(158, 35);
        display.printf("%4d hPa", pression);
        pression -= 950;
        pression <<= 1;
        pression /= 3;

        display.drawLine(90, 90, posAiguille[pression][0], posAiguille[pression][1], GxEPD_BLACK);

        display.setCursor(95, 98);
        display.print("Pout's");
        display.setCursor(80, 113);
        display.print("Family");
    }

    tm var_time;
    time_t epochTime = timeClient.getEpochTime();
    gmtime_r(&epochTime, &var_time);
    display.setCursor(139, 10);
    display.printf("%s%d", t_jour[timeClient.getDay()], var_time.tm_mday);
    display.setCursor(204, 10);
    display.print(t_mois[var_time.tm_mon]);
    Serial.printf("%s %d %s", t_jour[timeClient.getDay()], var_time.tm_mday, t_mois[var_time.tm_mon]);

    //-------------------------------------------------------------------------
    // Gestion de l'historique

    if(pression_pos == 0)
    {
        // Repart l'échantillonne à 0
        pression_pos = P_ECHANTILLONS;

        // Calculer la moyenne
        uint32_t pression_moyenne = 0;

        for (uint8_t x = 0; x < P_ECHANTILLONS; x++)
        {
            pression_moyenne += pressions[x];
        }
        Serial.printf(" pression cumulée : %d", pression_moyenne);
//        pression_moyenne >>= 5;           // A améliorer pour gérer les divisions par 2
        pression_moyenne /= 6000;
        Serial.printf(" / pression moyenne : %d", pression_moyenne);
        // Transformer en position y
        pression_moyenne = (104 - (pression_moyenne - 475));
        Serial.printf(" / position écran : %d\n", pression_moyenne);

        // Décaler le tableau d'historique et ajouter la nouvelle valeur
        for (uint8_t x = 1; x < P_HISTO; x++)
        {
            pressions_historique[x - 1] = pressions_historique[x];
        }
        pressions_historique[0] = pression_moyenne;

        // Affichage des valeurs dans le tableau, le 0 à droite

        display.drawRect(151, 44, 98, 62, GxEPD_BLACK);
        display.fillRect(152, 45, 96, 60, GxEPD_WHITE);
        for (uint8_t x = 0; x < P_HISTO; x++)
        {
            display.drawLine(248 - x, pressions_historique[x], 248 - x, 104, GxEPD_BLACK);
        }
    }

    display.update();
    delay(1000);
    Serial.println("veille...");
    Serial.flush();

    // Endort le wifi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    esp_light_sleep_start();

    Serial.printf("Statut WIFI : %d connecté : %d\n", WiFi.status(), WiFi.isConnected());

    WiFi.mode(WIFI_STA);
    WiFi.reconnect();
    Serial.printf("Trying to connect [%s] \n", WiFi.macAddress().c_str());

    while ((WiFi.status() != WL_CONNECTED))        // && ((millis() - tempo) >= 10000)) // attente max 10 secondes
    {
        Serial.print(".");
        delay(500);
    }
    Serial.printf("\nconnecté : %s / %s\n", WiFi.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str());
    Serial.printf("Statut WIFI : %d connecté : %d\n", WiFi.status(), WiFi.isConnected());

    timeClient.end();
}

void setup_init()
{
    pinMode(LED_DBG, OUTPUT);
    digitalWrite(LED_DBG, LOW);
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    Serial.println();
    Serial.println("Début de la configuration...");
    SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);
    display.init();        // enable diagnostic output on Serial

    if (!bmp.begin())
    {
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (1) 
        {
            digitalWrite(LED_DBG, (millis() % 200) < 50);
        }
    }

    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMono9pt7b);
    display.setCursor(0, 0);

    initWiFi();

    digitalWrite(LED_DBG, HIGH);
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------
void initWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.printf("Trying to connect [%s] \n", WiFi.macAddress().c_str());

    while ((WiFi.status() != WL_CONNECTED))// && ((millis() - tempo) >= 10000)) // attente max 10 secondes
    {
        Serial.print(".");
        delay(500);
    }
    Serial.printf("\nconnecté : %s / %s\n", WiFi.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str());

    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

