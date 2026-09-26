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
constexpr int16_t kLoadingRadius = 14;
constexpr int16_t kLoadingThickness = 3;
constexpr int16_t kLoadingMargin = 10;
constexpr int16_t kLoadingPad = 2;
// 3/4 圆环，缺口依次位于右上/右下/左下/左上（顺时针转）。
constexpr uint8_t kLoadingArcs[4] = {0x1 | 0x4 | 0x8, 0x1 | 0x2 | 0x8,
                                     0x1 | 0x2 | 0x4, 0x2 | 0x4 | 0x8};

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

void drawLoadingFrame(uint8_t phase);

void generateBitmap() {
  s_seed = esp_random();
  if (s_seed == 0) s_seed = kSeedFallback;

  drawLoadingFrame(0);  // 首次为整屏刷新：白底 + 起始图标

  uint32_t t0 = millis();
  xTaskCreate(generateScene, "shanshui_gen", kGenTaskStack, nullptr, 1, nullptr);
  uint8_t phase = 1;
  // 生成期间持续转动；生成过快时也要转满一圈再出图。
  while (!s_genDone || (phase & 3) != 0) {
    Serial.printf("loading frame %u\n", (unsigned)phase & 3);
    drawLoadingFrame(phase++);
  }
  Serial.printf("generate done: seed=%u algo=floyd %lums\n", s_seed,
                (unsigned long)(millis() - t0));
}

void drawLoadingIcon(uint8_t arc) {
  const int16_t cx = SHANSHUI_W - kLoadingMargin - kLoadingRadius;
  const int16_t cy = SHANSHUI_H - kLoadingMargin - kLoadingRadius;
  for (int16_t r = kLoadingRadius; r > kLoadingRadius - kLoadingThickness;
       --r) {
    display.drawCircleHelper(cx, cy, r, arc, GxEPD_BLACK);
  }
}

void drawLoadingFrame(uint8_t phase) {
  const int16_t cx = SHANSHUI_W - kLoadingMargin - kLoadingRadius;
  const int16_t cy = SHANSHUI_H - kLoadingMargin - kLoadingRadius;
  const int16_t wh = 2 * (kLoadingRadius + kLoadingPad);

  display.setPartialWindow(cx - kLoadingRadius - kLoadingPad,
                           cy - kLoadingRadius - kLoadingPad, wh, wh);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    drawLoadingIcon(kLoadingArcs[phase & 3]);
  } while (display.nextPage());
  // M01 的 writeImageAgain 不回写 previous buffer，需手动同步，否则黑像素不擦除。
  display.nextPageToPrevious();
}

void refreshDisplay() {
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
  display.init(115200);
  display.setRotation(0);
  Serial.println("display initialized");

  generateBitmap();
  refreshDisplay();
}

void loop() {}
