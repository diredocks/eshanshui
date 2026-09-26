#include "water.h"
#include <math.h>

namespace shanshui {

// 长波分段绘制（每段 ≤96 点）。
void Water::water(float xoff, float yoff, float seed) {
  const float hei = 2.0f, len = 800.0f;
  const int clu = 10;
  Pt row[96];
  float yk = 0;
  (void)seed;
  for (int i = 0; i < clu; i++) {
    float xk = (rng_.next() - 0.5f) * (len / 8);
    yk += rng_.next() * 5;
    float lk = len / 4 + rng_.next() * (len / 4);
    int c = 0;
    for (float j = -lk; j < lk && c < 96; j += 5) {
      row[c].x = j + xk + xoff;
      row[c].y = sinf(j * 0.2f) * hei * noise_.noise(j * 0.1f) - 20 + yk + yoff;
      c++;
      if (c == 96 && j + 5 < lk) {
        // 分段续画（重叠一点保连续）。
        uint8_t a = 77 + (uint8_t)(rng_.next() * 77);
        brush_.stroke(row, c, gray100(a), 1.0f, 0.5f, 1.0f, wfSin);
        row[0] = row[95];
        c = 1;
      }
    }
    if (i > 0 && c >= 2) { // 跳过第 0 簇。
      uint8_t a = 77 + (uint8_t)(rng_.next() * 77);
      brush_.stroke(row, c, gray100(a), 1.0f, 0.5f, 1.0f, wfSin);
    }
  }
}

} // namespace shanshui
