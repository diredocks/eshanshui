// shanshui/tree.h — 树（web/tree.js 的 MCU 版）。
#pragma once

#include "brush.h"

namespace shanshui {

class Tree {
 public:
  Tree(Prng& rng, Noise& noise, Brush& brush, Raster& raster)
      : rng_(rng), noise_(noise), brush_(brush), ras_(raster) {}

  void tree01(float x, float y, float hei, float wid, uint8_t gray, uint8_t alpha);
  void tree02(float x, float y, float hei, float wid, int clu, uint8_t gray, uint8_t alpha);
  void tree03(float x, float y, float hei, float wid, float benC, float benP,
              uint8_t gray, uint8_t alpha);
  void tree04(float x, float y, float hei, float wid, uint8_t gray, uint8_t alpha);
  void tree05(float x, float y, float hei, float wid, uint8_t gray, uint8_t alpha);
  void tree06(float x, float y, float hei, float wid, uint8_t gray, uint8_t alpha);
  void tree07(float x, float y, float hei, float wid, uint8_t gray, uint8_t alpha);
  void tree08(float x, float y, float hei, float wid, uint8_t gray, uint8_t alpha);

 private:
  Prng& rng_;
  Noise& noise_;
  Brush& brush_;
  Raster& ras_;
  float shape(float x);
  // 树干：返回每侧点数，o0/o1 各 n 点。
  int branch(float hei, float wid, float ang, float ben, float det, Pt* o0,
             Pt* o1, int maxOut);
  int trunkOutline(const Pt* s0, const Pt* s1, int n, float x, float y,
                   Pt* tr);
  int branchOutline(const Pt* b0, const Pt* b1, int bn, float ox, float oy,
                    Pt* bp);
  void edgeStroke(const Pt* tr, int begin, int end, uint8_t base);
  void twig(float tx, float ty, int dep, int dir, float sca, float wid,
            float ang, bool lea, float leaSz);
  void barkify(float x, float y, const Pt* s0, const Pt* s1, int n);
  void frac06(float xoff, float yoff, int dep, float hei, float wid, float ang,
              float ben);
  void frac08(float xoff, float yoff, int dep, float ang, float len, float ben);
};

} // namespace shanshui
