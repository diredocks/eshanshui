#include <GxEPD2_BW.h>
#include <esp_system.h>

#include "src/shanshui/shanshui.h"

namespace {

constexpr uint8_t kEpdCs = 5;
constexpr uint8_t kEpdDc = 17;
constexpr uint8_t kEpdRst = 16;
constexpr uint8_t kEpdBusy = 4;
constexpr uint32_t kGenTaskStack = 24576;
constexpr uint32_t kSeedFallback = 0x9E3779B9u;

GxEPD2_BW<GxEPD2_420_M01, GxEPD2_420_M01::HEIGHT> display(
    GxEPD2_420_M01(kEpdCs, kEpdDc, kEpdRst, kEpdBusy));

uint8_t s_bits[SHANSHUI_BITS_SIZE];
uint8_t s_grayBand[SHANSHUI_BAND_SIZE];
uint32_t s_seed;
volatile bool s_genDone = false;

void generateScene(void*) {
  shanshui::generateBanded(s_seed, s_grayBand, SHANSHUI_BAND_H, s_bits,
                           shanshui::DITHER_FLOYD);
  s_genDone = true;
  vTaskDelete(nullptr);
}

void generateBitmap() {
  s_seed = esp_random();
  if (s_seed == 0) s_seed = kSeedFallback;

  uint32_t t0 = millis();
  xTaskCreate(generateScene, "shanshui_gen", kGenTaskStack, nullptr, 1, nullptr);
  while (!s_genDone) delay(10);
  Serial.printf("generate done: seed=%u algo=floyd %lums\n", s_seed,
                (unsigned long)(millis() - t0));
}

void refreshDisplay() {
  display.init(115200);
  display.setRotation(0);
  Serial.println("display initialized");

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0, s_bits, SHANSHUI_W, SHANSHUI_H,
                               GxEPD_BLACK);
  } while (display.nextPage());
  Serial.println("epd refresh done");
  display.hibernate();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  generateBitmap();
  refreshDisplay();
}

void loop() {}
