// shanshui/shanshui.cpp — 场景组装（web/js/scene/chunks.js+view.js 的 MCU 版）。
// 策略差异（省 RAM）：web 先把每个 chunk 的绘图 op 存下来再回放；
// 这里 chunk 按 y 排好序后直接画进帧缓冲，不存 op 表。
#include "shanshui.h"
#include "arch.h"
#include "mount.h"
#include "planner.h"
#include "water.h"

namespace shanshui {

namespace {
enum Kind : uint8_t { K_MOUNT = 0, K_WATER, K_FLAT, K_DIST, K_BOAT };
struct Item {
  float ySort; // 深度键（水 y-10000，最先画=最远）。
  Kind kind;
  float x, y;
  float p0; // 序号（种子派生用）。
};

void sortItems(Item* a, int n) { // 插入排序（n 小，省 flash）。
  for (int i = 1; i < n; i++) {
    Item k = a[i];
    int j = i - 1;
    while (j >= 0 && a[j].ySort > k.ySort) {
      a[j + 1] = a[j];
      j--;
    }
    a[j + 1] = k;
  }
}
} // namespace

void renderGrayStrBand(const char* seed, uint8_t* grayBand, int y0, int h,
                       float cursx) {
  if (!grayBand) return;
  if (y0 < 0) { h += y0; y0 = 0; }
  if (y0 + h > SHANSHUI_H) h = SHANSHUI_H - y0;
  if (h <= 0) return;
  Prng rng;
  rng.seedStr(seed ? seed : "0");
  Noise noise(rng);
  Raster ras(grayBand, y0, h); // 定带后构造内 reset，只清带内。
  Brush brush(rng, noise, ras);
  Man man(rng, noise, brush, ras);
  Tree tree(rng, noise, brush, ras);
  Arch arch(rng, noise, brush, ras, man);
  Mount mount(rng, noise, brush, ras, tree, arch);
  Water water(rng, noise, brush);
  Planner planner(rng, noise);

  // 同一种子每带重播种 → RNG 流完全一致，带内行与整幅逐像素相同。
  // setTransform 仍传全局参数，Raster::py 内部减 band_y0_。
  ras.setTransform(SHANSHUI_ZOOM, -cursx * SHANSHUI_ZOOM, SHANSHUI_ZOOM, 0);

  // 按 web ChunkManager.chunkloader 的三窗口收集 regs。
  static Reg s_regs[128];
  int nreg = 0;
  planner.resetPlan();
  planner.plan(cursx, cursx + SHANSHUI_CWID, s_regs, 128, &nreg);
  planner.plan(cursx + SHANSHUI_CWID, cursx + 2 * SHANSHUI_CWID, s_regs, 128,
               &nreg);
  planner.plan(cursx - SHANSHUI_CWID, cursx, s_regs, 128, &nreg);

  // 展开为可排序绘制项（mount 附带 water）。
  static Item s_items[192];
  int n = 0;
  float viewL = cursx - 64, viewR = cursx + SHANSHUI_W + 64;
  for (int i = 0; i < nreg && n < 190; i++) {
    const Reg& r = s_regs[i];
    if (r.x < viewL - 400 || r.x > viewR + 400) continue; // 山体半宽裁剪
    if (r.tag == REG_MOUNT) {
      if (n + 2 > 192) break;
      s_items[n++] = {r.y - 10000, K_WATER, r.x, r.y, (float)i};
      s_items[n++] = {r.y, K_MOUNT, r.x, r.y, (float)i};
    } else {
      Kind k = r.tag == REG_FLAT ? K_FLAT : r.tag == REG_DIST ? K_DIST : K_BOAT;
      if (k != K_DIST && (r.x < viewL || r.x > viewR)) continue;
      if (k == K_DIST && (r.x < viewL - 200 || r.x > viewR + 200)) continue;
      s_items[n++] = {r.y, k, r.x, r.y, (float)i};
    }
  }
  sortItems(s_items, n);

  for (int i = 0; i < n; i++) {
    const Item& it = s_items[i];
    switch (it.kind) {
      case K_MOUNT:
        mount.mountain(it.x, it.y, it.p0 * 2 + rng.next(), true);
        break;
      case K_WATER:
        water.water(it.x, it.y, it.p0 * 2);
        break;
      case K_FLAT:
        mount.flatMount(it.x, it.y, 2 * rng.next() * 3.14159265f,
                        225 + rng.next() * 150, 38, 0.5f + rng.next() * 0.2f);
        break;
      case K_DIST:
        mount.distMount(it.x, it.y, rng.next() * 100);
        break;
      case K_BOAT:
        arch.boat01(it.x, it.y, rng.next(), 120, it.y / SHANSHUI_H,
                    rng.next() < 0.5f);
        break;
    }
  }
}

void renderGrayStr(const char* seed, uint8_t* gray, float cursx) {
  renderGrayStrBand(seed, gray, 0, SHANSHUI_H, cursx);
}

void renderGrayBand(uint32_t seed, uint8_t* grayBand, int y0, int h,
                    float cursx) {
  // uint32 -> 十进制字符串播种（与 seedStr 同分布族，桌面/单片机一致）。
  char buf[12];
  int l = 0;
  uint32_t v = seed;
  char tmp[12];
  int tl = 0;
  if (v == 0) tmp[tl++] = '0';
  while (v > 0 && tl < 11) {
    tmp[tl++] = (char)('0' + v % 10);
    v /= 10;
  }
  while (tl > 0) buf[l++] = tmp[--tl];
  buf[l] = 0;
  renderGrayStrBand(buf, grayBand, y0, h, cursx);
}

void renderGray(uint32_t seed, uint8_t* gray, float cursx) {
  renderGrayBand(seed, gray, 0, SHANSHUI_H, cursx);
}

void generate(uint32_t seed, uint8_t* gray, uint8_t* bits, DitherAlgo algo,
              float cursx) {
  if (!gray || !bits) return;
  renderGray(seed, gray, cursx);
  dither(gray, bits, algo);
}

void generateStr(const char* seed, uint8_t* gray, uint8_t* bits,
                 DitherAlgo algo, float cursx) {
  if (!gray || !bits) return;
  renderGrayStr(seed, gray, cursx);
  dither(gray, bits, algo);
}

// 分带一步到位：grayBand 只需 W*bandH（调用方静态提供，如 400x50=20KB）。
// 场景按带重放（时间约 ×带数），dither 误差跨带延续，结果与整幅逐位一致。
void generateStrBanded(const char* seed, uint8_t* grayBand, int bandH,
                       uint8_t* bits, DitherAlgo algo, float cursx) {
  if (!grayBand || !bits || bandH <= 0) return;
  ditherBegin(); // 扩散误差复位一次，带间保持。
  for (int y0 = 0; y0 < SHANSHUI_H; y0 += bandH) {
    int h = SHANSHUI_H - y0;
    if (h > bandH) h = bandH;
    renderGrayStrBand(seed, grayBand, y0, h, cursx);
    ditherBand(grayBand, bits, y0, h, algo);
  }
}

void generateBanded(uint32_t seed, uint8_t* grayBand, int bandH,
                    uint8_t* bits, DitherAlgo algo, float cursx) {
  char buf[12];
  int l = 0;
  uint32_t v = seed;
  char tmp[12];
  int tl = 0;
  if (v == 0) tmp[tl++] = '0';
  while (v > 0 && tl < 11) {
    tmp[tl++] = (char)('0' + v % 10);
    v /= 10;
  }
  while (tl > 0) buf[l++] = tmp[--tl];
  buf[l] = 0;
  generateStrBanded(buf, grayBand, bandH, bits, algo, cursx);
}

} // namespace shanshui
