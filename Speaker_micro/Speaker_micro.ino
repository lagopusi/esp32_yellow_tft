#include <lvgl.h>
#include <TFT_eSPI.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
uint32_t draw_buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8) / 4];

lv_obj_t * left_pupil;
lv_obj_t * right_pupil;
lv_obj_t * mouth;
lv_obj_t * info_label;

uint32_t last_msg_time = 0; // Время последнего сообщения от модуля

void face_timer_cb(lv_timer_t * timer) {
    // 1. ПРОВЕРКА UART (Самое главное)
    if (Serial.available() > 0) {
        // Если прилетели любые данные — запоминаем время
        while(Serial.available()) Serial.read(); // Чистим буфер
        last_msg_time = millis(); 
    }

    // 2. ЛОГИКА АНИМАЦИИ
    // Если данные приходили меньше 1 секунды назад — робот "говорит"
    if (millis() - last_msg_time < 1000) {
        int mouth_h = 5 + (rand() % 40);
        lv_obj_set_size(mouth, 100, mouth_h);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFF5722), 0); // Оранжевый фон
        lv_label_set_text(info_label, "I HEAR YOU!");
    } else {
        // Состояние покоя
        lv_obj_set_size(mouth, 80, 10);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x81C784), 0); // Зеленый фон
        lv_label_set_text(info_label, "Waiting for module...");
        
        // Зрачки плавно гуляют
        int x_offset = (int)(sin(millis() * 0.002) * 15);
        lv_obj_align(left_pupil, LV_ALIGN_CENTER, x_offset, 0);
        lv_obj_align(right_pupil, LV_ALIGN_CENTER, x_offset, 0);
    }
}

void setup() {
    // Пины 1 и 3
    Serial.begin(115200); 
    
    lv_init();

    static lv_display_t * disp;
    disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

    // Рисуем мордашку
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    lv_obj_t * eye_l = lv_obj_create(lv_screen_active());
    lv_obj_set_size(eye_l, 70, 90);
    lv_obj_align(eye_l, LV_ALIGN_CENTER, -50, -40);
    lv_obj_set_style_radius(eye_l, LV_RADIUS_CIRCLE, 0);

    lv_obj_t * eye_r = lv_obj_create(lv_screen_active());
    lv_obj_set_size(eye_r, 70, 90);
    lv_obj_align(eye_r, LV_ALIGN_CENTER, 50, -40);
    lv_obj_set_style_radius(eye_r, LV_RADIUS_CIRCLE, 0);

    left_pupil = lv_obj_create(eye_l);
    lv_obj_set_size(left_pupil, 30, 30);
    lv_obj_set_style_radius(left_pupil, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(left_pupil, lv_color_hex(0x000000), 0);
    lv_obj_align(left_pupil, LV_ALIGN_CENTER, 0, 0);

    right_pupil = lv_obj_create(eye_r);
    lv_obj_set_size(right_pupil, 30, 30);
    lv_obj_set_style_radius(right_pupil, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(right_pupil, lv_color_hex(0x000000), 0);
    lv_obj_align(right_pupil, LV_ALIGN_CENTER, 0, 0);

    mouth = lv_obj_create(lv_screen_active());
    lv_obj_set_size(mouth, 80, 10);
    lv_obj_align(mouth, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(mouth, lv_color_hex(0x000000), 0);

    info_label = lv_label_create(lv_screen_active());
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_timer_create(face_timer_cb, 30, NULL);
}

void loop() {
    lv_task_handler();
    lv_tick_inc(5);
    delay(5);
}