// shanshui/types.h — 最小公共类型：点 + 灰度墨色，无 STL 依赖。
#pragma once

#include "config.h"

#include <stddef.h>
#include <stdint.h>

namespace shanshui {

constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

// 世界/像素坐标点（float，ESP32 单精度 FPU 友好）。
struct Pt {
  float x, y;
};

// 灰度墨色：gray 0=黑..255=白，alpha 0=透明..255=不透明。
struct Ink {
  uint8_t gray;
  uint8_t alpha;
};

inline Ink ink(uint8_t gray, uint8_t alpha) {
  Ink c;
  c.gray = gray;
  c.alpha = alpha;
  return c;
}
inline Ink paper() { return ink(255, 255); }
inline Ink none() { return ink(0, 0); }
// web 的 `rgba(100,100,100,a)` 映射。
inline Ink gray100(uint8_t a) { return ink(100, a); }

// 中点。
inline Pt midPt(const Pt& a, const Pt& b) {
  Pt r;
  r.x = (a.x + b.x) * 0.5f;
  r.y = (a.y + b.y) * 0.5f;
  return r;
}

// 距离平方（调用方按需 sqrt）。
inline float distPt(const Pt& a, const Pt& b) {
  float dx = a.x - b.x, dy = a.y - b.y;
  return dx * dx + dy * dy;
}

inline float mapVal(float v, float istart, float istop, float ostart,
                    float ostop) {
  return ostart + (ostop - ostart) * ((v - istart) / (istop - istart));
}

// 拷贝并加世界偏移。
inline void copyOffset(Pt* dst, const Pt* src, int n, float dx, float dy) {
  for (int i = 0; i < n; i++) {
    dst[i].x = src[i].x + dx;
    dst[i].y = src[i].y + dy;
  }
}

// 就地加世界偏移。
inline void translate(Pt* pts, int n, float dx, float dy) {
  for (int i = 0; i < n; i++) {
    pts[i].x += dx;
    pts[i].y += dy;
  }
}

// 就地反转。
inline void reversePts(Pt* pts, int n) {
  for (int i = 0; i < n / 2; i++) {
    Pt t = pts[i];
    pts[i] = pts[n - 1 - i];
    pts[n - 1 - i] = t;
  }
}

} // namespace shanshui
