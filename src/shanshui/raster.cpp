#include "raster.h"
#include <math.h>

namespace shanshui {

// 单线程渲染。
static float s_px[SHANSHUI_POLY_MAX];
static float s_py[SHANSHUI_POLY_MAX];
static float s_xi[SHANSHUI_POLY_MAX];

void Raster::reset() {
  for (size_t i = 0; i < (size_t)band_h_ * SHANSHUI_W; i++) buf_[i] = 255;
}

void Raster::poly(const Pt* pts, int n, Ink fil, Ink str, float wid, float xof,
                  float yof) {
  if (!pts || n < 2) return;
  if (n > SHANSHUI_POLY_MAX) n = SHANSHUI_POLY_MAX;
  float* xs = s_px;
  float* ys = s_py;
  for (int i = 0; i < n; i++) {
    float x = pts[i].x + xof, y = pts[i].y + yof;
    if (x != x || y != y) { // NaN 保护。
      xs[i] = -1000.0f * sx_ + tx_;
      ys[i] = -1000.0f * sy_ + ty_;
    } else {
      xs[i] = px(x);
      ys[i] = py(y);
    }
  }
  if (fil.alpha > 0) fill(xs, ys, n, fil);
  if (str.alpha > 0 && wid > 0) stroke(xs, ys, n, str, wid * sx_);
}

void Raster::fill(const float* xs, const float* ys, int n, Ink col) {
  float ymin = 1e30f, ymax = -1e30f;
  for (int i = 0; i < n; i++) {
    if (ys[i] < ymin) ymin = ys[i];
    if (ys[i] > ymax) ymax = ys[i];
  }
  int y0 = (int)floorf(ymin + 0.5f), y1 = (int)floorf(ymax + 0.5f);
  // 全局行号裁剪到带窗口（setBand 已保证窗口在屏内）。
  int yb0 = band_y0_, yb1 = band_y0_ + band_h_ - 1;
  if (y0 < yb0) y0 = yb0;
  if (y1 > yb1) y1 = yb1;
  if (y1 < y0) return;
  float* xints = s_xi;
  const uint8_t a = col.alpha;
  const uint8_t g = col.gray;
  for (int y = y0; y <= y1; y++) {
    float sy = (float)y + 0.5f;
    int m = 0;
    for (int i = 0, j = n - 1; i < n; j = i++) {
      float yi = ys[i], yj = ys[j];
      if ((yi <= sy && yj > sy) || (yj <= sy && yi > sy)) {
        if (m < SHANSHUI_POLY_MAX)
          xints[m++] = xs[i] + ((sy - yi) / (yj - yi)) * (xs[j] - xs[i]);
      }
    }
    if (m < 2) continue;
    // 插入排序（n 小，比 std::sort 省 flash）。
    for (int i = 1; i < m; i++) {
      float k = xints[i]; int j = i - 1;
      while (j >= 0 && xints[j] > k) { xints[j + 1] = xints[j]; j--; }
      xints[j + 1] = k;
    }
    for (int k = 0; k + 1 < m; k += 2) {
      int xa = (int)ceilf(xints[k] - 0.5f), xb = (int)floorf(xints[k + 1] - 0.5f);
      if (xa < 0) xa = 0;
      if (xb > SHANSHUI_W - 1) xb = SHANSHUI_W - 1;
      uint8_t* row = buf_ + (size_t)(y - band_y0_) * SHANSHUI_W;
      for (int x = xa; x <= xb; x++) row[x] = mixGray(g, row[x], a);
    }
  }
}

void Raster::stroke(const float* xs, const float* ys, int n, Ink col,
                    float wdPx) {
  if (n < 2) return;
  float hw = wdPx > 0.75f ? wdPx * 0.5f : 0.375f;
  float qx[4], qy[4];
  for (int i = 0; i < n - 1; i++) {
    float dx = xs[i + 1] - xs[i], dy = ys[i + 1] - ys[i];
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-6f) continue;
    float nx = (-dy / len) * hw, ny = (dx / len) * hw;
    qx[0] = xs[i] + nx;     qy[0] = ys[i] + ny;
    qx[1] = xs[i + 1] + nx; qy[1] = ys[i + 1] + ny;
    qx[2] = xs[i + 1] - nx; qy[2] = ys[i + 1] - ny;
    qx[3] = xs[i] - nx;     qy[3] = ys[i] - ny;
    fill(qx, qy, 4, col);
  }
}

void Raster::circle(float wx, float wy, float r, Ink fil, Ink str, float wid) {
  float cx = px(wx), cy = py(wy), rr = r * sx_;
  if (fil.alpha > 0) {
    const int N = 24;
    float xs[24], ys[24];
    for (int i = 0; i < N; i++) {
      float a = (float)i / N * kTau;
      xs[i] = cx + cosf(a) * rr;
      ys[i] = cy + sinf(a) * rr;
    }
    fill(xs, ys, N, fil);
  }
  if (str.alpha > 0 && wid > 0) {
    const int N = 25;
    float xs[25], ys[25];
    for (int i = 0; i < N; i++) {
      float a = (float)i / (N - 1) * kTau;
      xs[i] = cx + cosf(a) * rr;
      ys[i] = cy + sinf(a) * rr;
    }
    stroke(xs, ys, N, str, wid * sx_);
  }
}

} // namespace shanshui
