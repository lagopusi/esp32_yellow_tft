#include <lvgl.h>
#include <TFT_eSPI.h>

#define VIB_PIN 27 
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

uint32_t draw_buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8) / 4];

lv_obj_t * left_eye; lv_obj_t * right_eye;
lv_obj_t * left_pupil; lv_obj_t * right_pupil;
lv_obj_t * mouth; lv_obj_t * info_label;

uint32_t last_vibration_time = 0;
uint32_t talking_until = 0; 

void lv_create_face_gui(void) {
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    // ... (код создания глаз и рта остается прежним)
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

    mouth = lv_obj_create(lv_screen_active());
    lv_obj_set_size(mouth, 80, 20);
    lv_obj_align(mouth, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(mouth, lv_color_hex(0x000000), 0);

    info_label = lv_label_create(lv_screen_active());
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -15);
}

void face_timer_cb(lv_timer_t * timer) {
    uint32_t now = millis();

    // 1. ДАТЧИК ВИБРАЦИИ
    if (digitalRead(VIB_PIN) == LOW) {
        last_vibration_time = now;
    }

    // 2. УМНЫЙ SERIAL (Поиск символа Q)
    if (Serial.available() > 0) {
        bool foundQ = false;
        
        // Проверяем все байты в буфере
        while (Serial.available() > 0) {
            char c = Serial.read();
            if (c == 'Q') {
                foundQ = true;
            }
        }

        if (foundQ) {
            // Если нашли 'Q' — анимация на 3 секунды
            talking_until = now + 4000; 
        } else {
            // Если пришли другие данные — анимация на 800 мс (короткая фраза)
            // Но только если сейчас уже не идет длинная анимация
            if (now > talking_until) {
                talking_until = now + 1200; 
            }
        }
    }

    // 3. ЛОГИКА ОТОБРАЖЕНИЯ
    if (now - last_vibration_time < 2000) {
        // ТРЯСКА
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFFEB3B), 0);
        lv_label_set_text(info_label, "Dizzy...");
        int angle = now / 2;
        lv_obj_align(left_pupil, LV_ALIGN_CENTER, (int)(cos(angle * 0.1) * 10), (int)(sin(angle * 0.1) * 10));
        lv_obj_align(right_pupil, LV_ALIGN_CENTER, (int)(cos(angle * 0.1) * 10), (int)(sin(angle * 0.1) * 10));
        lv_obj_set_size(mouth, 40, 40);
        lv_obj_set_style_radius(mouth, LV_RADIUS_CIRCLE, 0);

    } else if (now < talking_until) {
        // ГОВОРЕНИЕ
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFF5722), 0);
        lv_label_set_text(info_label, "Talking...");
        lv_obj_align(left_pupil, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(right_pupil, LV_ALIGN_CENTER, 0, 0);
        
        // Рот прыгает случайно
        int mouth_h = 10 + (rand() % 40);
        lv_obj_set_size(mouth, 100, mouth_h);
        lv_obj_set_style_radius(mouth, 10, 0);

    } else {
        // ПОКОЙ
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x81C784), 0);
        lv_label_set_text(info_label, "Idle");
        int x_offset = (int)(sin(now * 0.002) * 15);
        lv_obj_align(left_pupil, LV_ALIGN_CENTER, x_offset, 0);
        lv_obj_align(right_pupil, LV_ALIGN_CENTER, x_offset, 0);
        lv_obj_set_size(mouth, 80, 10);
        lv_obj_set_style_radius(mouth, 10, 0);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(VIB_PIN, INPUT_PULLUP);
    lv_init();
    static lv_display_t * disp;
    disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    lv_create_face_gui();
    lv_timer_create(face_timer_cb, 30, NULL);
}

void loop() {
    lv_task_handler();
    lv_tick_inc(5);
    delay(5);
}