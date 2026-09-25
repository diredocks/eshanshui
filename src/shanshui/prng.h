// shanshui/prng.h — 确定性种子随机数（header-only，零堆内存）。
//
// web/js/core/prng.js 用 `s = (s*s) % m`（m=999979*999983，双精度取模，
// 在 ESP32 上 double 为软浮点，又慢又占 flash），且 hash 走 btoa/JSON/字符串。
// 这里换成 mulberry32（32 位整数运算，几十字节代码），用 FNV-1a 把
// 任意种子字符串/整数折成 32 位状态。同一种子 => 同一幅画，跨平台一致。
#pragma once
#include <stdint.h>

namespace shanshui {

class Prng {
 public:
  Prng() : s_(0x12345678u) {}

  // ---- 播种 ----
  void seedUint(uint32_t x) {
    s_ = x ? x : 0x9E3779B9u;
    for (int i = 0; i < 10; i++) next(); // 与 web 同理：预热丢弃前 10 个。
  }
  // 字符串种子（含十进制数字字符串），FNV-1a。
  void seedStr(const char* str) {
    uint32_t h = 2166136261u;
    if (!str || !*str) h ^= 0x9E3779B9u;
    while (str && *str) {
      h ^= (uint8_t)*str++;
      h *= 16777619u;
    }
    seedUint(h);
  }

  // ---- 核心：mulberry32，返回 [0,1) ----
  inline float next() {
    s_ += 0x6D2B79F5u;
    uint32_t t = s_;
    t = (t ^ (t >> 15)) * (t | 1u);
    t ^= t + ((t ^ (t >> 7)) * (t | 61u));
    return (float)(t ^ (t >> 14)) / 4294967296.0f;
  }

  // ---- 辅助（对应 web Tools.randChoice/normRand/wtrand/randGaussian）----
  inline uint32_t nextU(uint32_t n) { return (uint32_t)(next() * (float)n); }
  inline float range(float lo, float hi) { return lo + (hi - lo) * next(); }
  template <int N>
  inline int choice(const int (&arr)[N]) {
    return arr[nextU(N)];
  }

 private:
  uint32_t s_;
};

} // namespace shanshui
