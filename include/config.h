#pragma once
/******************************************************************************
    BarometrePS                                     Stéphane Lepoutère (c) 2020

    Paramètres et configuration de la carte et des spécifications
*/

#include <Arduino.h>

#define SPI_MOSI 23
#define SPI_MISO -1
#define SPI_CLK 18

#define ELINK_SS 5
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17

#define BUTTON_PIN 39
#define LED_DBG GPIO_NUM_22

//#define TIME_TO_SLEEP  ( 15 * 60 )
#define TIME_TO_SLEEP (60 * 1)
#define uS_TO_S_FACTOR 1000000

#define P_ECHANTILLONS  30
#define P_HISTO         96