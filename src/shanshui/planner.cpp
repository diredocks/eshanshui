#include "planner.h"
#include "types.h"

namespace shanshui {

void Planner::resetPlan() {
  for (int i = 0; i < MTX_N; i++) mtx_[i] = 0;
}

float Planner::ns(float x, float y) {
  (void)y;
  float v = noise_.noise(x * 0.03f) - 0.55f;
  if (v < 0) v = 0;
  return v * 2;
}

float Planner::yr(float x) { return noise_.noise(x * 0.01f, kPi); }

bool Planner::locmax(float x, float y, float r) {
  float z0 = ns(x, y);
  if (z0 <= 0.3f) return false;
  for (float i = x - r; i < x + r; i++)
    for (float j = y - r; j < y + r; j++)
      if (ns(i, j) > z0) return false;
  return true;
}

int Planner::plan(float xmin, float xmax, Reg* regs, int maxRegs, int* count) {
  if (!regs || !count) return 0;
  int added = 0;
  auto chadd = [&](RegTag tag, float x, float y, float h, float mind) -> bool {
    for (int k = 0; k < *count; k++) {
      float dx = regs[k].x - x;
      if (dx < 0) dx = -dx;
      if (dx < mind) return false;
    }
    if (*count >= maxRegs) return false;
    regs[*count].tag = tag;
    regs[*count].x = x;
    regs[*count].y = y;
    regs[*count].h = h;
    (*count)++;
    added++;
    return true;
  };
  const float xstep = 5, mwid = 200;
  // 主峰。
  for (float i = xmin; i < xmax; i += xstep) {
    for (float j = 0; j < yr(i) * 180; j += 30) {
      if (locmax(i, j, 2)) {
        float xof = i + 2 * (rng_.next() - 0.5f) * 188;
        float yof = j + 112;
        float h = ns(i, j);
        int before = *count;
        if (chadd(REG_MOUNT, xof, yof, h, 10) && *count > before) {
          for (float k = (xof - mwid) / xstep; k < (xof + mwid) / xstep; k++) {
            int idx = (int)k + MTX_BIAS;
            if (idx >= 0 && idx < MTX_N) mtx_[idx]++;
          }
        }
      }
    }
    int ai = (int)i;
    int m = ai % 1000;
    if (m < 0) m = -m;
    if (m < 4) chadd(REG_DIST, i, 105 - rng_.next() * 19, ns(i, 0), 10);
  }
  // 坡岸。
  for (float i = xmin; i < xmax; i += xstep) {
    int idx = (int)(i / xstep) + MTX_BIAS;
    int v = (idx >= 0 && idx < MTX_N) ? mtx_[idx] : 1;
    if (v == 0 && rng_.next() < 0.01f) {
      int m = (int)(4 * rng_.next());
      for (int j = 0; j < m; j++)
        chadd(REG_FLAT, i + 2 * (rng_.next() - 0.5f) * 262, 262 - j * 19,
              ns(i, j), 10);
    }
  }
  // 船。
  for (float i = xmin; i < xmax; i += xstep)
    if (rng_.next() < 0.2f)
      chadd(REG_BOAT, i, 112 + rng_.next() * 146, 0, 150);
  return added;
}

} // namespace shanshui
