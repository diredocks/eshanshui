// display.hpp
#pragma once

#include <stdint.h>

namespace display {

void init();

// 若面板已休眠则重新初始化唤醒（showBitmap 后会休眠）。
void wake();

// 画一帧加载圆弧（phase 低 2 位选缺口方向：右上/右下/左下/左上）。
void drawLoadingFrame(uint8_t phase);

// 全屏刷新二值位图（bit=1=白，MSB 先行），完成后休眠面板。
void showBitmap(const uint8_t* bits);

} // namespace display
