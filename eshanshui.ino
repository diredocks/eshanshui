#include <GxEPD2_BW.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include "src/shanshui/shanshui.h" // 编译只认 sketch 根+-I，src/ 下源码会被递归编译（lib/ 不会）。

#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4

GxEPD2_BW<GxEPD2_420_M01, GxEPD2_420_M01::HEIGHT>
  display(GxEPD2_420_M01(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// 二值 15KB 放静态区。
// 有 PSRAM：整幅灰度 120KB 从 PSRAM 分配，场景只渲染 1 次（最快）。
// 无 PSRAM：回退分带（grayBand 20KB 静态），慢约 ×带数，输出逐位一致。
static uint8_t s_bits[SHANSHUI_BITS_SIZE];
static uint8_t s_grayBand[SHANSHUI_BAND_SIZE]; // 仅无 PSRAM 回退用。

static uint32_t s_seed;

// 生成任务：render 调用链栈峰值约 10KB（cloth/man/planner 大数组在栈上），
// loopTask 默认 8KB 必爆（Stack canary）。独立任务给 24KB 栈跑完即删。
static volatile bool s_genDone = false;
static bool s_fullFrame = false;

static void genTaskFn(void* arg) {
  (void)arg;
  // 整幅灰度放 PSRAM：一次渲染，避免分带把整个场景重放数遍。
  uint8_t* gray = (uint8_t*)heap_caps_malloc(SHANSHUI_GRAY_SIZE, MALLOC_CAP_SPIRAM);
  if (gray) {
    shanshui::generate(s_seed, gray, s_bits, shanshui::DITHER_ATKINSON);
    heap_caps_free(gray);
    s_fullFrame = true;
  } else {
    shanshui::generateBanded(s_seed, s_grayBand, SHANSHUI_BAND_H, s_bits,
                             shanshui::DITHER_FLOYD);
  }
  s_genDone = true;
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);

  // 每次开机随机画面：ESP32 硬件 RNG，无需播种。
  s_seed = esp_random();
  if (s_seed == 0) s_seed = 0x9E3779B9u;

  // 开机即生成：renderGray + Atkinson 抖动（bit=1=白/MSB先行）。
  uint32_t t0 = millis();
  xTaskCreate(genTaskFn, "shanshui_gen", 24576, NULL, 1, NULL);
  while (!s_genDone) delay(10); // 喂看门狗，等生成完成。
  Serial.printf("generate done: seed=%u algo=atkinson mode=%s %lums\n", s_seed,
                s_fullFrame ? "psram-full" : "banded",
                (unsigned long)(millis() - t0));

  // 后 init 显示：GxEPD2 初始化会占堆/DMA，峰值错开。
  display.init(115200);
  display.setRotation(0);
  Serial.println("display initialized");

  // 全屏刷墨水屏：白底 + bit=0 处画黑（与 drawInvertedBitmap 语义对齐）。
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0, s_bits, SHANSHUI_W, SHANSHUI_H,
                               GxEPD_BLACK);
  } while (display.nextPage());
  Serial.println("epd refresh done");
  display.hibernate();
}

void loop() {}
