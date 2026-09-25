// shanshui/types.h — 最小公共类型：点 + 灰度墨色，无 STL 依赖。
#pragma once
#include "config.h"
#include <stddef.h>
#include <stdint.h>

namespace shanshui {

// 世界/像素坐标点（float，ESP32 单精度 FPU 友好）。
struct Pt {
  float x, y;
};

// 灰度墨色：gray 0=黑..255=白，alpha 0=透明..255=不透明。
// web 端用 "rgba(...)" 字符串，这里直接存数值，省解析、省 flash。
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
// 常用色：纸白（不透明）、透明（跳过绘制）。
inline Ink paper() { return ink(255, 255); }
inline Ink none() { return ink(0, 0); }

// web 的 `rgba(100,100,100,a)` 映射：gray=100，alpha=a*255。
// a 以 0..255 直接传入，避免浮点字符串。
inline Ink gray100(uint8_t a) { return ink(100, a); }

// 中点（poly.js midPt）。
inline Pt midPt(const Pt& a, const Pt& b) {
  Pt r;
  r.x = (a.x + b.x) * 0.5f;
  r.y = (a.y + b.y) * 0.5f;
  return r;
}

inline float distPt(const Pt& a, const Pt& b) {
  float dx = a.x - b.x, dy = a.y - b.y;
  return dx * dx + dy * dy; // 调用方按需 sqrt（多数比较可直接比平方）。
}

inline float mapVal(float v, float istart, float istop, float ostart, float ostop) {
  return ostart + (ostop - ostart) * ((v - istart) / (istop - istart));
}

} // namespace shanshui
