// shanshui/mount.h — 山/石。
#pragma once

#include "arch.h"
#include "tree.h"

namespace shanshui {

class Mount {
 public:
  Mount(Prng& rng, Noise& noise, Brush& brush, Raster& raster, Tree& tree,
        Arch& arch)
      : rng_(rng),
        noise_(noise),
        brush_(brush),
        ras_(raster),
        tree_(tree),
        arch_(arch) {}

  // 主峰（hei/wid/tex 内部随机；veg 是否生植被/建筑）。
  void mountain(float xoff, float yoff, float seed, bool veg = true);
  // 坡岸平山。
  void flatMount(float xoff, float yoff, float seed, float wid, float hei,
                 float cho);
  // 远山（内部 hei=56，len 三选一）。
  void distMount(float xoff, float yoff, float seed);
  void rock(float xoff, float yoff, float seed, float wid, float hei, int tex,
            float sha);

 private:
  Prng& rng_;
  Noise& noise_;
  Brush& brush_;
  Raster& ras_;
  Tree& tree_;
  Arch& arch_;
  void foot(const Pt* grid, int I, int J, float xof, float yof);
  bool flatShoreline(float xoff, float yoff, const Pt (*fseg)[2], int fsg,
                     float& xmin, float& xmax, float& ymin, float& ymax);
  void flatDec(float xoff, float yoff, float xmin, float xmax, float ymin,
               float ymax);
  void ridgeTrees(const Pt* grid, float xoff, float yoff, float seed, float hei);
  void topTrees(const Pt* grid, float xoff, float yoff, float seed, float hei);
  void midTrees(const Pt* grid, float xoff, float yoff, float seed, float hei);
  void bottomTrees(const Pt* grid, float xoff, float yoff, float seed,
                   float hei);
  void bottomArch(const Pt* grid, float xoff, float yoff, float seed);
  void topArch(const Pt* grid, float xoff, float yoff, float seed);
  void transm(const Pt* grid, float xoff, float yoff, float seed);
  void bottomRocks(const Pt* grid, float xoff, float yoff);
};

} // namespace shanshui
