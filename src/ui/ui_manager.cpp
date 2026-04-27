#define LV_CONF_SKIP
#include "ui_manager.h"
#include "../state/app_state.h"
#include "screens/screen_boot.h"
#include "screens/screen_home.h"
#include "screens/screen_focus.h"
#include "screens/screen_wifi.h"

void UIManager::moveTo(Screen screen) {
    Serial.printf("[UI] moveTo screen %d...\n", (int)screen);
    lv_obj_t* new_screen = nullptr;

    switch (screen) {
        case Screen::BOOT:
            new_screen = create_screen_boot();
            break;
        case Screen::HOME:
            new_screen = create_screen_home();
            break;
        case Screen::FOCUS:
            new_screen = create_screen_focus();
            break;
        case Screen::WIFI:
            new_screen = create_screen_wifi();
            break;
    }

    if (new_screen) {
        lv_screen_load_anim(new_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);
        currentScreen = screen;
    }
}
