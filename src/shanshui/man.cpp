#include "man.h"
#include <math.h>

namespace shanshui {

static float clothW(Man::ClothMode m, float x, float sca) {
  float s = sinf(x * kPi);
  if (s < 0.0001f) s = 0.0001f;
  float k = powf(s, 0.1f);
  switch (m) {
    case Man::CM_SLEEVE:
      return sca * 8.0f * (sinf(0.5f * x * kPi) * k + (1 - x) * 0.4f);
    case Man::CM_BODY:
      return sca * 11.0f * (sinf(0.5f * x * kPi) * k + (1 - x) * 0.5f);
    case Man::CM_HEAD:
    default: {
      float v = 0.25f - (x - 0.5f) * (x - 0.5f);
      if (v < 0.0001f) v = 0.0001f;
      return sca * 7.0f * powf(v, 0.3f);
    }
  }
}

int Man::expand(const Pt* pts, int n, ClothMode mode, float sca, Pt* o0,
                Pt* o1, int maxOut) {
  if (!pts || n < 2) return 0;
  int c0 = 0, c1 = 0;
  for (int i = 1; i < n - 1; i++) {
    float w = clothW(mode, (float)i / n, sca);
    float a1 = atan2f(pts[i].y - pts[i - 1].y, pts[i].x - pts[i - 1].x);
    float a2 = atan2f(pts[i].y - pts[i + 1].y, pts[i].x - pts[i + 1].x);
    float a = (a1 + a2) * 0.5f;
    if (a < a2) a += kPi;
    if (c0 < maxOut) {
      o0[c0].x = pts[i].x + w * cosf(a);
      o0[c0].y = pts[i].y + w * sinf(a);
      c0++;
    }
    if (c1 < maxOut) {
      o1[c1].x = pts[i].x - w * cosf(a);
      o1[c1].y = pts[i].y - w * sinf(a);
      c1++;
    }
  }
  int l = n - 1;
  float a0 = atan2f(pts[1].y - pts[0].y, pts[1].x - pts[0].x) - kPi / 2;
  float a1 = atan2f(pts[l].y - pts[l - 1].y, pts[l].x - pts[l - 1].x) - kPi / 2;
  float w0 = clothW(mode, 0, sca), w1 = clothW(mode, 1, sca);
  // 端帽（web 在首尾 unshift/push，与 brush.stroke 不同）。
  // 为保持缓冲顺序简单：端点直接拼接到输出头尾。
  Pt h0 = {pts[0].x + w0 * cosf(a0), pts[0].y + w0 * sinf(a0)};
  Pt h1 = {pts[0].x - w0 * cosf(a0), pts[0].y - w0 * sinf(a0)};
  Pt t0 = {pts[l].x + w1 * cosf(a1), pts[l].y + w1 * sinf(a1)};
  Pt t1 = {pts[l].x - w1 * cosf(a1), pts[l].y - w1 * sinf(a1)};
  // o0 = [h0, mid..., t0]，o1 = [h1, mid..., t1]
  for (int i = c0; i > 0; i--) o0[i] = o0[i - 1];
  o0[0] = h0;
  if (c0 + 1 < maxOut) o0[++c0] = t0; else c0++;
  for (int i = c1; i > 0; i--) o1[i] = o1[i - 1];
  o1[0] = h1;
  if (c1 + 1 < maxOut) o1[++c1] = t1; else c1++;
  return c0 < c1 ? c0 : c1; // 两侧等长（正常情况相等）。
}

void Man::tranpoly(const Pt& p0, const Pt& p1, const float* local, int n,
                   bool fli, Pt* out) {
  float ang = atan2f(p1.y - p0.y, p1.x - p0.x) - kPi / 2;
  float dx = p1.x - p0.x, dy = p1.y - p0.y;
  float scl = sqrtf(dx * dx + dy * dy);
  for (int i = 0; i < n; i++) {
    float x = local[2 * i], y = local[2 * i + 1];
    if (fli) x = -x;
    float d = sqrtf(x * x + y * y);
    float a = atan2f(y, x);
    out[i].x = p0.x + d * scl * cosf(ang + a);
    out[i].y = p0.y + d * scl * sinf(ang + a);
  }
}

void Man::hat01(const Pt& p0, const Pt& p1, bool fli) {
  static const float T[] = {-0.3f, 0.5f, 0.3f, 0.8f, 0.2f, 1.0f, 0.0f, 1.1f,
                            -0.3f, 1.15f, -0.55f, 1.0f, -0.65f, 0.5f};
  Pt pl[7];
  tranpoly(p0, p1, T, 7, fli, pl);
  ras_.poly(pl, 7, gray100(204), none(), 0);
  float seed = rng_.next();
  Pt q[10];
  for (int i = 0; i < 10; i++) {
    float x = -0.3f - noise_.noise(i * 0.2f, seed) * i * 0.1f;
    float y = 0.5f - i * 0.3f;
    if (fli) x = -x;
    // 复用 tranpoly 逻辑中的变换：这里直接展开避免二次查表。
    q[i].x = x; q[i].y = y;
  }
  // q 为局部坐标，做同样变换。
  Pt w[10];
  float ang = atan2f(p1.y - p0.y, p1.x - p0.x) - kPi / 2;
  float dx = p1.x - p0.x, dy = p1.y - p0.y;
  float scl = sqrtf(dx * dx + dy * dy);
  for (int i = 0; i < 10; i++) {
    float d = sqrtf(q[i].x * q[i].x + q[i].y * q[i].y);
    float a = atan2f(q[i].y, q[i].x);
    w[i].x = p0.x + d * scl * cosf(ang + a);
    w[i].y = p0.y + d * scl * sinf(ang + a);
  }
  ras_.poly(w, 10, none(), gray100(204), 1.0f);
}

void Man::hat02(const Pt& p0, const Pt& p1, bool fli) {
  static const float T[] = {-0.3f, 0.5f, -1.1f, 0.5f, -1.2f, 0.6f, -1.1f, 0.7f,
                            -0.3f, 0.8f, 0.3f, 0.8f, 1.0f, 0.7f, 1.3f, 0.6f,
                            1.2f, 0.5f, 0.3f, 0.5f};
  Pt pl[10];
  tranpoly(p0, p1, T, 10, fli, pl);
  ras_.poly(pl, 10, gray100(204), none(), 0);
}

void Man::stick01(const Pt& p0, const Pt& p1, bool fli) {
  float seed = rng_.next();
  Pt q[12];
  for (int i = 0; i < 12; i++) {
    float x = -noise_.noise(i * 0.1f, seed) * 0.1f * sinf((float)i / 12 * kPi) * 5.0f;
    float y = i * 0.3f;
    if (fli) x = -x;
    q[i].x = x; q[i].y = y;
  }
  Pt w[12];
  float ang = atan2f(p1.y - p0.y, p1.x - p0.x) - kPi / 2;
  float dx = p1.x - p0.x, dy = p1.y - p0.y;
  float scl = sqrtf(dx * dx + dy * dy);
  for (int i = 0; i < 12; i++) {
    float d = sqrtf(q[i].x * q[i].x + q[i].y * q[i].y);
    float a = atan2f(q[i].y, q[i].x);
    w[i].x = p0.x + d * scl * cosf(ang + a);
    w[i].y = p0.y + d * scl * sinf(ang + a);
  }
  ras_.poly(w, 12, none(), gray100(128), 1.0f);
}

void Man::cloth(const Pt* plist, int n, ClothMode mode, float sca, float xoff,
                float yoff, bool fli) {
  Pt t[64];
  int tn = bezmh(plist, n, 2.0f, t, 64);
  if (tn < 2) return;
  Pt t1[64], t2[64];
  int c = expand(t, tn, mode, sca, t1, t2, 64);
  if (c < 2) return;
  Pt poly[96], e1[64], e2[64];
  int pc = 0;
  float sx = fli ? -1.0f : 1.0f;
  for (int i = 0; i < c && pc < 90; i++) {
    poly[pc].x = t1[i].x * sx + xoff;
    poly[pc].y = t1[i].y + yoff;
    pc++;
  }
  for (int i = c - 1; i >= 0 && pc < 90; i--) {
    poly[pc].x = t2[i].x * sx + xoff;
    poly[pc].y = t2[i].y + yoff;
    pc++;
  }
  ras_.poly(poly, pc, paper(), none(), 0);
  for (int i = 0; i < c; i++) {
    e1[i].x = t1[i].x * sx + xoff; e1[i].y = t1[i].y + yoff;
    e2[i].x = t2[i].x * sx + xoff; e2[i].y = t2[i].y + yoff;
  }
  brush_.stroke(e1, c, gray100(128), 1.0f, 0.5f, 1.0f, wfSin);
  brush_.stroke(e2, c, gray100(153), 1.0f, 0.5f, 1.0f, wfSin);
}

void Man::man(float xoff, float yoff, float sca, bool fli, int hat, int ite,
              const float* lenMul) {
  static const float DEF_LEN[9] = {0, 30, 20, 30, 30, 30, 30, 30, 30};
  if (!lenMul) lenMul = DEF_LEN;
  float r1 = rng_.next(), r2 = rng_.next(), r3 = rng_.next();
  float ang[9] = {0, -kPi / 2, 0, (kPi / 4) * r1, ((kPi * 3) / 4) * r2,
                  (kPi * 3) / 4, -kPi / 4, (-kPi * 3) / 4 - (kPi / 4) * r3, -kPi / 4};
  // 骨骼路径（root=0）：各关节 parent 链。
  // 显式路径求位置。
  static const uint8_t PATH[9][4] = {
      {0, 9, 9, 9}, {0, 1, 9, 9}, {0, 1, 2, 9}, {0, 3, 9, 9}, {0, 3, 4, 9},
      {0, 1, 5, 9}, {0, 1, 5, 6}, {0, 1, 7, 9}, {0, 1, 7, 8},
  };
  Pt pts[9];
  for (int k = 0; k < 9; k++) {
    float px = 0, py = 0, rot = 0;
    for (int d = 0; d < 4 && PATH[k][d] != 9; d++) {
      int j = PATH[k][d];
      rot += ang[j];
      if (j == 0 && lenMul[0] == 0) continue;
      px += lenMul[j] * sca * cosf(rot);
      py += lenMul[j] * sca * sinf(rot);
    }
    pts[k].x = px;
    pts[k].y = py;
  }
  yoff -= pts[4].y; // 双脚落地。
  float sx = fli ? -1.0f : 1.0f;
  Pt g[9];
  for (int k = 0; k < 9; k++) {
    g[k].x = pts[k].x * sx + xoff;
    g[k].y = pts[k].y + yoff;
  }
  if (ite == 1) stick01(g[8], g[6], fli);
  { Pt s1[3] = {pts[1], pts[7], pts[8]}; cloth(s1, 3, CM_SLEEVE, sca, xoff, yoff, fli); }
  { Pt s2[4] = {pts[1], pts[0], pts[3], pts[4]}; cloth(s2, 4, CM_BODY, sca, xoff, yoff, fli); }
  { Pt s3[3] = {pts[1], pts[5], pts[6]}; cloth(s3, 3, CM_SLEEVE, sca, xoff, yoff, fli); }
  { Pt s4[2] = {pts[1], pts[2]}; cloth(s4, 2, CM_HEAD, sca, xoff, yoff, fli); }
  // 头巾（深色盖头）。
  {
    Pt h[2] = {pts[1], pts[2]};
    Pt t[64];
    int tn = bezmh(h, 2, 2.0f, t, 64);
    Pt h1[64], h2[64];
    int c = expand(t, tn, CM_HEAD, sca, h1, h2, 64);
    int d1 = c / 10, d2 = (c * 95) / 100;
    Pt poly[96];
    int pc = 0;
    for (int i = d1; i < c && pc < 90; i++) {
      poly[pc].x = h1[i].x * sx + xoff; poly[pc].y = h1[i].y + yoff; pc++;
    }
    for (int i = c - 1; i >= d2 && pc < 90; i--) {
      poly[pc].x = h2[i].x * sx + xoff; poly[pc].y = h2[i].y + yoff; pc++;
    }
    if (pc >= 3) ras_.poly(poly, pc, gray100(153), none(), 0);
  }
  if (hat == 1)
    hat02(g[1], g[2], fli);
  else
    hat01(g[1], g[2], fli);
}

} // namespace shanshui
