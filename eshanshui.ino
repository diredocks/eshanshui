#include <esp_system.h>

#include "display.hpp"
#include "src/shanshui/shanshui.h"

namespace {

constexpr uint32_t kGenTaskStack = 24576;
constexpr uint32_t kSeedFallback = 0x9E3779B9u;

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
  display::drawLoadingFrame(0);

  s_seed = esp_random();
  if (s_seed == 0) s_seed = kSeedFallback;
  uint32_t t0 = millis();
  xTaskCreate(generateScene, "shanshui_gen", kGenTaskStack, nullptr, 1, nullptr);

  for (uint8_t phase = 1; !s_genDone; ++phase) {
    Serial.printf("loading frame %u\n", (unsigned)phase & 3);
    display::drawLoadingFrame(phase);
  }

  Serial.printf("generate done: seed=%u algo=floyd %lums\n", s_seed,
                (unsigned long)(millis() - t0));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  display::init();

  generateBitmap();
  display::showBitmap(s_bits);
}

void loop() {}
