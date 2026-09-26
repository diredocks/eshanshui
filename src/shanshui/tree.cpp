#include "tree.h"
#include <math.h>

namespace shanshui {

static const float PI = 3.14159265f;
static const int CH = 8; // choice([-1,1])
// 正负号二选一（branch 抖动方向 / twig 分叉方向共用，原为两处栈上复合字面量）。
static const int kSign[2] = {-1, 1};

float Tree::shape(float x) { return logf(50.0f * x + 1.0f) / 3.95f; }

int Tree::branch(float hei, float wid, float ang, float ben, float det,
                 Pt* o0, Pt* o1, int maxOut) {
  Pt t[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
  float nx = 0, ny = 0, a0 = 0;
  for (int i = 0; i < 3; i++) {
    int s = rng_.choice<2>(kSign);
    a0 += (ben * 0.5f + rng_.next() * ben * 0.5f) * s;
    nx += cosf(a0) * hei / 3.0f;
    ny -= sinf(a0) * hei / 3.0f;
    t[i + 1].x = nx;
    t[i + 1].y = ny;
  }
  float ta = atan2f(t[3].y, t[3].x);
  for (int i = 0; i < 4; i++) {
    float a = atan2f(t[i].y, t[i].x);
    float d = sqrtf(t[i].x * t[i].x + t[i].y * t[i].y);
    t[i].x = d * cosf(a - ta + ang);
    t[i].y = d * sinf(a - ta + ang);
  }
  if (det < 2) det = 2;
  int tl = (int)(3 * det);
  if (tl > maxOut) tl = maxOut;
  float lx = 0, ly = 0;
  for (int i = 0; i < tl; i++) {
    float fi = (float)i / det;
    int seg = (int)fi;
    if (seg > 2) seg = 2;
    float p = fi - seg;
    float qx = t[seg].x * (1 - p) + t[seg + 1].x * p;
    float qy = t[seg].y * (1 - p) + t[seg + 1].y * p;
    float aa = atan2f(qy - ly, qx - lx);
    float woff = (noise_.noise(i * 0.3f) - 0.5f) * wid * hei / 80.0f;
    float b = (p == 0.0f) ? rng_.next() * wid : 0;
    float nw = wid * (((tl - i) / (float)tl) * 0.5f + 0.5f);
    o0[i].x = qx + cosf(aa + PI / 2) * (nw + woff + b);
    o0[i].y = qy + sinf(aa + PI / 2) * (nw + woff + b);
    o1[i].x = qx + cosf(aa - PI / 2) * (nw - woff + b);
    o1[i].y = qy + sinf(aa - PI / 2) * (nw - woff + b);
    lx = qx;
    ly = qy;
  }
  return tl;
}

void Tree::twig(float tx, float ty, int dep, int dir, float sca, float wid,
                float ang, bool lea, float leaSz) {
  if (dep < 0 || dep > 3) return;
  Pt tw[10];
  float hs = rng_.next() * 0.5f + 0.5f;
  float a0 = (rng_.next() * PI / 6) * dir + ang;
  for (int i = 0; i < 10; i++) {
    float mx = dir * wfFalloff((float)i / 10) * 50.0f * sca * hs;
    float my = -i * 5.0f * sca;
    float a = atan2f(my, mx);
    float d = sqrtf(mx * mx + my * my);
    tw[i].x = cosf(a + a0) * d + tx;
    tw[i].y = sinf(a + a0) * d + ty;
    if ((i == 3 || i == 6) && dep > 0) {
      int nd = dir * rng_.choice<2>(kSign);
      twig(tw[i].x, tw[i].y, dep - 1, nd, sca * 0.8f, wid, ang, lea, leaSz);
    }
    if (i == 9 && lea) {
      for (int j = 0; j < 5; j++) {
        float dj = (j - 2.5f) * 5.0f;
        brush_.blob(tw[i].x + cosf(ang) * dj * wid,
                    tw[i].y + (sinf(ang) * dj - leaSz / (dep + 1)) * wid,
                    (15.0f + 12.0f * rng_.next()) * wid,
                    (6.0f + 3.0f * rng_.next()) * wid,
                    ang * 0.5f + PI / 2 + PI * 0.2f * (rng_.next() - 0.5f),
                    ink(100, (uint8_t)(128 + dep * 51)), 0.5f, bfLeaf);
      }
    }
  }
  brush_.stroke(tw, 10, ink(100, 128), 1.0f, 0.5f, 1.0f, wfCosHalf);
}

void Tree::barkify(float x, float y, const Pt* s0, const Pt* s1, int n) {
  for (int i = 2; i < n - 1; i++) {
    float a0 = atan2f(s0[i].y - s0[i - 1].y, s0[i].x - s0[i - 1].x);
    float a1 = atan2f(s1[i].y - s1[i - 1].y, s1[i].x - s1[i - 1].x);
    float p = rng_.next();
    float nx = s0[i].x * (1 - p) + s1[i].x * p;
    float ny = s0[i].y * (1 - p) + s1[i].y * p;
    if (rng_.next() < 0.2f) {
      brush_.blob(nx + x, ny + y, 15.0f, 6.0f - fabsf(p - 0.5f) * 10.0f,
                  (a0 + a1) * 0.5f, ink(100, 153), 1.0f, bfDefault);
    } else {
      // 树皮短笔（web 内联 bark 闭包）。
      Pt brk[21];
      float len = 10.0f + 10.0f * rng_.next();
      float n0 = rng_.next() * 10.0f;
      float ns[21];
      for (int k = 0; k <= 20; k++) ns[k] = noise_.noise(k * 0.05f, n0);
      loopNoise(ns, 21);
      float bw = 5.0f - fabsf(p - 0.5f) * 10.0f;
      float ba = (a0 + a1) * 0.5f;
      for (int k = 0; k <= 20; k++) {
        float pp = (float)k / 20 * 2.0f;
        float xo = len * 0.5f - fabsf(pp - 1) * len;
        float yo = bfDefault(pp) * bw * 0.5f;
        float l = sqrtf(xo * xo + yo * yo), aa = atan2f(yo, xo);
        float s = ns[k] * 0.5f + 0.5f;
        brk[k].x = nx + x + cosf(aa + ba) * l * s;
        brk[k].y = ny + y + sinf(aa + ba) * l * s;
      }
      brush_.stroke(brk, 21, ink(100, 102), 0.8f, 0.0f, 0.0f, wfSin3);
    }
    if (rng_.next() < 0.05f) {
      int jl = (int)(rng_.next() * 2 + 2);
      float bx = (rng_.next() < 0.5f ? s0[i].x : s1[i].x);
      float by = (rng_.next() < 0.5f ? s0[i].y : s1[i].y);
      float ba2 = (rng_.next() < 0.5f ? a0 : a1);
      for (int j = 0; j < jl; j++) {
        brush_.blob(bx + x + cosf(ba2) * (j - jl * 0.5f) * 4.0f,
                    by + y + sinf(ba2) * (j - jl * 0.5f) * 4.0f, 4.0f + 6.0f * rng_.next(),
                    4.0f, a0 + PI / 2, ink(100, 153), 0.5f, bfDefault);
      }
    }
  }
  // 皴线（分组描边，组长 ≤12 以适配静态缓冲）。
  Pt grp[12];
  int gc = 0;
  Pt tmp[96], dv[48];
  int tc = 0;
  for (int i = 0; i < 2 * n && tc < 96; i++) {
    const Pt& v = (i < n) ? s0[i] : s1[2 * n - 1 - i];
    tmp[tc++] = v;
  }
  for (int i = 0; i < tc; i++) {
    if (gc < 12) { grp[gc++] = tmp[i]; }
    if ((rng_.next() < 0.5f || gc >= 12 || i == tc - 1) && gc >= 2) {
      int dc = brush_.div(grp, gc, 4, dv, 48);
      for (int j = 0; j < dc; j++) {
        dv[j].x += (noise_.noise(i, j * 0.1f, 1) - 0.5f) * (15 + 5 * randGaussian(rng_));
        dv[j].y += (noise_.noise(i, j * 0.1f, 2) - 0.5f) * (15 + 5 * randGaussian(rng_));
        dv[j].x += x;
        dv[j].y += y;
      }
      brush_.stroke(dv, dc, ink(100, 179), 1.5f, 0.5f, 0.0f, wfSin);
      gc = 0;
    }
  }
}

void Tree::tree01(float x, float y, float hei, float wid, uint8_t gray,
                  uint8_t alpha) {
  const int reso = 10;
  float nsx[10], nsy[10];
  for (int i = 0; i < reso; i++) {
    nsx[i] = noise_.noise(i * 0.5f);
    nsy[i] = noise_.noise(i * 0.5f, 0.5f);
  }
  Pt l1[10], l2[10];
  for (int i = 0; i < reso; i++) {
    float nx = x, ny = y - i * hei / reso;
    if (i >= 3) {
      int cnt = (int)((reso - i) / 5.0f) + 1;
      for (int j = 0; j < cnt; j++) {
        uint8_t a = alpha + (uint8_t)(rng_.next() * 51);
        if (a < alpha) a = 255;
        brush_.blob(nx + (rng_.next() - 0.5f) * wid * 1.2f * (reso - i),
                    ny + (rng_.next() - 0.5f) * wid,
                    rng_.next() * 20 * (reso - i) * 0.2f + 10,
                    rng_.next() * 6 + 3, (rng_.next() - 0.5f) * PI / 6,
                    ink(gray, a), 0.5f, bfDefault);
      }
    }
    l1[i].x = nx + (nsx[i] - 0.5f) * wid - wid * 0.5f;
    l1[i].y = ny;
    l2[i].x = nx + (nsy[i] - 0.5f) * wid + wid * 0.5f;
    l2[i].y = ny;
  }
  Ink col = ink(gray, alpha);
  ras_.poly(l1, reso, none(), col, 1.5f);
  ras_.poly(l2, reso, none(), col, 1.5f);
}

void Tree::tree02(float x, float y, float hei, float wid, int clu,
                  uint8_t gray, uint8_t alpha) {
  for (int i = 0; i < clu; i++) {
    brush_.blob(x + randGaussian(rng_) * clu * 4,
                y + randGaussian(rng_) * clu * 4,
                rng_.next() * hei * 0.75f + hei * 0.5f,
                rng_.next() * wid * 0.75f + wid * 0.5f, PI / 2,
                ink(gray, alpha), 0.5f, bfLeaf);
  }
}

void Tree::tree03(float x, float y, float hei, float wid, float benC,
                  float benP, uint8_t gray, uint8_t alpha) {
  const int reso = 10;
  float nsx[10], nsy[10];
  for (int i = 0; i < reso; i++) {
    nsx[i] = noise_.noise(i * 0.5f);
    nsy[i] = noise_.noise(i * 0.5f, 0.5f);
  }
  Pt l1[10], l2[10];
  for (int i = 0; i < reso; i++) {
    float t = (float)i / reso;
    float nx = x + powf(t * benC > 0 ? t * benC : 0.0001f, benP) * 100.0f;
    float ny = y - i * hei / reso;
    if (i >= 2) {
      for (int j = 0; j < (reso - i) * 2; j++) {
        float ox = rng_.next() * wid * 2 * shape((reso - i) / (float)reso);
        uint8_t a = alpha + (uint8_t)(rng_.next() * 51);
        if (a < alpha) a = 255;
        brush_.blob(nx + ox * (rng_.next() < 0.5f ? -1 : 1),
                    ny + (rng_.next() - 0.5f) * wid * 2, ox * 2,
                    rng_.next() * 6 + 3, (rng_.next() - 0.5f) * PI / 6,
                    ink(gray, a), 0.5f, bfDefault);
      }
    }
    l1[i].x = nx + ((nsx[i] - 0.5f) * wid - wid * 0.5f) * (reso - i) / reso;
    l1[i].y = ny;
    l2[i].x = nx + ((nsy[i] - 0.5f) * wid + wid * 0.5f) * (reso - i) / reso;
    l2[i].y = ny;
  }
  Pt lc[20];
  for (int i = 0; i < reso; i++) lc[i] = l1[i];
  for (int i = 0; i < reso; i++) lc[reso + i] = l2[reso - 1 - i];
  ras_.poly(lc, 20, paper(), ink(gray, alpha), 1.5f);
}

void Tree::tree04(float x, float y, float hei, float wid, uint8_t gray,
                  uint8_t alpha) {
  Pt s0[48], s1[48];
  int n = branch(hei, wid, -PI / 2, PI * 0.2f, 10, s0, s1, 48);
  // 先铺白底+边缘（web 顺序的等价前置，避免白底盖掉皴笔）。
  Pt tr[96];
  int tc = 0;
  for (int i = 0; i < n && tc < 90; i++) {
    tr[tc].x = s0[i].x + x; tr[tc].y = s0[i].y + y; tc++;
  }
  for (int i = n - 1; i >= 0 && tc < 90; i--) {
    tr[tc].x = s1[i].x + x; tr[tc].y = s1[i].y + y; tc++;
  }
  ras_.poly(tr, tc, paper(), none(), 0);
  barkify(x, y, s0, s1, n);
  // 侧枝 + 嫩枝（枝形并入各自白底，不合并进主干多边形，省 RAM）。
  for (int i = 0; i < n; i++) {
    bool edge = (i >= n * 0.3f && i <= n * 0.7f && rng_.next() < 0.1f) ||
                i == n / 2 - 1;
    if (!edge) continue;
    float ba = PI * 0.2f - PI * 1.4f * (i > n / 2 ? 1 : 0);
    Pt b0[48], b1[48];
    int bn = branch(hei * (rng_.next() + 1) * 0.3f, wid * 0.5f, ba,
                    PI * 0.2f, 10, b0, b1, 48);
    // 枝白底。
    Pt bp[96];
    int bc = 0;
    for (int k = 1; k < bn && bc < 90; k++) {
      bp[bc].x = b0[k].x + s0[i].x + x; bp[bc].y = b0[k].y + s0[i].y + y; bc++;
    }
    for (int k = bn - 1; k >= 1 && bc < 90; k--) {
      bp[bc].x = b1[k].x + s0[i].x + x; bp[bc].y = b1[k].y + s0[i].y + y; bc++;
    }
    if (bc >= 3) ras_.poly(bp, bc, paper(), none(), 0);
    // 枝皴。
    for (int k = 0; k < bn; k++) {
      b0[k].x += s0[i].x; b0[k].y += s0[i].y;
      b1[k].x += s0[i].x; b1[k].y += s0[i].y;
    }
    barkify(x, y, b0, b1, bn);
    for (int j = 0; j < bn; j++) {
      if (rng_.next() < 0.2f || j == bn - 1) {
        twig(b0[j].x + x, b0[j].y + y, 1, ba > -PI / 2 ? 1 : -1, 0.5f * hei / 300,
             hei / 300, ba > -PI / 2 ? ba : ba + PI, true, 12);
      }
    }
  }
  // 主干边缘线。
  Pt e[96];
  int ec = 0;
  for (int i = 1; i < tc - 1 && ec < 96; i++) e[ec++] = tr[i];
  uint8_t ea = 102 + (uint8_t)(rng_.next() * 26);
  brush_.stroke(e, ec, ink(100, ea), 2.5f, 0.9f, 0.0f, wfSin1);
  (void)gray;
  (void)alpha;
}

void Tree::tree05(float x, float y, float hei, float wid, uint8_t gray,
                  uint8_t alpha) {
  Pt s0[48], s1[48];
  int n = branch(hei, wid, -PI / 2, 0.0f, 10, s0, s1, 48);
  Pt tr[96];
  int tc = 0;
  for (int i = 0; i < n && tc < 90; i++) {
    tr[tc].x = s0[i].x + x; tr[tc].y = s0[i].y + y; tc++;
  }
  for (int i = n - 1; i >= 0 && tc < 90; i--) {
    tr[tc].x = s1[i].x + x; tr[tc].y = s1[i].y + y; tc++;
  }
  ras_.poly(tr, tc, paper(), none(), 0);
  barkify(x, y, s0, s1, n);
  for (int i = 0; i < n; i++) {
    float p = fabsf(i - n * 0.5f) / (n * 0.5f);
    bool edge = ((i >= n * 0.2f && i <= n * 0.8f && i % 3 == 0 &&
                  rng_.next() > p) ||
                 i == n / 2 - 1);
    if (!edge) continue;
    float bar = rng_.next() * 0.2f;
    float ba = -bar * PI - (1 - bar * 2) * PI * (i > n / 2 ? 1 : 0);
    Pt b0[48], b1[48];
    int bn = branch(hei * (0.3f * p - rng_.next() * 0.05f), wid * 0.5f, ba,
                    0.5f, 10, b0, b1, 48);
    Pt bp[96];
    int bc = 0;
    for (int k = 1; k < bn && bc < 90; k++) {
      bp[bc].x = b0[k].x + s0[i].x + x; bp[bc].y = b0[k].y + s0[i].y + y; bc++;
    }
    for (int k = bn - 1; k >= 1 && bc < 90; k--) {
      bp[bc].x = b1[k].x + s0[i].x + x; bp[bc].y = b1[k].y + s0[i].y + y; bc++;
    }
    if (bc >= 3) ras_.poly(bp, bc, paper(), none(), 0);
    for (int k = 0; k < bn; k++) {
      b0[k].x += s0[i].x; b0[k].y += s0[i].y;
    }
    barkify(x, y, b0, b1, bn);
    for (int j = 0; j < bn; j++) {
      if (j % 20 == 0 || j == bn - 1) {
        twig(b0[j].x + x, b0[j].y + y, 0, ba > -PI / 2 ? 1 : -1,
             0.2f * hei / 300, hei / 300, ba > -PI / 2 ? ba : ba + PI, true, 5);
      }
    }
  }
  Pt e[96];
  int ec = 0;
  for (int i = 1; i < tc - 1 && ec < 96; i++) e[ec++] = tr[i];
  uint8_t ea = 102 + (uint8_t)(rng_.next() * 26);
  brush_.stroke(e, ec, ink(100, ea), 2.5f, 0.9f, 0.0f, wfSin1);
  (void)gray; (void)alpha;
}

void Tree::frac06(float xoff, float yoff, int dep, float hei, float wid,
                  float ang, float ben) {
  if (dep < 0) return;
  Pt s0[48], s1[48];
  float det = hei / 20.0f;
  if (det < 2) det = 2;
  int n = branch(hei, wid, ang, ben, det, s0, s1, 40);
  for (int i = 0; i < n; i++) {
    bool fork = ((rng_.next() < 0.025f && i >= n * 0.2f && i <= n * 0.8f) ||
                 i == n / 2 - 1 || i == n / 2 + 1) && dep > 0;
    if (!fork) continue;
    float bar = 0.02f + rng_.next() * 0.08f;
    float ba = bar * PI - bar * 2 * PI * (i > n / 2 ? 1 : 0);
    float cx = s0[i].x + xoff, cy = s0[i].y + yoff;
    frac06(cx, cy, dep - 1, hei * (0.7f + rng_.next() * 0.2f), wid * 0.6f,
           ang + ba, 0.55f);
    // 子枝嫩枝。
    if (rng_.next() < 0.15f) {
      twig(cx, cy, 2, ba > 0 ? 1 : -1, 0.3f, 1.0f,
           ba * (rng_.next() * 0.5f + 0.75f), false, 0);
    }
  }
  Pt poly[96];
  int pc = 0;
  for (int i = 0; i < n && pc < 90; i++) {
    poly[pc].x = s0[i].x + xoff; poly[pc].y = s0[i].y + yoff; pc++;
  }
  for (int i = n - 1; i >= 0 && pc < 90; i--) {
    poly[pc].x = s1[i].x + xoff; poly[pc].y = s1[i].y + yoff; pc++;
  }
  if (pc >= 3) ras_.poly(poly, pc, paper(), none(), 0);
  barkify(xoff, yoff, s0, s1, n);
}

void Tree::tree06(float x, float y, float hei, float wid, uint8_t gray,
                  uint8_t alpha) {
  frac06(x, y, 2, hei, wid, -PI / 2, 0.0f); // 深 2（原 3，MCU 省栈）。
  // 主干边缘线（近似：竖向描边）。
  Pt e[8] = {{x, y}, {x, y - hei * 0.5f}, {x, y - hei}};
  uint8_t ea = 102 + (uint8_t)(rng_.next() * 26);
  brush_.stroke(e, 3, ink(100, ea), 2.5f, 0.9f, 0.0f, wfSin1);
  (void)gray; (void)alpha;
}

void Tree::tree07(float x, float y, float hei, float wid, uint8_t gray,
                  uint8_t alpha) {
  const int reso = 10;
  Pt l1[10], l2[10];
  for (int i = 0; i < reso; i++) {
    float t = (float)i / reso;
    float nx = x + sqrtf(t > 0 ? t : 0.0001f) * 0.2f * 100.0f;
    float ny = y - i * hei / reso;
    if (i >= 3) {
      Pt bp[21];
      float bx = nx + (rng_.next() - 0.5f) * wid * 1.2f * (reso - i) * 0.5f;
      float by = ny + (rng_.next() - 0.5f) * wid * 0.5f;
      float bl = rng_.next() * 50 + 20, bw = rng_.next() * 12 + 12;
      float n0 = rng_.next() * 10.0f;
      float ns[21];
      for (int k = 0; k <= 20; k++) ns[k] = noise_.noise(k * 0.05f, n0);
      loopNoise(ns, 21);
      for (int k = 0; k <= 20; k++) {
        float pp = (float)k / 20 * 2.0f;
        float xo = bl * 0.5f - fabsf(pp - 1) * bl;
        float yo = bfPine(pp) * bw * 0.5f;
        float l = sqrtf(xo * xo + yo * yo), aa = atan2f(yo, xo);
        float s = ns[k] * 0.5f + 0.5f;
        float ba = -rng_.next() * PI / 6;
        bp[k].x = bx + cosf(aa + ba) * l * s;
        bp[k].y = by + sinf(aa + ba) * l * s;
      }
      // 叶块按中心噪声定灰（原 triangulate shading 的等价简化）。
      float mx = 0, my = 0;
      for (int k = 0; k <= 20; k++) { mx += bp[k].x; my += bp[k].y; }
      mx /= 21; my /= 21;
      uint8_t c = (uint8_t)(noise_.noise(mx * 0.02f, my * 0.02f) * 200 + 50);
      ras_.poly(bp, 21, ink(c, 204), ink(c, 204), 0);
    }
    l1[i].x = nx + (noise_.noise(i * 0.5f) - 0.5f) * wid - wid * 0.5f;
    l1[i].y = ny;
    l2[i].x = nx + (noise_.noise(i * 0.5f, 0.5f) - 0.5f) * wid + wid * 0.5f;
    l2[i].y = ny;
  }
  Pt lc[20];
  for (int i = 0; i < reso; i++) lc[i] = l1[i];
  for (int i = 0; i < reso; i++) lc[reso + i] = l2[reso - 1 - i];
  float mx = 0, my = 0;
  for (int i = 0; i < 20; i++) { mx += lc[i].x; my += lc[i].y; }
  mx /= 20; my /= 20;
  uint8_t c = (uint8_t)(noise_.noise(mx * 0.02f, my * 0.02f) * 200 + 50);
  ras_.poly(lc, 20, ink(c, 204), ink(gray, alpha), 0);
  (void)gray;
}

void Tree::frac08(float xoff, float yoff, int dep, float ang, float len,
                  float ben) {
  if (dep < 0 || dep > 4) return;
  Pt a = {xoff, yoff};
  Pt b = {xoff + cosf(ang) * len, yoff + sinf(ang) * len};
  Pt seg[2] = {a, {xoff + len, yoff}};
  Pt dv[12];
  int dc = brush_.div(seg, 2, 10, dv, 12);
  float pick = rng_.next() < 0.5f ? 1.0f : -1.0f;
  for (int i = 0; i < dc; i++) {
    float f = (float)i / (dc > 1 ? dc - 1 : 1);
    dv[i].y += sinf(f * PI) * pick * 2.0f;
  }
  for (int i = 0; i < dc; i++) {
    float dx = dv[i].x - a.x, dy = dv[i].y - a.y;
    float d = sqrtf(dx * dx + dy * dy), aa = atan2f(dy, dx);
    dv[i].x = a.x + d * cosf(aa + ang);
    dv[i].y = a.y + d * sinf(aa + ang);
  }
  brush_.stroke(dv, dc, ink(100, 128), 0.8f, 0.5f, 1.0f,
                dep == 0 ? wfCosHalf : wfOne);
  if (dep == 0) return;
  float nben = ben + (rng_.next() < 0.5f ? -1 : 1) * PI * 0.001f * dep * dep;
  float nl = len * rng_.range(0.8f, 0.9f);
  if (rng_.next() < 0.5f) {
    int s1 = rng_.next() < 0.5f ? -1 : 1, s2 = rng_.next() < 0.5f ? -1 : 1;
    float r1 = s1 > 0 ? rng_.range(0.5f, 1.0f) : rng_.range(-1.0f, 0.5f);
    float r2 = s2 > 0 ? rng_.range(0.5f, 1.0f) : rng_.range(-1.0f, -0.5f);
    frac08(b.x, b.y, dep - 1, ang + ben + PI * r1 * 0.2f, nl, nben);
    frac08(b.x, b.y, dep - 1, ang + ben + PI * r2 * 0.2f, nl, nben);
  } else {
    frac08(b.x, b.y, dep - 1, ang + ben, nl, nben);
  }
}

void Tree::tree08(float x, float y, float hei, float wid, uint8_t gray,
                  uint8_t alpha) {
  float ang = rng_.range(-1, 1) * PI * 0.2f;
  Pt s0[48], s1[48];
  float det = hei / 20.0f;
  if (det < 2) det = 2;
  if (det > 10) det = 10;
  int n = branch(hei, wid, -PI / 2 + ang, PI * 0.2f, det, s0, s1, 48);
  Pt tr[96];
  int tc = 0;
  for (int i = 0; i < n && tc < 90; i++) {
    tr[tc].x = s0[i].x + x; tr[tc].y = s0[i].y + y; tc++;
  }
  for (int i = n - 1; i >= 0 && tc < 90; i--) {
    tr[tc].x = s1[i].x + x; tr[tc].y = s1[i].y + y; tc++;
  }
  ras_.poly(tr, tc, paper(), ink(gray, alpha), 0);
  for (int i = 0; i < n; i++) {
    if (rng_.next() < 0.2f) {
      frac08(x + s0[i].x, y + s0[i].y, (int)(4 * rng_.next()), -PI / 2 - ang * rng_.next(), 15, 0);
    } else if (i == n / 2) {
      frac08(x + s0[i].x, y + s0[i].y, 3, -PI / 2 + ang, 15, 0);
    }
  }
  Pt e[96];
  int ec = 0;
  for (int i = 0; i < tc && ec < 96; i++) e[ec++] = tr[i];
  uint8_t ea = 153 + (uint8_t)(rng_.next() * 26);
  brush_.stroke(e, ec, ink(100, ea), 2.5f, 0.9f, 0.0f, wfSin1);
}

} // namespace shanshui
