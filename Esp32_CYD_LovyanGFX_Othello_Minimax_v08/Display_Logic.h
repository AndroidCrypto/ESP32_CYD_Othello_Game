// --------------------------------------------------------------
// The LittleFS (FLASH file system) is used to hold touch screen calibration data

#include <LittleFS.h>

// --------------------------------------------------------------
// LovyanGFX library

#include <LovyanGFX.hpp>

#include "LovyanGFX_CYD_ILI9341_Settings.h"
//#include "LovyanGFX_CYD_ST7789_Settings.h"
#include "LGFX_TFT_eSPI.h"
LGFX tft;

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The LittleFS file name must start with "/".
#define CALIBRATION_FILE "/CalDatLovyLfsSt7789"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

// Keypad start position, key sizes and spacing
// on a 240 x 240 px screen a box is max 30 px large without any spacing

#define KEY_X 15  // Centre of key
#define KEY_Y 15
#define KEY_W 25  // Width and height
#define KEY_H 25
#define KEY_SPACING_X 5  // X and Y gap
#define KEY_SPACING_Y 5

#define NUMBER_OF_KEYS 64

int keyColor[NUMBER_OF_KEYS];

#define SCREEN_WIDTH 240
#define RESULT_LINE_Y 244
#define RESULT_DATA_Y 250
#define STATUS_DATA_Y 270

// definitions for startup / settings page
#define GAME_NAME_1_Y 10
#define GAME_NAME_2_Y 40
#define GAME_MODE_1_Y 80
#define GAME_MODE_2_Y 140
#define GAME_MODE_21_X 60
#define GAME_MODE_22_X 180
#define GAME_MODE_W 110
#define GAME_MODE_H 40 
//#define GAME_DIFFICULTY_LEVEL_1_Y 170
#define GAME_DIFFICULTY_LEVEL_1_Y 190
#define GAME_DIFFICULTY_LEVEL_2_Y 230
#define GAME_DIFFICULTY_LEVEL_EASY_X 40
#define GAME_DIFFICULTY_LEVEL_MIDLE_X 120
#define GAME_DIFFICULTY_LEVEL_HARD_X 200
#define GAME_DIFFICULTY_LEVEL_W 75
#define GAME_DIFFICULTY_LEVEL_H 40
#define GAME_COPYRIGHT_Y 280

LGFX_Button key[NUMBER_OF_KEYS];

LGFX_Button modeButton[2]; // 0 = Human vs AI, 1 = AI vs AI
LGFX_Button difficultyButton[3]; // 0 = easy, 1 = medium, 2 = hard

int touchedButton = -1;
bool isButtonTouched = false;
bool isWaitingForHumanMove = false;

bool isSettingsMode = true;

//------------------------------------------------------------------------------------------

void touch_calibrate_LovyanGFX() {
  uint16_t calData[8];
  uint8_t calDataOK = 0;

  // debug:
  Serial.printf("calData size: %d\n", sizeof(calData));
  // calData size: 16

  // check if calibration file exists and size is correct
  if (LittleFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL) {
      // Delete if we want to re-calibrate
      LittleFS.remove(CALIBRATION_FILE);
    } else {
      File f = LittleFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 16) == 16)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouchCalibrate(calData);  // LovyanGFX

  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    Serial.println("Your Screen Calibration");
    Serial.printf("uint16_t calData[8] = { %d, %d, %d, %d, %d, %d, %d, %d };\n", calData[0], calData[1], calData[2], calData[3], calData[4], calData[5], calData[6], calData[7]);
    // uint16_t calData[8] = { 3756, 373, 3718, 3827, 314, 370, 248, 3848 };
    //Serial.printf("uint16_t calData[5] = { %d, %d, %d, %d, %d };\n", calData[0], calData[1], calData[2], calData[3], calData[4]);


    // store data
    File f = LittleFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 16);
      f.close();
    }
  }
}

//------------------------------------------------------------------------------------------

void initKeyColors() {
  for(int i = 0; i < NUMBER_OF_KEYS; i++) {
    keyColor[i] = TFT_DARKGREY;
  }
}

void drawKeypad() {

  // Draw the buttons
  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t col = 0; col < 8; col++) {
      uint8_t b = col + row * 8;

      key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
                        KEY_Y + row * (KEY_H + KEY_SPACING_Y),  // x, y, w, h, outline, fill, text
                        KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
                        "", 1);

      key[b].drawButton();
    }
  }
}

void drawGameName() {
  tft.setFont(&fonts::Font0);
  tft.setTextColor(TFT_RED, TFT_DARKGREY);
  tft.setFreeFont(&FreeSansBold18pt7b);
  //tft.setTextSize(2);
  tft.drawCentreString("Reversi", SCREEN_WIDTH / 2, GAME_NAME_1_Y);
  tft.drawCentreString("Othello", SCREEN_WIDTH / 2, GAME_NAME_2_Y);
  tft.setTextColor(TFT_YELLOW, TFT_DARKGREY);
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.drawCentreString("Difficulty", SCREEN_WIDTH / 2, GAME_DIFFICULTY_LEVEL_1_Y);
  tft.drawCentreString(String(DIFFICULTY), SCREEN_WIDTH / 2, GAME_DIFFICULTY_LEVEL_2_Y);
  tft.setTextColor(TFT_GREEN, TFT_DARKGREY);
  tft.drawCentreString("AndroidCrypto", SCREEN_WIDTH / 2, GAME_COPYRIGHT_Y);
}

void drawModeButtons() {
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  //tft.setTextSize(2);
  tft.drawCentreString("Select Mode", SCREEN_WIDTH / 2, GAME_MODE_1_Y);
  tft.setFont(&fonts::Font0);
  modeButton[0].initButton(&tft, GAME_MODE_21_X,
                        GAME_MODE_2_Y,  // x, y, w, h, outline, fill, text
                        GAME_MODE_W, GAME_MODE_H, TFT_WHITE, TFT_BLUE, TFT_WHITE,
                        "Human-AI", 2);
  modeButton[0].drawButton();

  modeButton[1].initButton(&tft, GAME_MODE_22_X,
                        GAME_MODE_2_Y,  // x, y, w, h, outline, fill, text
                        GAME_MODE_W, GAME_MODE_H, TFT_WHITE, TFT_BLUE, TFT_WHITE,
                        "AI - AI", 2);
  modeButton[1].drawButton();
}

void clearStatus() {
  tft.setTextColor(TFT_SKYBLUE, TFT_DARKGREY);
    tft.setTextSize(2);
    tft.setTextWrap(true, true);
    tft.drawString("                                                           ", 5, STATUS_DATA_Y);
}
