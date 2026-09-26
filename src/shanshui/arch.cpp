#include "arch.h"
#include <math.h>

namespace shanshui {

static const float PI PROGMEM = 3.14159265f;

void Arch::hut(float xoff, float yoff, float hei, float wid, int tex) {
  const int RI = 10, RJ = 10;
  Pt grid[10 * 10];
  for (int i = 0; i < RI; i++) {
    float heir = hei + hei * 0.2f * rng_.next();
    for (int j = 0; j < RJ; j++) {
      grid[i * RJ + j].x = wid * ((float)i / (RI - 1) - 0.5f) *
                           powf((float)j / (RJ - 1), 0.7f);
      grid[i * RJ + j].y = heir * ((float)j / (RJ - 1));
    }
  }
  Pt edge[20];
  int ec = 0;
  for (int j = 0; j < RJ - 1; j++) edge[ec++] = grid[j];
  for (int j = RJ - 1; j >= 0 && ec < 20; j--) edge[ec++] = grid[(RI - 1) * RJ + j];
  ras_.poly(edge, ec, paper(), none(), 0, xoff, yoff);
  ras_.poly(grid, RJ, none(), ink(100, 77), 2.0f, xoff, yoff);
  ras_.poly(grid + (RI - 1) * RJ, RJ, none(), ink(100, 77), 2.0f, xoff, yoff);
  TexArgs t;
  t.tex = tex;
  t.wid = 1.0f;
  t.len = 0.25f;
  t.gray = 120;
  t.a0 = 77;
  t.a1 = 153;
  t.noiK = 5.0f;
  t.dis = DIS_HUT;
  brush_.texture(grid, RI, RJ, xoff, yoff, t);
}

int Arch::deco(int style, const Pt& pul, const Pt& pur, const Pt& pdl,
               const Pt& pdr, Pt lines[][8]) {
  if (style < 1 || style > 3) return 0;
  int h0 = 1, h1 = 3, v0 = 1, v1 = 2;
  if (style == 1) { h0 = 1; h1 = 5; v0 = 1; v1 = 2; }
  if (style == 2) { h0 = 1; h1 = 5; v0 = 1; v1 = 2; }
  if (style == 3) { h0 = 1; h1 = 4; v0 = 1; v1 = 3; }
  Pt dl[8], dr[8], du[8], dd[8];
  Pt e1[2] = {pul, pdl}, e2[2] = {pur, pdr}, e3[2] = {pul, pur},
     e4[2] = {pdl, pdr};
  int nl = brush_.div(e1, 2, v1, dl, 8);
  int nr = brush_.div(e2, 2, v1, dr, 8);
  int nu = brush_.div(e3, 2, h1, du, 8);
  int nd = brush_.div(e4, 2, h1, dd, 8);
  int c = 0;
  Pt tmp[8];
  if (style == 1) {
    Pt mlu = du[h0], mru = du[nu - 1 - h0], mld = dd[h0], mrd = dd[nu - 1 - h0];
    for (int i = v0; i < nl - v0 && c + 2 < 16; i += v0) {
      Pt a1[2] = {mlu, mld}, a2[2] = {mru, mrd};
      Pt mml[8], mmr[8];
      brush_.div(a1, 2, v1, mml, 8);
      brush_.div(a2, 2, v1, mmr, 8);
      Pt q1[2] = {mml[i < 8 ? i : 7], dl[i]}, q2[2] = {mmr[i < 8 ? i : 7], dr[i]};
      brush_.div(q1, 2, 5, lines[c], 8); c++;
      brush_.div(q2, 2, 5, lines[c], 8); c++;
    }
    Pt q1[2] = {mlu, mld}, q2[2] = {mru, mrd};
    if (c < 16) { brush_.div(q1, 2, 5, lines[c], 8); c++; }
    if (c < 16) { brush_.div(q2, 2, 5, lines[c], 8); c++; }
  } else if (style == 2) {
    for (int i = h0; i < nu - h0 && c < 16; i += h0) {
      Pt q[2] = {du[i], dd[i]};
      brush_.div(q, 2, 5, lines[c], 8);
      c++;
    }
  } else {
    Pt mlu = du[h0], mru = du[nu - 1 - h0], mld = dd[h0], mrd = dd[nu - 1 - h0];
    for (int i = v0; i < nl - v0 && c + 2 < 16; i += v0) {
      Pt a1[2] = {mlu, mld}, a2[2] = {mru, mrd}, a3[2] = {mlu, mru},
         a4[2] = {mld, mrd};
      Pt mml[8], mmr[8], mmu[8], mmd[8];
      brush_.div(a1, 2, v1, mml, 8);
      brush_.div(a2, 2, v1, mmr, 8);
      brush_.div(a3, 2, v1, mmu, 8);
      brush_.div(a4, 2, v1, mmd, 8);
      int k = i < 8 ? i : 7;
      Pt q1[2] = {mml[k], mmr[k]}, q2[2] = {mmu[k], mmd[k]};
      brush_.div(q1, 2, 5, lines[c], 8); c++;
      brush_.div(q2, 2, 5, lines[c], 8); c++;
    }
    Pt q1[2] = {mlu, mld}, q2[2] = {mru, mrd};
    if (c < 16) { brush_.div(q1, 2, 5, lines[c], 8); c++; }
    if (c < 16) { brush_.div(q2, 2, 5, lines[c], 8); c++; }
  }
  (void)tmp; (void)nd; (void)nr;
  return c;
}

void Arch::box(float xoff, float yoff, float hei, float wid, float rot,
               float per, bool tra, bool bot, float wei, int decoStyle) {
  float mid = -wid * 0.5f + wid * rot;
  float bmid = -wid * 0.5f + wid * (1 - rot);
  Pt store[20][8];
  int sc = 0;
  Pt e[2];
#define DIV2(ax, ay, bx, by)                       \
  e[0].x = (ax); e[0].y = (ay); e[1].x = (bx); e[1].y = (by); \
  brush_.div(e, 2, 5, store[sc], 8); sc++
  DIV2(-wid * 0.5f, -hei, -wid * 0.5f, 0);
  DIV2(wid * 0.5f, -hei, wid * 0.5f, 0);
  if (bot) {
    DIV2(-wid * 0.5f, 0, mid, per);
    DIV2(wid * 0.5f, 0, mid, per);
  }
  DIV2(mid, -hei, mid, per);
  if (tra) {
    if (bot) {
      DIV2(-wid * 0.5f, 0, bmid, -per);
      DIV2(wid * 0.5f, 0, bmid, -per);
    }
    DIV2(bmid, -hei, bmid, -per);
  }
#undef DIV2
  if (decoStyle > 0 && sc < 18) {
    float surf = (rot < 0.5f) * 2 - 1;
    Pt pul = {surf * wid * 0.5f, -hei}, pur = {mid, -hei + per},
       pdl = {surf * wid * 0.5f, 0}, pdr = {mid, per};
    Pt dl[16][8];
    int dc = deco(decoStyle, pul, pur, pdl, pdr, dl);
    for (int i = 0; i < dc && sc < 20; i++, sc++)
      for (int k = 0; k < 6; k++) store[sc][k] = dl[i][k];
  }
  if (!tra) {
    Pt po[5] = {{-wid * 0.5f, -hei}, {wid * 0.5f, -hei}, {wid * 0.5f, 0},
                {mid, per}, {-wid * 0.5f, 0}};
    ras_.poly(po, 5, paper(), none(), 0, xoff, yoff);
  }
  for (int i = 0; i < sc; i++) {
    Pt w[8];
    for (int k = 0; k < 6; k++) {
      w[k].x = store[i][k].x + xoff;
      w[k].y = store[i][k].y + yoff;
    }
    brush_.stroke(w, 6, ink(100, 102), wei, 1.0f, 1.0f, wfOne);
  }
}

void Arch::rail(float xoff, float yoff, float seed, float hei, float wid,
                float rot, float per, int seg, float wei, bool tra, bool fro) {
  float mid = -wid * 0.5f + wid * rot;
  float bmid = -wid * 0.5f + wid * (1 - rot);
  Pt pl[10][8];
  int pc = 0;
  Pt e[2];
#define RL(ax, ay, bx, by) \
  e[0].x = (ax); e[0].y = (ay); e[1].x = (bx); e[1].y = (by); \
  brush_.div(e, 2, seg, pl[pc], 8); pc++
  if (fro) {
    RL(-wid * 0.5f, 0, mid, per);
    RL(mid, per, wid * 0.5f, 0);
  }
  if (tra) {
    RL(-wid * 0.5f, 0, bmid, -per);
    RL(bmid, -per, wid * 0.5f, 0);
  }
  if (fro) {
    RL(-wid * 0.5f, -hei, mid, -hei + per);
    RL(mid, -hei + per, wid * 0.5f, -hei);
  }
  if (tra) {
    RL(-wid * 0.5f, -hei, bmid, -hei - per);
    RL(bmid, -hei - per, wid * 0.5f, -hei);
  }
#undef RL
  if (tra) {
    // 透视开口：web 会随机剪短两条线；MCU 版保持闭合（省分支，视觉无差）。
    (void)0;
  }
  for (int i = 0; i < pc / 2; i++) {
    for (int j = 0; j <= seg; j++) {
      int oi = (pc / 2 + i) % pc;
      int oj = j % (seg + 1);
      if (j > 7 || oj > 7) break;
      pl[i][j].y += (noise_.noise(i, j * 0.5f, seed) - 0.5f) * hei;
      pl[oi][oj].y += (noise_.noise(i + 0.5f, j * 0.5f, seed) - 0.5f) * hei;
      Pt ln[2] = {{pl[i][j].x + (rng_.next() - 0.5f) * hei * 0.5f + xoff,
                   pl[i][j].y + yoff},
                  {pl[oi][oj].x + xoff, pl[oi][oj].y + yoff}};
      ras_.poly(ln, 2, none(), ink(100, 128), 2.0f);
    }
  }
  for (int i = 0; i < pc; i++) {
    Pt w[8];
    for (int k = 0; k <= seg && k < 8; k++) {
      w[k].x = pl[i][k].x + xoff;
      w[k].y = pl[i][k].y + yoff;
    }
    brush_.stroke(w, seg + 1, ink(100, 128), wei, 0.5f, 1.0f, wfOne);
  }
}

void Arch::roof(float xoff, float yoff, float hei, float wid, float rot,
                float per, float cor, float wei) {
  float rrot = rot < 0.5f ? 1 - rot : rot;
  float sgn = rot < 0.5f ? -1.0f : 1.0f; // flip 等价。
  float mid = -wid * 0.5f + wid * rrot;
  float quat = (mid + wid * 0.5f) * 0.5f - mid;
  Pt store[7][8];
  Pt tri[3];
  int sc = 0;
#define RF(ax, ay, bx, by, cx, cy) \
  tri[0].x = sgn * (ax); tri[0].y = (ay); tri[1].x = sgn * (bx); tri[1].y = (by); \
  tri[2].x = sgn * (cx); tri[2].y = (cy); \
  brush_.div(tri, 3, 5, store[sc], 8); sc++
  RF(-wid * 0.5f + quat, -hei - per / 2, -wid * 0.5f + quat * 0.5f,
     -hei / 2 - per / 4, -wid * 0.5f - cor, 0);
  RF(mid + quat, -hei, (mid + quat + wid * 0.5f) / 2, -hei / 2, wid * 0.5f + cor, 0);
  RF(mid + quat, -hei, mid + quat / 2, -hei / 2 + per / 2, mid + cor, per);
#undef RF
  Pt e[2];
  e[0].x = sgn * (-wid * 0.5f - cor); e[0].y = 0;
  e[1].x = sgn * (mid + cor); e[1].y = per;
  brush_.div(e, 2, 5, store[sc], 8); sc++;
  e[0].x = sgn * (wid * 0.5f + cor); e[0].y = 0;
  e[1].x = sgn * (mid + cor); e[1].y = per;
  brush_.div(e, 2, 5, store[sc], 8); sc++;
  e[0].x = sgn * (-wid * 0.5f + quat); e[0].y = -hei - per / 2;
  e[1].x = sgn * (mid + quat); e[1].y = -hei;
  brush_.div(e, 2, 5, store[sc], 8); sc++;
  Pt po[5] = {{sgn * -wid * 0.5f, 0},
              {sgn * (-wid * 0.5f + quat), -hei - per / 2},
              {sgn * (mid + quat), -hei},
              {sgn * wid * 0.5f, 0},
              {sgn * mid, per}};
  ras_.poly(po, 5, paper(), none(), 0, xoff, yoff);
  for (int i = 0; i < sc; i++) {
    Pt w[12];
    int n = brush_.div(store[i], 6, 1, w, 12);
    for (int k = 0; k < n; k++) { w[k].x += xoff; w[k].y += yoff; }
    brush_.stroke(w, n, ink(100, 102), wei, 1.0f, 1.0f, wfOne);
  }
}

void Arch::pagroof(float xoff, float yoff, float hei, float wid, float per,
                   float cor, int sid, float wei) {
  Pt po[8];
  int pc = 0;
  po[pc++] = {0, -hei};
  Pt lines[14][3];
  int lc = 0;
  for (int i = 0; i < sid && i < 7; i++) {
    float fx = wid * ((float)i / (sid - 1) - 0.5f);
    float fy = per * (1 - fabsf((float)i / (sid - 1) - 0.5f) * 2);
    float fxx = (wid + cor) * ((float)i / (sid - 1) - 0.5f);
    if (i > 0 && lc < 14) {
      lines[lc][0] = lines[lc - 1][2];
      lines[lc][1] = {fxx, fy};
      lc++;
    }
    if (lc < 14) {
      lines[lc][0] = {0, -hei};
      lines[lc][1] = {fx * 0.5f, (-hei + fy) * 0.5f};
      lines[lc][2] = {fxx, fy};
      lc++;
    }
    if (pc < 8) po[pc++] = {fxx, fy};
  }
  ras_.poly(po, pc, paper(), none(), 0, xoff, yoff);
  for (int i = 0; i < lc; i++) {
    Pt dv[12];
    int n = brush_.div(lines[i], 3, 5, dv, 12);
    for (int k = 0; k < n; k++) { dv[k].x += xoff; dv[k].y += yoff; }
    brush_.stroke(dv, n, ink(100, 102), wei, 1.0f, 1.0f, wfOne);
  }
}

void Arch::arch01(float xoff, float yoff, float seed, float hei, float wid,
                  float rot, float per) {
  (void)rot;
  float p = 0.4f + rng_.next() * 0.2f;
  hut(xoff, yoff - hei, hei * p, wid, 300);
  box(xoff, yoff, hei * (1 - p), wid * 2 / 3, 0.7f, per, true, false, 3, 0);
  rail(xoff, yoff, seed, 10, wid, 0.7f, per * 2, 3 + (int)(rng_.next() * 3),
       1.0f, true, false);
  static const int kManCount[4] PROGMEM = {0, 1, 1, 2};
  int mcnt = rng_.choice<4>(kManCount);
  if (mcnt == 1) {
    man_.man(xoff + rng_.range(-wid / 3, wid / 3), yoff, 0.42f,
             rng_.next() < 0.5f, 0, 0, nullptr);
  } else if (mcnt == 2) {
    man_.man(xoff + rng_.range(-wid / 4, -wid / 5), yoff, 0.42f, false, 0, 0,
             nullptr);
    man_.man(xoff + rng_.range(wid / 5, wid / 4), yoff, 0.42f, true, 0, 0,
             nullptr);
  }
  rail(xoff, yoff, seed, 10, wid, 0.7f, per * 2, 3 + (int)(rng_.next() * 3),
       1.0f, false, true);
}

void Arch::arch02(float xoff, float yoff, float seed, float hei, float wid,
                  float rot, float per, int sto, int sty, bool rai) {
  (void)seed;
  float hoff = 0;
  for (int i = 0; i < sto; i++) {
    float w = wid * powf(0.85f, i);
    int hsp = sty == 3 ? 4 : 5, vsp = sty == 3 ? 3 : 2;
    (void)hsp; (void)vsp;
    box(xoff, yoff - hoff, hei, w, rot, per, false, true, 1.5f, sty);
    if (rai)
      rail(xoff, yoff - hoff, i * 0.2f, hei / 2, w * 1.1f, rot, per, 5, 0.5f,
           false, true);
    roof(xoff, yoff - hoff - hei, hei, wid * powf(0.9f, i), rot, per, 5, 1.5f);
    hoff += hei * 1.5f;
  }
}

void Arch::arch03(float xoff, float yoff, float seed, float hei, float wid,
                  float rot, float per, int sto) {
  float hoff = 0;
  for (int i = 0; i < sto; i++) {
    float w = wid * powf(0.85f, i);
    box(xoff, yoff - hoff, hei, w, rot, per / 2, false, true, 1.5f, 1);
    rail(xoff, yoff - hoff, i * 0.2f, hei / 2, w * 1.1f, rot, per / 2, 5, 0.5f,
         false, true);
    pagroof(xoff, yoff - hoff - hei, hei * 1.5f, wid * powf(0.9f, i), per, 10,
            4, 1.5f);
    hoff += hei * 1.5f;
  }
  (void)seed;
}

void Arch::arch04(float xoff, float yoff, float seed, float hei, float wid,
                  float rot, float per, int sto) {
  float hoff = 0;
  for (int i = 0; i < sto; i++) {
    float w = wid * powf(0.85f, i);
    box(xoff, yoff - hoff, hei, w, rot, per / 2, true, true, 1.5f, 0);
    rail(xoff, yoff - hoff, i * 0.2f, hei / 3, w * 1.2f, rot, per / 2, 3, 0.5f,
         true, true);
    pagroof(xoff, yoff - hoff - hei, hei, wid * powf(0.9f, i), per, 10, 4,
            1.5f);
    hoff += hei * 1.2f;
  }
  (void)seed;
}

void Arch::boat01(float xoff, float yoff, float seed, float len, float sca,
                  bool fli) {
  (void)seed;
  float dir = fli ? -1.0f : 1.0f;
  static const float BLEN[9] PROGMEM = {0, 30, 20, 30, 10, 30, 30, 30, 30};
  man_.man(xoff + 20 * sca * dir, yoff, 0.5f * sca, !fli, 1, 1, BLEN);
  Pt p1[32], p2[32];
  int c1 = 0, c2 = 0;
  for (float i = 0; i < len * sca && c1 < 32; i += 5 * sca) {
    float f = i / len;
    float s = sinf(f * PI);
    if (s <= 0) s = 0.0001f;
    float sq = sqrtf(s);
    p1[c1].x = i * dir + xoff;
    p1[c1].y = sq * 7 * sca + yoff;
    c1++;
    p2[c2].x = i * dir + xoff;
    p2[c2].y = sq * 10 * sca + yoff;
    c2++;
  }
  Pt hull[64];
  int hc = 0;
  for (int i = 0; i < c1 && hc < 63; i++) hull[hc++] = p1[i];
  for (int i = c2 - 1; i >= 0 && hc < 63; i--) hull[hc++] = p2[i];
  ras_.poly(hull, hc, paper(), none(), 0);
  brush_.stroke(hull, hc, ink(100, 102), 1.0f, 0.5f, 1.0f, wfSin);
}

void Arch::quickstroke(const Pt* pl, int n, float xoff, float yoff) {
  Pt dv[24];
  int dc = brush_.div(pl, n, 5, dv, 24);
  for (int i = 0; i < dc; i++) { dv[i].x += xoff; dv[i].y += yoff; }
  brush_.stroke(dv, dc, ink(100, 102), 1.0f, 0.5f, 0.5f, wfOne);
}

void Arch::tower01(float xoff, float yoff, float hei, float wid) {
  Pt p00 = {-wid * 0.05f, -hei}, p01 = {wid * 0.05f, -hei};
  Pt p10 = {-wid * 0.1f, -hei * 0.9f}, p11 = {wid * 0.1f, -hei * 0.9f};
  Pt p20 = {-wid * 0.2f, -hei * 0.5f}, p21 = {wid * 0.2f, -hei * 0.5f};
  Pt p30 = {-wid * 0.5f, 0}, p31 = {wid * 0.5f, 0};
  static const float BCH[3][2] PROGMEM = {{0.7f, -0.85f}, {1.0f, -0.675f}, {0.7f, -0.5f}};
  for (int i = 0; i < 3; i++) {
    float bx = BCH[i][0] * wid, by = BCH[i][1] * hei;
    Pt l1[2] = {{-bx, by}, {bx, by}};
    Pt l2[2] = {{-bx, by}, {0, (BCH[i][1] - 0.05f) * hei}};
    Pt l3[2] = {{bx, by}, {0, (BCH[i][1] - 0.05f) * hei}};
    Pt l4[2] = {{-bx, by}, {-bx, (BCH[i][1] + 0.1f) * hei}};
    Pt l5[2] = {{bx, by}, {bx, (BCH[i][1] + 0.1f) * hei}};
    quickstroke(l1, 2, xoff, yoff);
    quickstroke(l2, 2, xoff, yoff);
    quickstroke(l3, 2, xoff, yoff);
    quickstroke(l4, 2, xoff, yoff);
    quickstroke(l5, 2, xoff, yoff);
  }
  Pt l10[4] = {p00, p10, p20, p30}, l11[4] = {p01, p11, p21, p31};
  Pt d10[24], d11[24];
  int n10 = brush_.div(l10, 4, 5, d10, 24);
  int n11 = brush_.div(l11, 4, 5, d11, 24);
  for (int i = 0; i < n10 - 1 && i < n11 - 1; i++) {
    Pt a[2] = {d10[i], d11[i + 1]}, b[2] = {d11[i], d10[i + 1]};
    quickstroke(a, 2, xoff, yoff);
    quickstroke(b, 2, xoff, yoff);
  }
  Pt e1[2] = {p00, p01}, e2[2] = {p10, p11}, e3[2] = {p20, p21};
  quickstroke(e1, 2, xoff, yoff);
  quickstroke(e2, 2, xoff, yoff);
  quickstroke(e3, 2, xoff, yoff);
  quickstroke(l10, 4, xoff, yoff);
  quickstroke(l11, 4, xoff, yoff);
  (void)n10; (void)n11;
}

} // namespace shanshui
