#include <OneButton.h>
#include <esp_system.h>

#include "display.hpp"
#include "src/shanshui/shanshui.h"

namespace {

constexpr uint8_t kBtnUpPin = 14;
constexpr uint8_t kBtnDnPin = 12;
constexpr uint32_t kGenTaskStack = 24576;
constexpr uint32_t kSeedFallback = 0x9E3779B9u;
// 每次按键水平视窗移动的世界坐标距离（zoom=1.142 → 约 73 像素）。
constexpr float kPanStep = 64.0f;

uint8_t s_bits[SHANSHUI_BITS_SIZE];
uint8_t s_grayBand[SHANSHUI_BAND_SIZE];
uint32_t s_seed;
float s_cursx = 0.0f;
volatile bool s_genDone = false;
volatile int16_t s_pendingPan = 0;

OneButton s_btnUp(kBtnUpPin, true, true);
OneButton s_btnDn(kBtnDnPin, true, true);

// cursor 增大 → 世界坐标左移 → 画面左移。
void panLeft() { ++s_pendingPan; }
void panRight() { --s_pendingPan; }

void generateScene(void*) {
  shanshui::generateBanded(s_seed, s_grayBand, SHANSHUI_BAND_H, s_bits,
                           shanshui::DITHER_FLOYD, s_cursx);
  s_genDone = true;
  vTaskDelete(nullptr);
}

void renderBitmap() {
  display::wake();

  // 先起生成任务，再画右下角局部 loading（不全屏刷新）。
  // 旧画面一直保留，直到最后 showBitmap() 才全屏刷新一次。
  s_genDone = false;
  uint32_t t0 = millis();
  xTaskCreate(generateScene, "shanshui_gen", kGenTaskStack, nullptr, 1, nullptr);

  for (uint8_t phase = 0; !s_genDone; ++phase) {
    display::drawLoadingFrame(phase);
  }

  Serial.printf("render done: seed=%u cursx=%d algo=floyd %lums\n", s_seed,
                (int)s_cursx, (unsigned long)(millis() - t0));
  display::showBitmap(s_bits);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  display::init();

  s_btnUp.attachPress(panLeft);
  s_btnDn.attachPress(panRight);

  s_seed = esp_random();
  if (s_seed == 0) s_seed = kSeedFallback;
  renderBitmap();
}

void loop() {
  s_btnUp.tick();
  s_btnDn.tick();

  if (s_pendingPan != 0) {
    s_cursx += (float)s_pendingPan * kPanStep;
    s_pendingPan = 0;
    renderBitmap();
  }
}
