// shanshui/arch.h — 建筑/船/塔。
#pragma once

#include "man.h"

namespace shanshui {

class Arch {
 public:
  Arch(Prng& rng, Noise& noise, Brush& brush, Raster& raster, Man& man)
      : rng_(rng), noise_(noise), brush_(brush), ras_(raster), man_(man) {}

  void hut(float xoff, float yoff, float hei, float wid, int tex);
  // decoStyle: 0=无装饰，1=-|||-, 2=||||, 3=|##|。
  void box(float xoff, float yoff, float hei, float wid, float rot, float per,
           bool tra, bool bot, float wei, int decoStyle);
  void rail(float xoff, float yoff, float seed, float hei, float wid,
            float rot, float per, int seg, float wei, bool tra, bool fro);
  void roof(float xoff, float yoff, float hei, float wid, float rot, float per,
            float cor, float wei);
  void pagroof(float xoff, float yoff, float hei, float wid, float per,
               float cor, int sid, float wei);

  void arch01(float xoff, float yoff, float seed, float hei, float wid,
              float rot, float per);
  void arch02(float xoff, float yoff, float seed, float hei, float wid,
              float rot, float per, int sto, int sty, bool rai);
  void arch03(float xoff, float yoff, float seed, float hei, float wid,
              float rot, float per, int sto);
  void arch04(float xoff, float yoff, float seed, float hei, float wid,
              float rot, float per, int sto);
  void boat01(float xoff, float yoff, float seed, float len, float sca,
              bool fli);
  void tower01(float xoff, float yoff, float hei, float wid);

 private:
  Prng& rng_;
  Noise& noise_;
  Brush& brush_;
  Raster& ras_;
  Man& man_;
  // 写入 lines（每条 ≤8 点），返回条数。
  int deco(int style, const Pt& pul, const Pt& pur, const Pt& pdl,
           const Pt& pdr, Pt lines[][8]);
  void quickstroke(const Pt* pl, int n, float xoff, float yoff);
};

} // namespace shanshui
