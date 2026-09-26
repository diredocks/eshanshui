#include <GxEPD2_BW.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include "src/shanshui/shanshui.h" // src/ 下源码会被递归编译（lib/ 不会）。

#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4

GxEPD2_BW<GxEPD2_420_M01, GxEPD2_420_M01::HEIGHT>
  display(GxEPD2_420_M01(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// 有 PSRAM：整幅灰度从 PSRAM 分配，场景只渲染 1 次。
// 无 PSRAM：回退分带（慢约 ×带数，输出逐位一致）。
static uint8_t s_bits[SHANSHUI_BITS_SIZE];
static uint8_t s_grayBand[SHANSHUI_BAND_SIZE];
static uint32_t s_seed;

// render 调用链栈峰值约 10KB，loopTask 默认 8KB 不够，独立任务给 24KB 跑完即删。
static volatile bool s_genDone = false;
static bool s_fullFrame = false;

static void genTaskFn(void* arg) {
  (void)arg;
  uint8_t* gray =
      (uint8_t*)heap_caps_malloc(SHANSHUI_GRAY_SIZE, MALLOC_CAP_SPIRAM);
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

  s_seed = esp_random();
  if (s_seed == 0) s_seed = 0x9E3779B9u;

  uint32_t t0 = millis();
  xTaskCreate(genTaskFn, "shanshui_gen", 24576, NULL, 1, NULL);
  while (!s_genDone) delay(10); // 喂看门狗。
  Serial.printf("generate done: seed=%u algo=atkinson mode=%s %lums\n", s_seed,
                s_fullFrame ? "psram-full" : "banded",
                (unsigned long)(millis() - t0));

  display.init(115200);
  display.setRotation(0);
  Serial.println("display initialized");

  // 白底 + bit=0 处画黑。
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
