// shanshui/brush.h — 笔触/墨块/细分/皴法（web/js/brush/brush.js 的 MCU 版）。
// 约定：所有坐标均为世界坐标；col 用 Ink（灰度+alpha）代替 rgba 字符串；
// 宽度函数用普通函数指针（不用 std::function，避免堆分配）。
#pragma once
#include "noise.h"
#include "raster.h"

namespace shanshui {

typedef float (*WFun)(float x); // 笔宽包络，x in [0,1]。
typedef float (*BFun)(float x); // 墨块轮廓，x in [0,2]。

// ---- 笔宽包络 ----
float wfSin(float x);     // sin(x*PI)（默认）。
float wfCosHalf(float x); // cos(x*PI/2)（细梢）。
float wfOne(float x);     // 1（建筑线条）。
float wfSin1(float x);    // sin(1) 常数（树干）。
float wfSin3(float x);    // sin(x*3*PI)（树皮）。
float wfSleeve(float x);  // 人物衣袖。
float wfBody(float x);    // 人物身体。
float wfFalloff(float x); // -1/(x+1)^5+1（嫩枝）。
// ---- 墨块轮廓 ----
float bfDefault(float x);
float bfLeaf(float x); // 叶簇。
float bfPine(float x); // 松针.

// 二次贝塞尔光顺（web Tools.bezmh），返回点数。
int bezmh(const Pt* P, int n, float w, Pt* out, int maxOut);
// 首尾相接的噪声归一到 0..1（web Tools.loopNoise）。
void loopNoise(float* a, int n);
// 高斯抖动（web Tools.randGaussian），范围约 [-1,1]。
float randGaussian(Prng& rng);

enum DisMode : uint8_t { DIS_MID = 0, DIS_HUT, DIS_ROCK, DIS_FLAT };

struct TexArgs {
  int tex = 100;         // 笔数（原 mountain 默认 200，经 TEX_DIV 缩放）。
  float wid = 1.5f;      // 线宽。
  float len = 0.2f;      // 横向半长（占列数比例）。
  float sha = 0.0f;      // >0 时加淡影层。
  uint8_t gray = 100;    // 墨色。
  uint8_t a0 = 0, a1 = 77; // 每笔 alpha 随机区间（默认 ~0..0.3）。
  float noiK = -1.0f;    // >=0 用常数，否则用 30/x。
  DisMode dis = DIS_MID;
};

class Brush {
 public:
  Brush(Prng& rng, Noise& noise, Raster& raster)
      : rng_(rng), noise_(noise), ras_(raster) {}

  // 中线 -> 轮廓多边形填充 + 可选描边（out 为描边宽，0 则不描）。
  void stroke(const Pt* pts, int n, Ink col, float wid = 2.0f, float noi = 0.5f,
              float out = 1.0f, WFun fun = wfSin);
  // 椭圆墨块（直接绘制，无 ret 取点模式：tree07 已改写，不再需要）。
  void blob(float x, float y, float len, float wid, float ang, Ink col,
            float noi = 0.5f, BFun fun = bfDefault);
  // 折线细分：(n-1)*reso+1 点。
  int div(const Pt* in, int n, int reso, Pt* out, int maxOut);
  // 皴法：在 I×J 网格上随机取横向短笔，流式绘制（不存整表，省 RAM）。
  void texture(const Pt* grid, int I, int J, float xof, float yof,
               const TexArgs& a);

 private:
  Prng& rng_;
  Noise& noise_;
  Raster& ras_;
  float disPick();
  DisMode disMode_ = DIS_MID;
};

} // namespace shanshui
