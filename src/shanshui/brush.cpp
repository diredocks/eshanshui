#include "brush.h"
#include <math.h>

namespace shanshui {

float wfSin(float x) { return sinf(x * kPi); }
float wfCosHalf(float x) { return cosf(x * kPi * 0.5f); }
float wfOne(float x) { (void)x; return 1.0f; }
float wfSin1(float x) { (void)x; return 0.84147098f; }
float wfSin3(float x) { return sinf(x * 3.0f * kPi); }
float wfFalloff(float x) { return -1.0f / powf(x + 1.0f, 5.0f) + 1.0f; }

float bfDefault(float x) {
  if (x <= 1) {
    float s = sinf(x * kPi);
    return sqrtf(s > 0 ? s : 0.0001f);
  }
  float s = sinf((x + 1) * kPi);
  return -sqrtf(s > 0 ? s : 0.0f);
}
float bfLeaf(float x) {
  if (x <= 1) {
    float s = sinf(x * kPi) * x;
    return sqrtf(s > 0 ? s : 0.0001f);
  }
  float s = sinf((x - 2) * kPi * (x - 2));
  return -sqrtf(s > 0 ? s : 0.0f);
}
float bfPine(float x) {
  if (x <= 1) return 2.75f * x * powf(1 - x > 0 ? 1 - x : 0.0001f, 1.0f / 1.8f);
  return 2.75f * (x - 2) * powf(x - 1 > 0 ? x - 1 : 0.0001f, 1.0f / 1.8f);
}

int bezmh(const Pt* P, int n, float w, Pt* out, int maxOut) {
  if (!P || n < 2 || !out || maxOut <= 0) return 0;
  Pt tmp[8];
  const Pt* p = P;
  int m = n;
  if (n == 2) {
    tmp[0] = P[0];
    tmp[1] = midPt(P[0], P[1]);
    tmp[2] = P[1];
    p = tmp;
    m = 3;
  }
  if (m > 8) m = 8;
  int cnt = 0;
  const int pl = 20;
  for (int j = 0; j < m - 2; j++) {
    Pt p0 = (j == 0) ? p[j] : midPt(p[j], p[j + 1]);
    Pt p1 = p[j + 1];
    Pt p2 = (j == m - 3) ? p[j + 2] : midPt(p[j + 1], p[j + 2]);
    int steps = pl + (j == m - 3 ? 1 : 0);
    for (int i = 0; i < steps && cnt < maxOut; i++) {
      float t = (float)i / pl;
      float u = (1 - t) * (1 - t) + 2 * t * (1 - t) * w + t * t;
      if (u == 0) u = 1e-6f;
      out[cnt].x = ((1 - t) * (1 - t) * p0.x + 2 * t * (1 - t) * p1.x * w + t * t * p2.x) / u;
      out[cnt].y = ((1 - t) * (1 - t) * p0.y + 2 * t * (1 - t) * p1.y * w + t * t * p2.y) / u;
      cnt++;
    }
  }
  return cnt;
}

void loopNoise(float* a, int n) {
  if (!a || n < 2) return;
  float dif = a[n - 1] - a[0];
  float mn = 100.0f, mx = -100.0f;
  for (int i = 0; i < n; i++) {
    a[i] += dif * (n - 1 - i) / (n - 1);
    if (a[i] < mn) mn = a[i];
    if (a[i] > mx) mx = a[i];
  }
  float span = mx - mn;
  if (span < 1e-6f) span = 1e-6f;
  for (int i = 0; i < n; i++) a[i] = (a[i] - mn) / span;
}

float randGaussian(Prng& rng) {
  // web Tools.wtrand 拒绝采样：y < exp(-24(x-0.5)^2)，再映射到 [-1,1]。
  for (;;) {
    float x = rng.next(), y = rng.next();
    float dx = x - 0.5f;
    if (y < expf(-24.0f * dx * dx)) return x * 2.0f - 1.0f;
  }
}

void Brush::stroke(const Pt* pts, int n, Ink col, float wid, float noi,
                   float out, WFun fun) {
  if (!pts || n == 0 || col.alpha == 0) return;
  if (!fun) fun = wfSin;
  if (n == 1) {
    blob(pts[0].x, pts[0].y, wid * 2, wid * 2, 0, col, 0);
    return;
  }
  if (n == 2) {
    float dx = pts[1].x - pts[0].x, dy = pts[1].y - pts[0].y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-6f) return;
    float hw = wid * 0.5f;
    float nx = -dy / len * hw, ny = dx / len * hw;
    Pt q[4] = {{pts[0].x + nx, pts[0].y + ny},
               {pts[1].x + nx, pts[1].y + ny},
               {pts[1].x - nx, pts[1].y - ny},
               {pts[0].x - nx, pts[0].y - ny}};
    ras_.poly(q, 4, col, col, out);
    return;
  }
  float n0 = rng_.next() * 10.0f;
  // BSS 缓冲：stroke 不递归调用自身，无重入问题。
  static Pt s_v0[SHANSHUI_STROKE_MAX], s_v1[SHANSHUI_STROKE_MAX];
  static Pt s_vtx[SHANSHUI_STROKE_MAX * 2 + 2];
  Pt* v0 = s_v0;
  Pt* v1 = s_v1;
  Pt* vtx = s_vtx;
  int c0 = 0, c1 = 0;
  for (int i = 1; i < n - 1; i++) {
    float w = wid * fun((float)i / n);
    w = w * (1 - noi) + w * noi * noise_.noise(i * 0.5f, n0);
    float a1 = atan2f(pts[i].y - pts[i - 1].y, pts[i].x - pts[i - 1].x);
    float a2 = atan2f(pts[i].y - pts[i + 1].y, pts[i].x - pts[i + 1].x);
    float a = (a1 + a2) * 0.5f;
    if (a < a2) a += kPi;
    float cw = cosf(a) * w, sw = sinf(a) * w;
    if (c0 < SHANSHUI_STROKE_MAX) {
      v0[c0].x = pts[i].x + cw; v0[c0].y = pts[i].y + sw; c0++;
    }
    if (c1 < SHANSHUI_STROKE_MAX) {
      v1[c1].x = pts[i].x - cw; v1[c1].y = pts[i].y - sw; c1++;
    }
  }
  int c = 0, cap = SHANSHUI_STROKE_MAX * 2 + 2;
  vtx[c++] = pts[0];
  for (int i = 0; i < c0 && c < cap; i++) vtx[c++] = v0[i];
  for (int i = c1 - 1; i >= 0 && c < cap; i--) vtx[c++] = v1[i];
  if (c < cap) vtx[c++] = pts[n - 1];
  if (c < cap) vtx[c++] = pts[0];
  ras_.poly(vtx, c, col, col, out);
}

void Brush::blob(float x, float y, float len, float wid, float ang, Ink col,
                 float noi, BFun fun) {
  if (col.alpha == 0) return;
  if (!fun) fun = bfDefault;
  const int reso = 20;
  float lal[21], laa[21], ns[21];
  float n0 = rng_.next() * 10.0f;
  for (int i = 0; i <= reso; i++) {
    float p = (float)i / reso * 2.0f;
    float xo = len * 0.5f - fabsf(p - 1) * len;
    float yo = fun(p) * wid * 0.5f;
    lal[i] = sqrtf(xo * xo + yo * yo);
    laa[i] = atan2f(yo, xo);
    ns[i] = noise_.noise(i * 0.05f, n0);
  }
  loopNoise(ns, reso + 1);
  Pt pl[21];
  for (int i = 0; i <= reso; i++) {
    float s = ns[i] * noi + (1 - noi);
    pl[i].x = x + cosf(laa[i] + ang) * lal[i] * s;
    pl[i].y = y + sinf(laa[i] + ang) * lal[i] * s;
  }
  ras_.poly(pl, reso + 1, col, col, 0);
}

int Brush::div(const Pt* in, int n, int reso, Pt* out, int maxOut) {
  if (!in || n <= 0 || !out || maxOut <= 0) return 0;
  if (reso < 1) reso = 1;
  int tl = (n - 1) * reso;
  int cnt = 0;
  for (int i = 0; i < tl && cnt < maxOut; i++) {
    int a = i / reso, b = (i + reso - 1) / reso; // ceil(i/reso)
    if (b > n - 1) b = n - 1;
    float p = (float)(i % reso) / reso;
    out[cnt].x = in[a].x * (1 - p) + in[b].x * p;
    out[cnt].y = in[a].y * (1 - p) + in[b].y * p;
    cnt++;
  }
  if (cnt < maxOut && n > 0) out[cnt++] = in[n - 1];
  return cnt;
}

float Brush::disPick() {
  switch (disMode_) {
    case DIS_HUT: // wtrand(a*a) 拒绝采样。
      for (;;) {
        float x = rng_.next(), y = rng_.next();
        if (y < x * x) return x;
      }
    case DIS_ROCK:
      return rng_.next() > 0.5f ? 0.15f + 0.15f * rng_.next()
                                : 0.85f - 0.15f * rng_.next();
    case DIS_FLAT:
      return rng_.next() > 0.5f ? 0.1f + 0.4f * rng_.next()
                                : 0.9f - 0.4f * rng_.next();
    case DIS_MID:
    default:
      return rng_.next() > 0.5f ? (1.0f / 3) * rng_.next()
                                : (1.0f * 2) / 3 + (1.0f / 3) * rng_.next();
  }
}

int Brush::texLayer(const Pt* grid, int I, int J, float xof, float yof,
                    const TexArgs& a, float layer, Pt* line) {
  int mid = (int)(disPick() * J);
  int hlen = (int)(rng_.next() * (J * a.len));
  int s = mid - hlen; if (s < 0) s = 0;
  int e = mid + hlen; if (e > J) e = J;
  int fl = (int)layer, ce = fl + 1; if (ce > I - 1) ce = I - 1;
  float p = layer - fl;
  float nk = a.noiK >= 0 ? a.noiK : 30.0f / (layer + 1);
  int c = 0;
  for (int j = s; j < e && c < SHANSHUI_MOUNT_J; j++) {
    float gx = grid[fl * J + j].x * p + grid[ce * J + j].x * (1 - p);
    float gy = grid[fl * J + j].y * p + grid[ce * J + j].y * (1 - p);
    line[c].x = gx + nk * (noise_.noise(gx, j * 0.5f) - 0.5f) + xof;
    line[c].y = gy + nk * (noise_.noise(gy, j * 0.5f) - 0.5f) + yof;
    c++;
  }
  return c;
}

void Brush::texture(const Pt* grid, int I, int J, float xof, float yof,
                    const TexArgs& a) {
  if (!grid || I < 2 || J < 2 || a.tex <= 0) return;
  disMode_ = a.dis;
  int tex = a.tex / SHANSHUI_TEX_DIV;
  if (tex < 8) tex = 8;
  int step = (a.sha > 0) ? 2 : 1;
  Pt line[SHANSHUI_MOUNT_J];
  // SHADE 层（淡影）。
  for (int i = 0; i < tex && a.sha > 0; i += step) {
    float layer = (float)i / tex * (I - 1);
    int c = texLayer(grid, I, J, xof, yof, a, layer, line);
    if (c >= 2) stroke(line, c, gray100(26), a.sha, 0.5f, 1.0f, wfSin);
  }
  // TEXTURE 层。
  for (int i = (a.sha > 0 ? 1 : 0); i < tex; i += step) {
    float layer = (float)i / tex * (I - 1);
    int c = texLayer(grid, I, J, xof, yof, a, layer, line);
    if (c >= 2) {
      uint8_t al = a.a0 + (uint8_t)(rng_.next() * (a.a1 - a.a0));
      stroke(line, c, ink(a.gray, al), a.wid, 0.5f, 1.0f, wfSin);
    }
  }
}

} // namespace shanshui
