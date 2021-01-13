// Sources : http://www.lilygo.cn/prod_view.aspx?TypeId=50031&Id=1149

#include <Arduino.h>
// include library, include base class, make path known
#include <wire.h>
#include "SPI.h"
#include <GxEPD.h>

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

#define SDCARD_SS 13
#define SDCARD_CLK 14
#define SDCARD_MOSI 15
#define SDCARD_MISO 2

#define BUTTON_PIN 39

#define TIME_TO_SLEEP  ( 15 * 60 )
#define uS_TO_S_FACTOR 1000000

GxIO_Class io(SPI, /*CS=5*/ ELINK_SS, /*DC=*/ELINK_DC, /*RST=*/ELINK_RESET);
GxEPD_Class display(io, /*RST=*/ELINK_RESET, /*BUSY=*/ELINK_BUSY);

SPIClass sdSPI(VSPI);

int startX = 0, startY = 0;

Adafruit_BMP085 bmp;

void setup_init();
void affichage_pression();

void setup()
{
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    setup_init();

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

    affichage_pression();
    //------------ la suite doit être nettoyée

    // goto sleep
//    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, LOW);
    esp_sleep_enable_timer_wakeup((uint64_t) (TIME_TO_SLEEP * uS_TO_S_FACTOR));
    Serial.printf("Setup ESP32 to sleep for every %d Seconds", TIME_TO_SLEEP);

}

void loop()
{
    static int32_t pression_pre = 0;
    int32_t pression = bmp.readPressure() / 100;

    Serial.println("lecture...");

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
    Serial.flush();
    esp_light_sleep_start();
}

void setup_init()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("setup");
    SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);
    display.init();        // enable diagnostic output on Serial

    if (!bmp.begin())
    {
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (1) {}
    }

    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(0, 0);
}
void affichage_pression()
{
//    display.fillScreen(GxEPD_WHITE);

}
