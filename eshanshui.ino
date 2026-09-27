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

void renderBitmap() {
  s_genDone = false;
  uint32_t t0 = millis();
  xTaskCreate(generateScene, "shanshui_gen", kGenTaskStack, nullptr, 1, nullptr);

  while (!s_genDone) {
    delay(10);
  }

  Serial.printf("render done: seed=%u algo=floyd %lums\n", s_seed,
                (unsigned long)(millis() - t0));
  display::showBitmap(s_bits);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  display::init();

  s_seed = esp_random();
  if (s_seed == 0) s_seed = kSeedFallback;
  renderBitmap();
}

void loop() {}
