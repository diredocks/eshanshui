#include "mount.h"
#include <math.h>

namespace shanshui {

static const float PI PROGMEM = 3.14159265f;

// 网格放 BSS（mountain 10×50，rock 10×50，flat 5×50，无重入调用，安全）。
static Pt s_mgrid[SHANSHUI_MOUNT_I * SHANSHUI_MOUNT_J];
static Pt s_rgrid[SHANSHUI_MOUNT_I * SHANSHUI_MOUNT_J];
static Pt s_fgrid[SHANSHUI_FLAT_I * SHANSHUI_FLAT_J];
// 山腰候选点（web 为动态数组；BSS 定长 256，避免截断改画面）。
static Pt s_cand[256];

static inline uint8_t aOf(float a01) {
  if (a01 < 0) a01 = 0;
  if (a01 > 1) a01 = 1;
  return (uint8_t)(a01 * 255);
}

void Mount::foot(const Pt* grid, int I, int J, float xof, float yof) {
  const int span = 10;
  Pt f0[24], f1[24];
  int i = 0, ni = 0;
  for (i = 0; i < I - 2; i++) {
    if (i != ni) continue;
    static const int kStep[2] PROGMEM = {1, 2};
    ni = i + rng_.choice<2>(kStep);
    if (ni > I - 1) ni = I - 1;
    int c0 = 0, c1 = 0;
    int lim = J / 8;
    if (lim > 10) lim = 10;
    for (int j = 0; j < lim && c0 < 12; j++) {
      f0[c0].x = grid[i * J + j].x + noise_.noise(j * 0.1f, i) * 10;
      f0[c0].y = grid[i * J + j].y;
      c0++;
      f1[c1].x = grid[i * J + (J - 1 - j)].x - noise_.noise(j * 0.1f, i) * 10;
      f1[c1].y = grid[i * J + (J - 1 - j)].y;
      c1++;
    }
    // 反转前半（web reverse 语义）。
    for (int a = 0; a < c0 / 2; a++) {
      Pt t = f0[a]; f0[a] = f0[c0 - 1 - a]; f0[c0 - 1 - a] = t;
    }
    for (int a = 0; a < c1 / 2; a++) {
      Pt t = f1[a]; f1[a] = f1[c1 - 1 - a]; f1[c1 - 1 - a] = t;
    }
    for (int j = 0; j < span && c0 < 23 && c1 < 23; j++) {
      float p = (float)j / span;
      float x1 = grid[i * J].x * (1 - p) + grid[ni * J].x * p;
      float y1 = grid[i * J].y * (1 - p) + grid[ni * J].y * p;
      float x2 = grid[i * J + J - 1].x * (1 - p) + grid[ni * J + J - 1].x * p;
      float y2 = grid[i * J + J - 1].y * (1 - p) + grid[ni * J + J - 1].y * p;
      float vib = -1.7f * (p - 1) * powf(p > 0 ? p : 0.0001f, 1.0f / 5);
      y1 += vib * 5 + noise_.noise(xof * 0.05f, i) * 5;
      y2 += vib * 5 + noise_.noise(xof * 0.05f, i) * 5;
      f0[c0].x = x1; f0[c0].y = y1; c0++;
      f1[c1].x = x2; f1[c1].y = y2; c1++;
    }
    ras_.poly(f0, c0, paper(), none(), 0, xof, yof);
    ras_.poly(f1, c1, paper(), none(), 0, xof, yof);
    Pt w0[24], w1[24];
    for (int k = 0; k < c0; k++) { w0[k].x = f0[k].x + xof; w0[k].y = f0[k].y + yof; }
    for (int k = 0; k < c1; k++) { w1[k].x = f1[k].x + xof; w1[k].y = f1[k].y + yof; }
    uint8_t a = 26 + (uint8_t)(rng_.next() * 26);
    brush_.stroke(w0, c0, ink(100, a), 1.0f, 0.5f, 1.0f, wfSin);
    brush_.stroke(w1, c1, ink(100, a), 1.0f, 0.5f, 1.0f, wfSin);
  }
}

void Mount::mountain(float xoff, float yoff, float seed, bool veg) {
  float hei = 100 + rng_.next() * 400;
  float wid = 400 + rng_.next() * 200;
  int tex = 200; // web 默认；TEX_DIV 缩放在 Brush::texture 内统一做。
  Pt* grid = s_mgrid;
  const int I = SHANSHUI_MOUNT_I, J = SHANSHUI_MOUNT_J;
  float hoff = 0;
  for (int j = 0; j < I; j++) {
    hoff += (rng_.next() * yoff) / 100;
    for (int i = 0; i < J; i++) {
      float x = ((float)i / J - 0.5f) * PI;
      float y = cosf(x) * noise_.noise(x + 10, j * 0.15f, seed);
      float p = 1 - (float)j / I;
      grid[j * J + i].x = (x / PI) * wid * p;
      grid[j * J + i].y = -y * hei * p + hoff;
    }
  }
  // 山脊点树（RIM）。
  for (int j = 0; j < J; j++) {
    float ns = noise_.noise(j * 0.1f, seed);
    if (ns * ns * ns < 0.1f && fabsf(grid[j].y) / hei > 0.2f) {
      float a = noise_.noise(0.01f * (grid[j].x + xoff), 0.01f * (grid[j].y + yoff)) * 0.5f * 0.3f + 0.5f;
      tree_.tree02(grid[j].x + xoff, grid[j].y + yoff - 5, 16, 8, 2, 100, aOf(a));
    }
  }
  // 白底 + 轮廓。
  Pt bg[SHANSHUI_MOUNT_J + 1];
  for (int i = 0; i < J; i++) bg[i] = grid[i];
  bg[J].x = 0; bg[J].y = I * 4;
  ras_.poly(bg, J + 1, paper(), none(), 0, xoff, yoff);
  Pt edge[SHANSHUI_MOUNT_J];
  for (int i = 0; i < J; i++) {
    edge[i].x = grid[i].x + xoff; edge[i].y = grid[i].y + yoff;
  }
  brush_.stroke(edge, J, ink(100, 77), 3.0f, 1.0f, 1.0f, wfSin);
  foot(grid, I, J, xoff, yoff);
  TexArgs t;
  t.tex = tex;
  static const int kShade[5] PROGMEM = {0, 0, 0, 0, 5};
  t.sha = (float)rng_.choice<5>(kShade);
  brush_.texture(grid, I, J, xoff, yoff, t);
  // 山顶簇树（TOP）。
  for (int i = 0; i < I; i++) {
    for (int j = 0; j < J; j++) {
      float ns = noise_.noise(i * 0.1f, j * 0.1f, seed + 2);
      if (ns * ns * ns < 0.1f && fabsf(grid[i * J + j].y) / hei > 0.5f) {
        float a = noise_.noise(0.01f * (grid[i * J + j].x + xoff), 0.01f * (grid[i * J + j].y + yoff)) * 0.5f * 0.3f + 0.5f;
        tree_.tree02(grid[i * J + j].x + xoff, grid[i * J + j].y + yoff, 16, 8, 5, 100, aOf(a));
      }
    }
  }
  if (veg) {
    // 山腰松（MIDDLE，聚集 proof）。独立作用域：cand 用完即释放栈。
    {
      Pt* cand = s_cand;
      int cc = 0;
      for (int i = 0; i < I && cc < 256; i++)
        for (int j = 0; j < J && cc < 256; j++) {
        float ns = noise_.noise(i * 0.2f, j * 0.05f, seed);
        if ((j % 2) && ns * ns * ns * ns < 0.012f &&
            fabsf(grid[i * J + j].y) / hei < 0.3f) {
          cand[cc++] = grid[i * J + j];
        }
      }
    for (int i = 0; i < cc; i++) {
      int nb = 0;
      for (int k = 0; k < cc && nb <= 2; k++) {
        if (i == k) continue;
        float dx = cand[i].x - cand[k].x, dy = cand[i].y - cand[k].y;
        if (dx * dx + dy * dy < 900) nb++;
      }
      if (nb > 2) {
        float ht = ((hei + cand[i].y) / hei) * 70;
        ht = ht * 0.3f + rng_.next() * ht * 0.7f;
        float a = noise_.noise(0.01f * (cand[i].x + xoff), 0.01f * (cand[i].y + yoff)) * 0.5f * 0.3f + 0.3f;
        tree_.tree01(cand[i].x + xoff, cand[i].y + yoff, ht,
                     rng_.next() * 3 + 1, 100, aOf(a));
      }
    }
    } // 释放 cand 栈。
    // 山脚杂树（BOTTOM）。
    for (int i = 0; i < I; i++)
      for (int j = 0; j < J; j++) {
        float ns = noise_.noise(i * 0.2f, j * 0.05f, seed);
        if ((j == 0 || j == J - 1) && ns * ns * ns * ns < 0.012f) {
          float ht = ((hei + grid[i * J + j].y) / hei) * 120;
          ht = ht * 0.5f + rng_.next() * ht * 0.5f;
          float bc = rng_.next() * 0.1f;
          float a = noise_.noise(0.01f * (grid[i * J + j].x + xoff), 0.01f * (grid[i * J + j].y + yoff)) * 0.5f * 0.3f + 0.3f;
          tree_.tree03(grid[i * J + j].x + xoff, grid[i * J + j].y + yoff, ht,
                       5, bc, 1.0f, 100, aOf(a));
        }
      }
  }
  // 山脚建筑（BOTT ARCH）。
  for (int i = 0; i < I; i++)
    for (int j = 0; j < J; j++) {
      float ns = noise_.noise(i * 0.2f, j * 0.05f, seed + 10);
      if (i != 0 && (j == 1 || j == J - 2) && ns * ns * ns * ns < 0.008f) {
        static const int kBottArch[6] PROGMEM = {0, 0, 1, 1, 1, 2};
        static const int kArch02Sto[4] PROGMEM = {1, 2, 2, 3};
        static const int kArch02Sty[3] PROGMEM = {1, 2, 3};
        static const int kArch04Sto[5] PROGMEM = {1, 1, 1, 2, 2};
        int tt = rng_.choice<6>(kBottArch);
        if (tt == 1)
          arch_.arch02(grid[i * J + j].x + xoff, grid[i * J + j].y + yoff,
                       seed, 10, rng_.range(40, 70), rng_.next(), 5,
                       rng_.choice<4>(kArch02Sto),
                       rng_.choice<3>(kArch02Sty), false);
        else if (tt == 2)
          arch_.arch04(grid[i * J + j].x + xoff, grid[i * J + j].y + yoff,
                       seed, 15, 30, 0.7f, 5,
                       rng_.choice<5>(kArch04Sto));
      }
    }
  // 山顶塔（TOP ARCH）：第 1 排中央，2% 概率。
  for (int j = 0; j < J; j++) {
    int dj = j - J / 2;
    if (dj < 0) dj = -dj;
    if (dj < 1 && rng_.next() < 0.02f) {
      static const int kTopArchSto[2] PROGMEM = {5, 7};
      arch_.arch03(grid[1 * J + j].x + xoff, grid[1 * J + j].y + yoff, seed,
                   10, 50, 0.7f, 5, rng_.choice<2>(kTopArchSto));
    }
  }
  // 电塔（TRANSM）。
  for (int i = 0; i < I; i += 2)
    for (int j = 0; j < J; j++) {
      float ns = noise_.noise(i * 0.2f, j * 0.05f, seed + 20 * PI);
      if ((j == 1 || j == J - 2) && ns * ns * ns * ns < 0.002f)
        arch_.tower01(grid[i * J + j].x + xoff, grid[i * J + j].y + yoff, 100, 20);
    }
  // 山脚石（BOTT ROCK）。
  for (int i = 0; i < I; i++)
    for (int j = 0; j < J; j++)
      if ((j == 0 || j == J - 1) && rng_.next() < 0.1f)
        rock(grid[i * J + j].x + xoff, grid[i * J + j].y + yoff,
             rng_.next() * 100, 20 + rng_.next() * 20, 20 + rng_.next() * 20,
             40, 2);
}

void Mount::flatMount(float xoff, float yoff, float seed, float wid, float hei,
                      float cho) {
  Pt* grid = s_fgrid;
  const int I = SHANSHUI_FLAT_I, J = SHANSHUI_FLAT_J;
  float hoff = 0;
  float flat[SHANSHUI_FLAT_I][2][2]; // 每行至多一段平台 [x0,y0,x1,y1] 近似
  int fn = 0;
  (void)fn;
  Pt fseg[SHANSHUI_FLAT_I * 2][2];
  int fsg = 0;
  for (int j = 0; j < I; j++) {
    hoff += (rng_.next() * yoff) / 100;
    bool inFlat = false;
    float fx0 = 0, fy0 = 0;
    for (int i = 0; i < J; i++) {
      float x = ((float)i / J - 0.5f) * PI;
      float y = (cosf(x * 2) + 1) * noise_.noise(x + 10, j * 0.1f, seed);
      float p = 1 - ((float)j / I) * 0.6f;
      float nx = (x / PI) * wid * p;
      float ny = -y * hei * p + hoff;
      float h = hei / 2.6f;
      bool nowFlat = ny < -h * cho + hoff;
      if (nowFlat) {
        ny = -h * cho + hoff;
        if (!inFlat && fsg < SHANSHUI_FLAT_I * 2) {
          fx0 = nx; fy0 = ny; inFlat = true;
        }
      } else if (inFlat) {
        if (fsg < SHANSHUI_FLAT_I * 2) {
          fseg[fsg][0].x = fx0; fseg[fsg][0].y = fy0;
          fseg[fsg][1] = grid[j * J + i - 1];
          fsg++;
        }
        inFlat = false;
      }
      grid[j * J + i].x = nx;
      grid[j * J + i].y = ny;
    }
    if (inFlat && fsg < SHANSHUI_FLAT_I * 2) {
      fseg[fsg][0].x = fx0; fseg[fsg][0].y = fy0;
      fseg[fsg][1] = grid[j * J + J - 1];
      fsg++;
    }
  }
  (void)flat;
  Pt bg[SHANSHUI_FLAT_J + 1];
  for (int i = 0; i < J; i++) bg[i] = grid[i];
  bg[J].x = 0; bg[J].y = I * 4;
  ras_.poly(bg, J + 1, paper(), none(), 0, xoff, yoff);
  Pt edge[SHANSHUI_FLAT_J];
  for (int i = 0; i < J; i++) {
    edge[i].x = grid[i].x + xoff; edge[i].y = grid[i].y + yoff;
  }
  brush_.stroke(edge, J, ink(100, 77), 3.0f, 1.0f, 1.0f, wfSin);
  TexArgs t;
  t.tex = 80;
  t.wid = 2.0f;
  t.dis = DIS_FLAT;
  brush_.texture(grid, I, J, xoff, yoff, t);
  // 平台岸线（取偶数行两端点，web grlist1/grlist2 语义简化）。
  // 平台岸线（独立作用域，大缓冲用完即释，再进 flatDec 深调用）。
  float xmin, xmax, ymin, ymax;
  {
    Pt g1[16], g2[16];
    int n1 = 0, n2 = 0;
  for (int k = 0; k < fsg && n1 < 14 && n2 < 14; k += 2) {
    g1[n1++] = fseg[k][0];
    g2[n2++] = fseg[k][1];
  }
  if (n1 == 0) return;
  float wb0 = g1[0].x, wb1 = g2[0].x;
  for (int i = 0; i < 3 && n1 < 15 && n2 < 15; i++) {
    float p = 0.8f - i * 0.2f;
    for (int k = n1; k > 0; k--) g1[k] = g1[k - 1];
    g1[0].x = wb0 * p; g1[0].y = g1[1].y - 5; n1++;
    for (int k = n2; k > 0; k--) g2[k] = g2[k - 1];
    g2[0].x = wb1 * p; g2[0].y = g2[1].y - 5; n2++;
  }
  wb0 = g1[n1 - 1].x; wb1 = g2[n2 - 1].x;
  for (int i = 0; i < 3 && n1 < 16 && n2 < 16; i++) {
    float p = 0.6f - i * i * 0.1f;
    g1[n1].x = wb0 * p; g1[n1].y = g1[n1 - 1].y + 1; n1++;
    g2[n2].x = wb1 * p; g2[n2].y = g2[n2 - 1].y + 1; n2++;
  }
  Pt d1[64], d2[64];
  int m1 = brush_.div(g1, n1, 5, d1, 64);
  int m2 = brush_.div(g2, n2, 5, d2, 64);
  Pt gr[96];
  int gc = 0;
  for (int i = m1 - 1; i >= 0 && gc < 90; i--) gr[gc++] = d1[i];
  for (int i = 0; i < m2 && gc < 90; i++) gr[gc++] = d2[i];
  if (gc > 0 && gc < 96) gr[gc++] = d1[m1 - 1];
  for (int i = 0; i < gc; i++) {
    float v = (1 - fabsf(((i % 5) - 2.5f)) / 2.5f) * 0.12f;
    gr[i].x *= 1 - v + noise_.noise(gr[i].y * 0.5f) * v;
  }
  ras_.poly(gr, gc, paper(), none(), 0, xoff, yoff);
  Pt ge[96];
  for (int i = 0; i < gc && i < 96; i++) {
    ge[i].x = gr[i].x + xoff; ge[i].y = gr[i].y + yoff;
  }
  brush_.stroke(ge, gc, ink(100, 51), 3.0f, 0.5f, 1.0f, wfSin);
  xmin = 1e30f; xmax = -1e30f; ymin = 1e30f; ymax = -1e30f;
  for (int i = 0; i < gc; i++) {
    if (gr[i].x < xmin) xmin = gr[i].x;
    if (gr[i].x > xmax) xmax = gr[i].x;
    if (gr[i].y < ymin) ymin = gr[i].y;
    if (gr[i].y > ymax) ymax = gr[i].y;
  }
  }
  flatDec(xoff, yoff, xmin, xmax, ymin, ymax);
}

void Mount::flatDec(float xoff, float yoff, float xmin, float xmax, float ymin,
                    float ymax) {
  static const int kFlatKind[6] PROGMEM = {0, 0, 1, 2, 3, 4};
  static const int kFlatTree[4] PROGMEM = {0, 0, 1, 2};
  static const int kPineCount[7] PROGMEM = {1, 1, 1, 1, 2, 2, 3};
  int tt = rng_.choice<6>(kFlatKind);
  int n = (int)(rng_.next() * 5);
  for (int j = 0; j < n; j++)
    rock(xoff + rng_.range(xmin, xmax), yoff + (ymin + ymax) / 2 + rng_.range(-10, 10) + 10,
         rng_.next() * 100, 10 + rng_.next() * 20, 10 + rng_.next() * 20, 40, 2);
  int ng = rng_.choice<4>(kFlatTree);
  for (int j = 0; j < ng; j++) {
    float xr = xoff + rng_.range(xmin, xmax);
    float yr = yoff + (ymin + ymax) / 2 + rng_.range(-5, 5) + 20;
    int k2 = 2 + (int)(rng_.next() * 3);
    for (int k = 0; k < k2; k++) {
      float xx = xr + rng_.range(-30, 30);
      if (xx < xoff + xmin) xx = xoff + xmin;
      if (xx > xoff + xmax) xx = xoff + xmax;
      tree_.tree08(xx, yr, 60 + rng_.next() * 40, 1, 100, 128);
    }
  }
  if (tt == 0) {
    int m = (int)(rng_.next() * 3);
    for (int j = 0; j < m; j++)
      rock(xoff + rng_.range(xmin, xmax), yoff + (ymin + ymax) / 2 + rng_.range(-5, 5) + 20,
           rng_.next() * 100, 50 + rng_.next() * 20, 40 + rng_.next() * 20, 40, 5);
  } else if (tt == 1) {
    float pmin = rng_.next() * 0.5f, pmax = rng_.next() * 0.5f + 0.5f;
    float xa = xmin * (1 - pmin) + xmax * pmin;
    float xb = xmin * (1 - pmax) + xmax * pmax;
    for (float i = xa; i < xb; i += 30)
      tree_.tree05(xoff + i + 20 * rng_.range(-1, 1),
                   yoff + (ymin + ymax) / 2 + 20, 100 + rng_.next() * 200, 5,
                   100, 128);
    int m = (int)(rng_.next() * 4);
    for (int j = 0; j < m; j++)
      rock(xoff + rng_.range(xmin, xmax), yoff + (ymin + ymax) / 2 + rng_.range(-5, 5) + 20,
           rng_.next() * 100, 50 + rng_.next() * 20, 40 + rng_.next() * 20, 40, 5);
  } else if (tt == 2) {
    int m = kPineCount[rng_.nextU(7)];
    for (int i = 0; i < m; i++) {
      float xr = rng_.range(xmin, xmax);
      float yr = (ymin + ymax) / 2;
      tree_.tree04(xoff + xr, yoff + yr + 20, 300, 6, 100, 128);
      int m2 = (int)(rng_.next() * 2);
      for (int j = 0; j < m2; j++) {
        float xx = xr + rng_.range(-50, 50);
        if (xx < xmin) xx = xmin;
        if (xx > xmax) xx = xmax;
        rock(xoff + xx, yoff + yr + rng_.range(-5, 5) + 20,
             j * i * rng_.next() * 100, 50 + rng_.next() * 20,
             40 + rng_.next() * 20, 40, 5);
      }
    }
  } else if (tt == 3) {
    int m = kPineCount[rng_.nextU(7)];
    for (int i = 0; i < m; i++)
      tree_.tree06(xoff + rng_.range(xmin, xmax), yoff + (ymin + ymax) / 2,
                   60 + rng_.next() * 60, 6, 100, 128);
  } else {
    float pmin = rng_.next() * 0.5f, pmax = rng_.next() * 0.5f + 0.5f;
    float xa = xmin * (1 - pmin) + xmax * pmin;
    float xb = xmin * (1 - pmax) + xmax * pmax;
    for (float i = xa; i < xb; i += 20)
      tree_.tree07(xoff + i + 20 * rng_.range(-1, 1),
                   yoff + (ymin + ymax) / 2 + rng_.range(-1, 1),
                   rng_.range(40, 80), 4, 100, 255);
  }
  int n2 = (int)(50 * rng_.next());
  for (int i = 0; i < n2; i++)
    tree_.tree02(xoff + rng_.range(xmin, xmax), yoff + rng_.range(ymin, ymax),
                 16, 8, 5, 100, 128);
  static const int kFlatHut[5] PROGMEM = {0, 0, 0, 0, 1};
  int ts = rng_.choice<5>(kFlatHut);
  if (ts == 1 && tt != 4)
    arch_.arch01(xoff + rng_.range(xmin, xmax), yoff + (ymin + ymax) / 2 + 20,
                 rng_.next(), rng_.range(80, 100), rng_.range(160, 200), 0.7f,
                 rng_.next());
}

void Mount::distMount(float xoff, float yoff, float seed) {
  float hei = 56;
  static const int kDistLen[3] PROGMEM = {200, 400, 550};
  int len = kDistLen[rng_.nextU(3)];
  const int seg = 5, span = 10;
  int nseg = len / span / seg;
  for (int i = 0; i < nseg; i++) {
    Pt top[8], bot[4];
    for (int j = 0; j <= seg; j++) {
      float k = (float)(i * seg + j);
      float s = sinf(PI * k / (len / span));
      if (s < 0) s = 0;
      top[j].x = xoff + k * span;
      top[j].y = yoff - hei * noise_.noise(k * 0.05f, seed) * sqrtf(s);
    }
    // 底边按段宽均分（j*seg/2），与相邻段共端点，避免楔形裂缝。
    // （web 原样 j*2 在 seg=5 时末段短一步，靠同色三角描边 bleed 掩盖。）
    for (int j = 0; j <= seg / 2; j++) {
      float k = i * seg + j * (seg / 2.0f);
      float s = sinf(PI * k / (len / span));
      if (s < 0) s = 0;
      bot[j].x = xoff + k * span;
      bot[j].y = yoff + 24 * noise_.noise(k * 0.05f, 2, seed) * s;
    }
    Pt pg[12];
    int c = 0;
    for (int j = seg / 2; j >= 0; j--) pg[c++] = bot[j]; // 下边：右→左
    for (int j = 0; j <= seg; j++) pg[c++] = top[j];     // 上边：左→右
    float mx = top[seg].x, my = top[seg].y;
    uint8_t g = (uint8_t)(noise_.noise(mx * 0.02f, my * 0.02f, yoff) * 55 + 200);
    Ink fillc = ink(g, 255);
    ras_.poly(pg, c, fillc, fillc, 1.0f); // 同色描边：接缝 bleed（web 三角同色描边等价）
    // 顶部皴笔（代替 triangulate 明暗）。
    brush_.stroke(top, seg + 1, ink(g > 40 ? g - 40 : 0, 102), 1.0f, 0.5f,
                  1.0f, wfSin);
  }
}

void Mount::rock(float xoff, float yoff, float seed, float wid, float hei,
                 int tex, float sha) {
  Pt* grid = s_rgrid;
  const int I = SHANSHUI_MOUNT_I, J = SHANSHUI_MOUNT_J;
  for (int i = 0; i < I; i++) {
    float ns[SHANSHUI_MOUNT_J];
    for (int j = 0; j < J; j++) ns[j] = noise_.noise(i, j * 0.2f, seed);
    loopNoise(ns, J);
    for (int j = 0; j < J; j++) {
      float a = ((float)j / J) * PI * 2 - PI / 2;
      float hc = hei * cosf(a), ws = wid * sinf(a);
      float l = (wid * hei) / sqrtf(hc * hc + ws * ws);
      l *= 0.7f + 0.3f * ns[j];
      float p = 1 - (float)i / I;
      float nx = cosf(a) * l * p;
      float ny = -sinf(a) * l * p;
      if (a > PI || a < 0) ny *= 0.2f;
      ny += hei * ((float)i / I) * 0.2f;
      grid[i * J + j].x = nx;
      grid[i * J + j].y = ny;
    }
  }
  Pt bg[SHANSHUI_MOUNT_J + 1];
  for (int i = 0; i < J; i++) bg[i] = grid[i];
  bg[J].x = 0; bg[J].y = 0;
  ras_.poly(bg, J + 1, paper(), none(), 0, xoff, yoff);
  Pt edge[SHANSHUI_MOUNT_J];
  for (int i = 0; i < J; i++) {
    edge[i].x = grid[i].x + xoff; edge[i].y = grid[i].y + yoff;
  }
  brush_.stroke(edge, J, ink(100, 77), 3.0f, 1.0f, 1.0f, wfSin);
  TexArgs t;
  t.tex = tex;
  t.wid = 3.0f;
  t.sha = sha;
  t.gray = 180;
  t.a0 = 77;
  t.a1 = 153;
  t.dis = DIS_ROCK;
  brush_.texture(grid, I, J, xoff, yoff, t);
}

} // namespace shanshui
