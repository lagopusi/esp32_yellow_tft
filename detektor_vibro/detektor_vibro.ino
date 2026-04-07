#include <lvgl.h>
#include <TFT_eSPI.h>

// Пин датчика вибрации на разъеме CN1
#define VIB_PIN 27 

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
uint32_t draw_buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8) / 4];

// Объекты мордашки
lv_obj_t * left_eye;
lv_obj_t * right_eye;
lv_obj_t * left_pupil;
lv_obj_t * right_pupil;
lv_obj_t * mouth;
lv_obj_t * info_label;

// Состояние
bool is_dizzy = false;
uint32_t last_vibration_time = 0;

void apply_face_style(bool dizzy) {
    if (dizzy) {
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFFEB3B), 0); // Желтый испуганный фон
        lv_label_set_text(info_label, "STOP SHAKING ME!");
    } else {
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x81C784), 0); // Зеленый спокойный фон
        lv_label_set_text(info_label, "I feel good...");
    }
}

// Таймер для анимации и опроса датчика
void face_timer_cb(lv_timer_t * timer) {
    // Датчик SW-18010P обычно выдает LOW при срабатывании (зависит от настройки модуля)
    // Если у тебя он выдает HIGH, просто убери '!'
    bool vibration = (digitalRead(VIB_PIN) == LOW);

    if (vibration) {
        last_vibration_time = millis();
        if (!is_dizzy) {
            is_dizzy = true;
            apply_face_style(true);
        }
    }

    // Если тряски нет больше 2 секунд — успокаиваемся
    if (is_dizzy && (millis() - last_vibration_time > 2000)) {
        is_dizzy = false;
        apply_face_style(false);
    }

    // АНИМАЦИЯ ГЛАЗ
    if (is_dizzy) {
        // Вращаем зрачки по кругу при головокружении
        int angle = millis() / 2; 
        int r = 10; // радиус вращения
        int ox = (int)(cos(angle * 0.1) * r);
        int oy = (int)(sin(angle * 0.1) * r);
        
        lv_obj_align(left_pupil, LV_ALIGN_CENTER, ox, oy);
        lv_obj_align(right_pupil, LV_ALIGN_CENTER, ox, oy);
        
        // Рот превращается в "O"
        lv_obj_set_size(mouth, 40, 40);
        lv_obj_set_style_radius(mouth, LV_RADIUS_CIRCLE, 0);
    } else {
        // Плавно водим глазами влево-вправо в покое
        int x_offset = (int)(sin(millis() * 0.002) * 15);
        lv_obj_align(left_pupil, LV_ALIGN_CENTER, x_offset, 0);
        lv_obj_align(right_pupil, LV_ALIGN_CENTER, x_offset, 0);
        
        // Рот — улыбка
        lv_obj_set_size(mouth, 80, 20);
        lv_obj_set_style_radius(mouth, 10, 0);
    }
}

void lv_create_face_gui(void) {
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    // Создаем глаза (белки)
    left_eye = lv_obj_create(lv_screen_active());
    lv_obj_set_size(left_eye, 70, 90);
    lv_obj_align(left_eye, LV_ALIGN_CENTER, -50, -40);
    lv_obj_set_style_radius(left_eye, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(left_eye, lv_color_hex(0xFFFFFF), 0);

    right_eye = lv_obj_create(lv_screen_active());
    lv_obj_set_size(right_eye, 70, 90);
    lv_obj_align(right_eye, LV_ALIGN_CENTER, 50, -40);
    lv_obj_set_style_radius(right_eye, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(right_eye, lv_color_hex(0xFFFFFF), 0);

    // Создаем зрачки
    left_pupil = lv_obj_create(left_eye);
    lv_obj_set_size(left_pupil, 30, 30);
    lv_obj_set_style_radius(left_pupil, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(left_pupil, lv_color_hex(0x000000), 0);
    lv_obj_align(left_pupil, LV_ALIGN_CENTER, 0, 0);

    right_pupil = lv_obj_create(right_eye);
    lv_obj_set_size(right_pupil, 30, 30);
    lv_obj_set_style_radius(right_pupil, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(right_pupil, lv_color_hex(0x000000), 0);
    lv_obj_align(right_pupil, LV_ALIGN_CENTER, 0, 0);

    // Создаем рот
    mouth = lv_obj_create(lv_screen_active());
    lv_obj_set_size(mouth, 80, 20);
    lv_obj_align(mouth, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(mouth, lv_color_hex(0x000000), 0);

    // Текст подсказки
    info_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_18, 0);
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    apply_face_style(false);
}

void setup() {
    Serial.begin(115200);
    
    // Настройка пина датчика вибрации
    pinMode(VIB_PIN, INPUT_PULLUP);

    lv_init();

    // Инициализация дисплея
    static lv_display_t * disp;
    disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    
    lv_create_face_gui();

    // Быстрый таймер для плавной анимации и отлова тряски (50 кадров в секунду)
    lv_timer_create(face_timer_cb, 20, NULL);
}

void loop() {
    lv_task_handler();
    lv_tick_inc(5);
    delay(5);
}