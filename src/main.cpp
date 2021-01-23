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

#include <ArduinoOTA.h>

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
#include <Fonts/FreeMonoBold9pt7b.h>

#include <GxIO/GxIO.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>

#define SPI_MOSI 23
#define SPI_MISO -1
#define SPI_CLK 18

#define ELINK_SS 5
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17

#define BUTTON_PIN  39
#define LED_DBG     GPIO_NUM_22

//#define TIME_TO_SLEEP  ( 15 * 60 )
#define TIME_TO_SLEEP (60 * 10)
#define uS_TO_S_FACTOR 1000000

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
//    struct tm timeinfo;

    Serial.println("lecture...");
    int32_t pression = bmp.readPressure() / 100;
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
        display.setCursor(150, 75);
        display.print("pression : ");
        Serial.printf("Pression : %dhPa\n", pression);
        display.setCursor(150, 91);
        display.printf("%d hPa", pression);
        pression -= 950;
        pression <<= 1;
        pression /= 3;

        display.drawLine(90, 90, posAiguille[pression][0], posAiguille[pression][1], GxEPD_BLACK);

        display.setCursor(40, 111);
        display.println("Pout's Family");

        display.update();
    }
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
    display.setFont(&FreeMonoBold9pt7b);
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
    unsigned long tempo = millis();
    while ((WiFi.status() != WL_CONNECTED))// && ((millis() - tempo) >= 10000)) // attente max 10 secondes
    {
        Serial.print(".");
        delay(500);
    }
    Serial.printf("\nconnecté : %s / %s\n", WiFi.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str());

    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

