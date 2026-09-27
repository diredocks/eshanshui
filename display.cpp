#include "display.hpp"

#include <Arduino.h>
#include <GxEPD2_BW.h>

#include "src/shanshui/shanshui.h"

namespace display {

namespace {

constexpr uint8_t kEpdCs = 5;
constexpr uint8_t kEpdDc = 17;
constexpr uint8_t kEpdRst = 16;
constexpr uint8_t kEpdBusy = 4;

GxEPD2_BW<GxEPD2_420_M01, GxEPD2_420_M01::HEIGHT> s_display(
    GxEPD2_420_M01(kEpdCs, kEpdDc, kEpdRst, kEpdBusy));

} // namespace

void init() {
  s_display.init(115200, true);
  s_display.setRotation(0);
  Serial.println("display initialized");
}

void showBitmap(const uint8_t* bits) {
  s_display.setFullWindow();
  s_display.firstPage();
  do {
    s_display.fillScreen(GxEPD_WHITE);
    s_display.drawInvertedBitmap(0, 0, bits, SHANSHUI_W, SHANSHUI_H,
                                 GxEPD_BLACK);
  } while (s_display.nextPage());
  Serial.println("epd refresh done");
  s_display.hibernate();
}

} // namespace display
