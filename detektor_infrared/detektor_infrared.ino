#include <lvgl.h>
#include <TFT_eSPI.h>

// Пин датчика на CN1
#define IR_PIN 27 

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
uint32_t draw_buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8) / 4];

lv_obj_t * counter_label;
lv_obj_t * status_indicator;
lv_obj_t * info_text;

int visitor_count = 0;
bool last_ir_state = true; 

void ir_sensor_timer_cb(lv_timer_t * timer) {
  bool current_state = digitalRead(IR_PIN);

  if (current_state == LOW && last_ir_state == HIGH) {
    visitor_count++;
    char buf[32];
    lv_snprintf(buf, sizeof(buf), "COUNT: %d", visitor_count);
    lv_label_set_text(counter_label, buf);

    lv_obj_set_style_bg_color(status_indicator, lv_palette_main(LV_PALETTE_RED), 0);
    lv_label_set_text(info_text, "STATUS: OBJECT DETECTED");
    lv_obj_set_style_text_color(info_text, lv_palette_main(LV_PALETTE_RED), 0);
  } 
  else if (current_state == HIGH && last_ir_state == LOW) {
    lv_obj_set_style_bg_color(status_indicator, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_label_set_text(info_text, "STATUS: SCANNING...");
    lv_obj_set_style_text_color(info_text, lv_color_hex(0x888888), 0);
  }
  last_ir_state = current_state;
}

void lv_create_proximity_gui(void) {
  // Фон
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);

  // 1. Заголовок (сверху)
  lv_obj_t * title = lv_label_create(lv_screen_active());
  lv_label_set_text(title, "IR PROXIMITY SENSOR");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

  // 2. Индикатор детекции (Строго в центре)
  status_indicator = lv_obj_create(lv_screen_active());
  lv_obj_set_size(status_indicator, 120, 120); // Чуть увеличил для солидности
  lv_obj_set_style_radius(status_indicator, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(status_indicator, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_border_width(status_indicator, 4, 0);
  lv_obj_set_style_border_color(status_indicator, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(status_indicator, LV_ALIGN_CENTER, 0, 0); // ЦЕНТР

  // 3. Текст счетчика (под кругом)
  counter_label = lv_label_create(lv_screen_active());
  lv_label_set_text(counter_label, "COUNT: 0");
  lv_obj_set_style_text_font(counter_label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(counter_label, lv_color_hex(0xFFFFFF), 0);
  // Привязываем к низу круга с отступом 20
  lv_obj_align_to(counter_label, status_indicator, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

  // 4. Текст статуса (в самом низу, минимальный паддинг)
  info_text = lv_label_create(lv_screen_active());
  lv_label_set_text(info_text, "STATUS: SCANNING...");
  lv_obj_set_style_text_font(info_text, &lv_font_montserrat_12, 0);
  lv_obj_align(info_text, LV_ALIGN_BOTTOM_MID, 0, -5); // Паддинг всего 5 пикселей от края
}

void setup() {
  pinMode(IR_PIN, INPUT);

  lv_init();

  static lv_display_t * disp;
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    
  lv_create_proximity_gui();

  // Опрос датчика 20 раз в секунду
  lv_timer_create(ir_sensor_timer_cb, 50, NULL);
}

void loop() {
  lv_task_handler();
  lv_tick_inc(5);
  delay(5);
}