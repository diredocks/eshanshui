// shanshui/planner.h — 场景布局。
#pragma once

#include "noise.h"

namespace shanshui {

enum RegTag : uint8_t { REG_MOUNT = 0, REG_FLAT, REG_DIST, REG_BOAT };

struct Reg {
  RegTag tag;
  float x, y, h;
};

class Planner {
 public:
  Planner(Prng& rng, Noise& noise) : rng_(rng), noise_(noise) {}
  void resetPlan();
  // 规划 [xmin,xmax)：追加写入 regs（*count 为 in/out），返回追加数。
  int plan(float xmin, float xmax, Reg* regs, int maxRegs, int* count);

 private:
  Prng& rng_;
  Noise& noise_;
  static const int MTX_N = 512; // 覆盖 [-1280,1280)/5
  static const int MTX_BIAS = 256;
  int mtx_[MTX_N];
  bool locmax(float x, float y, float r);
  float ns(float x, float y);
  float yr(float x);
};

} // namespace shanshui
