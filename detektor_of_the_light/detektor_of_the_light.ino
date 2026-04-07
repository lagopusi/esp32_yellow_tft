#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#define LDR_PIN 34
int ldr_threshold = 2000; // Начальное значение порога

// Touchscreen pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

uint32_t draw_buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8) / 4];

// Объекты интерфейса
lv_obj_t * ldr_val_label;
lv_obj_t * threshold_label;
lv_obj_t * status_label;
lv_obj_t * slider;
lv_display_t * disp;

bool is_currently_dark = false;

// Чтение тачскрина (нужно для работы слайдера)
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    // Калибровка под ориентацию
    int x_map = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    int y_map = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x_map;
    data->point.y = y_map;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// Применение темы
void apply_theme(bool dark) {
  if (dark) {
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x1A1A1A), 0); // Темно-серый
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(status_label, "Status: DARK MODE");
  } else {
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xF0F0F0), 0); // Светло-серый
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0x000000), 0);
    lv_label_set_text(status_label, "Status: LIGHT MODE");
  }
}

// Обработка движений слайдера
static void slider_event_cb(lv_event_t * e) {
  lv_obj_t * slider_obj = (lv_obj_t*)lv_event_get_target(e);
  ldr_threshold = (int)lv_slider_get_value(slider_obj);
  
  // Обновляем текст порога
  char buf[32];
  lv_snprintf(buf, sizeof(buf), "Threshold: %d", ldr_threshold);
  lv_label_set_text(threshold_label, buf);
}

// Таймер опроса датчика
void ldr_timer_cb(lv_timer_t * timer) {
  int raw_ldr = analogRead(LDR_PIN);
  
  // Обновляем текст текущего значения
  char buf[32];
  lv_snprintf(buf, sizeof(buf), "Current LDR: %d", raw_ldr);
  lv_label_set_text(ldr_val_label, buf);

  // Логика переключения
  bool dark_now = (raw_ldr > ldr_threshold);
  if (dark_now != is_currently_dark) {
    is_currently_dark = dark_now;
    apply_theme(is_currently_dark);
  }
}

void lv_create_main_gui(void) {
  lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

  // 1. Заголовок
  lv_obj_t * title = lv_label_create(lv_screen_active());
  lv_label_set_text(title, "LDR Educational Tool");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

  // 2. Текущее значение LDR
  ldr_val_label = lv_label_create(lv_screen_active());
  lv_label_set_text(ldr_val_label, "Current LDR: 0");
  lv_obj_align(ldr_val_label, LV_ALIGN_TOP_MID, 0, 50);

  // 3. Слайдер для настройки порога
  slider = lv_slider_create(lv_screen_active());
  lv_obj_set_size(slider, 200, 20);
  lv_slider_set_range(slider, 0, 4095);
  lv_slider_set_value(slider, ldr_threshold, LV_ANIM_OFF);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 10);
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // 4. Значение порога (под слайдером)
  threshold_label = lv_label_create(lv_screen_active());
  char buf[32];
  lv_snprintf(buf, sizeof(buf), "Threshold: %d", ldr_threshold);
  lv_label_set_text(threshold_label, buf);
  lv_obj_align_to(threshold_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  // 5. Статус (Light/Dark)
  status_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -30);

  // Инициализация темы
  apply_theme(false);
}

void setup() {
  Serial.begin(115200);
  pinMode(LDR_PIN, INPUT);

  lv_init();

  // Инициализация тача
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2);

  // Инициализация дисплея
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    
  // Регистрация тачскрина в LVGL
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  lv_create_main_gui();

  // Таймер опроса: 200мс для очень быстрой реакции
  lv_timer_create(ldr_timer_cb, 200, NULL);
}

void loop() {
  lv_task_handler();
  lv_tick_inc(5);
  delay(5);
}