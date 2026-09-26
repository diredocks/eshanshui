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
constexpr int16_t kLoadingRadius = 14;
constexpr int16_t kLoadingThickness = 3;
constexpr int16_t kLoadingMargin = 10;
constexpr int16_t kLoadingPad = 2;
// 3/4 圆环，缺口依次位于右上/右下/左下/左上（顺时针转）。
constexpr uint8_t kLoadingArcs[4] = {0x1 | 0x4 | 0x8, 0x1 | 0x2 | 0x8,
                                     0x1 | 0x2 | 0x4, 0x2 | 0x4 | 0x8};

GxEPD2_BW<GxEPD2_420_M01, GxEPD2_420_M01::HEIGHT> s_display(
    GxEPD2_420_M01(kEpdCs, kEpdDc, kEpdRst, kEpdBusy));
bool s_sleeping = false;

void drawLoadingIcon(uint8_t arc) {
  const int16_t cx = SHANSHUI_W - kLoadingMargin - kLoadingRadius;
  const int16_t cy = SHANSHUI_H - kLoadingMargin - kLoadingRadius;
  for (int16_t r = kLoadingRadius; r > kLoadingRadius - kLoadingThickness;
       --r) {
    s_display.drawCircleHelper(cx, cy, r, arc, GxEPD_BLACK);
  }
}

} // namespace

void init() {
  // initial=false：禁止库的强制首刷全刷。
  // 否则首个局部刷新会被强制转为全屏刷新，清掉旧画面。
  // 首屏最终会由 showBitmap() 做一次全刷来保证显示质量。
  s_display.init(115200, false);
  s_display.setRotation(0);
  s_sleeping = false;
  Serial.println("display initialized");
}

void wake() {
  if (!s_sleeping) return;
  // 同上：wake 后只做右下角局部刷新，必须用 initial=false，
  // 否则 drawLoadingFrame() 会被库强制为全屏刷新。
  s_display.init(115200, false);
  s_display.setRotation(0);
  s_sleeping = false;
}

void drawLoadingFrame(uint8_t phase) {
  // 局部窗口：只刷右下角小方块，旧画面保留。
  // 全屏刷新只在 showBitmap() 里做一次。
  const int16_t cx = SHANSHUI_W - kLoadingMargin - kLoadingRadius;
  const int16_t cy = SHANSHUI_H - kLoadingMargin - kLoadingRadius;
  const int16_t wh = 2 * (kLoadingRadius + kLoadingPad);

  s_display.setPartialWindow(cx - kLoadingRadius - kLoadingPad,
                             cy - kLoadingRadius - kLoadingPad, wh, wh);
  s_display.firstPage();
  do {
    s_display.fillScreen(GxEPD_WHITE);
    drawLoadingIcon(kLoadingArcs[phase & 3]);
  } while (s_display.nextPage());
  // M01 的 writeImageAgain 不回写 previous buffer，需手动同步，否则黑像素不擦除。
  s_display.nextPageToPrevious();
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
  s_sleeping = true;
}

} // namespace display
