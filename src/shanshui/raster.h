// shanshui/raster.h — 灰度软件光栅器。
#pragma once

#include "types.h"

namespace shanshui {

// alpha==255 直接覆盖。
inline uint8_t mixGray(uint8_t gray, uint8_t dst, uint8_t alpha) {
  if (alpha == 255) return gray;
  return (uint8_t)((gray * alpha + dst * (255 - alpha) + 127) / 255);
}

class Raster {
 public:
  explicit Raster(uint8_t* gray) : buf_(gray) { reset(); }
  // buf_ 只容纳 h 行（全局行 y0..y0+h-1）。
  Raster(uint8_t* gray, int y0, int h) : buf_(gray) {
    setBand(y0, h);
    reset();
  }
  void reset(); // 当前带全纸白 255。
  // 分带窗口，带外图元被裁剪。
  void setBand(int y0, int h) {
    if (y0 < 0) y0 = 0;
    if (y0 > SHANSHUI_H) y0 = SHANSHUI_H;
    if (h < 1) h = 1;
    if (y0 + h > SHANSHUI_H) h = SHANSHUI_H - y0;
    band_y0_ = y0;
    band_h_ = h;
  }
  // 世界->像素：px = x*sx+tx, py = y*sy+ty。
  void setTransform(float sx, float tx, float sy, float ty) {
    sx_ = sx; tx_ = tx; sy_ = sy; ty_ = ty;
  }

  // fil 填充 / str+wid 描边（wid<=0 只填充）。xof/yof 为世界单位偏移。
  void poly(const Pt* pts, int n, Ink fil, Ink str, float wid = 0,
            float xof = 0, float yof = 0);
  void circle(float wx, float wy, float r, Ink fil, Ink str, float wid = 0);

  inline void blend(int x, int y, uint8_t gray, uint8_t alpha) {
    int yl = y - band_y0_;
    if ((unsigned)x >= (unsigned)SHANSHUI_W || yl < 0 || yl >= band_h_ ||
        alpha == 0)
      return;
    uint8_t& d = buf_[(size_t)yl * SHANSHUI_W + x];
    d = mixGray(gray, d, alpha);
  }

 private:
  uint8_t* buf_;
  int band_y0_ = 0;
  int band_h_ = SHANSHUI_H;
  float sx_ = 1, tx_ = 0, sy_ = 1, ty_ = 0;
  inline float px(float x) const { return x * sx_ + tx_; }
  // 返回全局行坐标；带裁剪只发生在 fill 的整数行号上。
  inline float py(float y) const { return y * sy_ + ty_; }
  void fill(const float* xs, const float* ys, int n, Ink col);
  void stroke(const float* xs, const float* ys, int n, Ink col, float wdPx);
};

} // namespace shanshui
