// shanshui/prng.h — 确定性种子随机数（header-only，零堆内存）。
// web 用双精度取模，ESP32 上是软浮点；这里改 mulberry32（32 位整数）。
#pragma once

#include <stdint.h>

namespace shanshui {

class Prng {
 public:
  Prng() : s_(0x12345678u) {}

  void seedUint(uint32_t x) {
    s_ = x ? x : 0x9E3779B9u;
    for (int i = 0; i < 10; i++) next(); // 与 web 一致：预热丢弃前 10 个。
  }
  // FNV-1a。
  void seedStr(const char* str) {
    uint32_t h = 2166136261u;
    if (!str || !*str) h ^= 0x9E3779B9u;
    while (str && *str) {
      h ^= (uint8_t)*str++;
      h *= 16777619u;
    }
    seedUint(h);
  }

  // mulberry32，返回 [0,1)。
  inline float next() {
    s_ += 0x6D2B79F5u;
    uint32_t t = s_;
    t = (t ^ (t >> 15)) * (t | 1u);
    t ^= t + ((t ^ (t >> 7)) * (t | 61u));
    return (float)(t ^ (t >> 14)) / 4294967296.0f;
  }

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
