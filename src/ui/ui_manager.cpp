#include "ui_manager.h"
#include "../state/app_state.h"

void UIManager::moveTo(Screen screen) {
    lv_obj_t* new_screen = nullptr;

    switch (screen) {
        case Screen::BOOT:
            new_screen = createBootScreen();
            break;
        case Screen::HOME:
            new_screen = createHomeScreen();
            break;
        case Screen::FOCUS:
            new_screen = createFocusScreen();
            break;
        case Screen::WIFI:
            new_screen = createWifiScreen();
            break;
    }

    if (new_screen) {
        lv_screen_load_anim(new_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);
        currentScreen = screen;
    }
}

lv_obj_t* UIManager::createBootScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A1A), 0);
    
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "FocusOS Booting...");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    
    return scr;
}

lv_obj_t* UIManager::createHomeScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x003366), 0);
    
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "Dashboard (Home)");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    
    lv_obj_t* info = lv_label_create(scr);
    lv_label_set_text(info, "Part 1 Implemented");
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(info, lv_color_hex(0xCCCCCC), 0);
    
    return scr;
}

lv_obj_t* UIManager::createFocusScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "Focus Timer Stub");
    lv_obj_center(label);
    return scr;
}

lv_obj_t* UIManager::createWifiScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "WiFi Settings Stub");
    lv_obj_center(label);
    return scr;
}
