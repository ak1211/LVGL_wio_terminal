#include <lvgl.h>

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

/*Set to your screen resolution and rotation*/
#define TFT_HOR_RES 320
#define TFT_VER_RES 240
#define TFT_ROTATION LV_DISPLAY_ROTATION_0

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

/* 外部ファイルにあるマウスカーソルの画像 */
extern const lv_image_dsc_t mouse_cursor_icon;

/* マウスカーソルの位置 */
static lv_point_t pointer_loc{ .x = TFT_HOR_RES / 2, .y = TFT_VER_RES / 2 };

/* LovyanGFXの実体 */
static LGFX lcd;

#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}
#endif

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  /*Copy `px map` to the `area`*/
  uint32_t width = (area->x2 - area->x1 + 1);
  uint32_t height = (area->y2 - area->y1 + 1);
  if (!lcd.getSwapBytes()) {
    lv_draw_sw_rgb565_swap(px_map, width * height);  // LGFXでバイト順変換をする場合は不要になる
  }
  lcd.pushImageDMA<uint16_t>(area->x1, area->y1, width, height, (uint16_t *)px_map);
  /*Call it to tell LVGL you are ready*/
  lv_display_flush_ready(disp);
}

/* マウスカーソルの読み込み処理 */
void my_pointer_read(lv_indev_t *indev, lv_indev_data_t *data) {
  const auto step = 5;
  if (digitalRead(WIO_5S_UP) == LOW) {
    pointer_loc.y -= step;
  } else if (digitalRead(WIO_5S_DOWN) == LOW) {
    pointer_loc.y += step;
  }
  if (digitalRead(WIO_5S_LEFT) == LOW) {
    pointer_loc.x -= step;
  } else if (digitalRead(WIO_5S_RIGHT) == LOW) {
    pointer_loc.x += step;
  }
  if (pointer_loc.x <= 0) {
    pointer_loc.x = 0;
  } else if (pointer_loc.x >= TFT_HOR_RES) {
    pointer_loc.x = TFT_HOR_RES;
  }
  if (pointer_loc.y <= 0) {
    pointer_loc.y = 0;
  } else if (pointer_loc.y >= TFT_VER_RES) {
    pointer_loc.x = TFT_VER_RES;
  }
  data->point.x = pointer_loc.x;
  data->point.y = pointer_loc.y;

  if (digitalRead(WIO_5S_PRESS) == LOW) {
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void) {
  return millis();
}

void setup() {
  /* 5方向ボタンの初期化 */
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);

  lcd.init();              // 最初に初期化関数を呼び出します。
  lcd.setColorDepth(16);   // RGB565の16ビットに設定
  lcd.setSwapBytes(true);  // バイト順変換を有効にする。

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.begin(115200);
  for (auto t0 = millis(); (millis() - t0 < 2000) && !Serial;) {
    /* シリアル初期化完了待ち */
  }
  Serial.println(LVGL_Arduino);

  lv_init();

  /*Set a tick source so that LVGL will know how much time elapsed. */
  lv_tick_set_cb(my_tick);

  /* register print function for debugging */
#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print);
#endif

  lv_display_t *disp;
  disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *mouse_indev = lv_indev_create();
  lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(mouse_indev, my_pointer_read);
  LV_IMAGE_DECLARE(mouse_cursor_icon);                        /* Declare the image source. */
  lv_obj_t *cursor_obj = lv_image_create(lv_screen_active()); /* Create image Widget for cursor. */
  lv_image_set_src(cursor_obj, &mouse_cursor_icon);           /* Set image source. */
  lv_indev_set_cursor(mouse_indev, cursor_obj);               /* Connect image to Input Device. */

  /* Create a simple label
     * ---------------------
     lv_obj_t *label = lv_label_create( lv_screen_active() );
     lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
     lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

     * Try an example. See all the examples
     *  - Online: https://docs.lvgl.io/master/examples.html
     *  - Source codes: https://github.com/lvgl/lvgl/tree/master/examples
     * ----------------------------------------------------------------

     lv_example_btn_1();

     * Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMO_WIDGETS
     * -------------------------------------------------------------------------------------------

     lv_demo_widgets();
     */

  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello Arduino, I'm LVGL!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  Serial.println("Setup done");
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
}