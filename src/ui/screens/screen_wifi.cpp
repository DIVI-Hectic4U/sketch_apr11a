#define LV_CONF_SKIP
#include "screen_wifi.h"
#include "../../state/app_state.h"
#include "../ui_manager.h"
#include "../../wifi/wifi_manager.h"

static lv_obj_t* kb;
static lv_obj_t* user_id_ta;
static lv_obj_t* pass_ta;
static lv_obj_t* network_list;
static String selected_ssid = "";

static void back_event_cb(lv_event_t * e) {
    UIManager::getInstance().moveTo(Screen::HOME);
}

static void scan_btn_cb(lv_event_t * e) {
    WiFiManagerWrapper::getInstance().scanNetworks();
    // Re-create the list items
    lv_obj_clean(network_list);
    AppState& state = AppState::getInstance();
    for (const auto& ssid : state.scannedSSIDs) {
        lv_obj_t * btn = lv_list_add_btn(network_list, NULL, ssid.c_str());
        lv_obj_add_event_cb(btn, [](lv_event_t* ev){
            selected_ssid = lv_list_get_btn_text(network_list, (lv_obj_t*)lv_event_get_target(ev));
            lv_obj_remove_flag(pass_ta, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(kb, pass_ta);
        }, LV_EVENT_CLICKED, NULL);
    }
}

static void connect_event_cb(lv_event_t * e) {
    const char* pass = lv_textarea_get_text(pass_ta);
    WiFiManagerWrapper::getInstance().connectTo(selected_ssid, String(pass));
    lv_obj_add_flag(pass_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

static void ta_event_cb(lv_event_t * e) {
    lv_obj_t * ta = (lv_obj_t*)lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        if (ta == user_id_ta) {
            AppState::getInstance().userId = lv_textarea_get_text(ta);
            AppState::getInstance().save();
        }
    }
}

lv_obj_t* create_screen_wifi() {
    AppState& state = AppState::getInstance();

    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Header
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 480, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 80, 35);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x334155), 0);
    lv_obj_add_event_cb(back_btn, back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "Back");
    lv_obj_center(back_lbl);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi & Identity");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // User ID Section
    lv_obj_t* user_id_lbl = lv_label_create(scr);
    lv_label_set_text(user_id_lbl, "User ID:");
    lv_obj_set_style_text_color(user_id_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(user_id_lbl, LV_ALIGN_TOP_LEFT, 20, 60);

    user_id_ta = lv_textarea_create(scr);
    lv_obj_set_size(user_id_ta, 150, 35);
    lv_obj_align(user_id_ta, LV_ALIGN_TOP_LEFT, 80, 55);
    lv_textarea_set_text(user_id_ta, state.userId.c_str());
    lv_textarea_set_one_line(user_id_ta, true);
    lv_obj_add_event_cb(user_id_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // WiFi Scan Section
    lv_obj_t* scan_btn = lv_btn_create(scr);
    lv_obj_set_size(scan_btn, 100, 35);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_RIGHT, -20, 55);
    lv_obj_set_style_bg_color(scan_btn, lv_color_hex(0x3B82F6), 0);
    lv_obj_add_event_cb(scan_btn, scan_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* scan_lbl = lv_label_create(scan_btn);
    lv_label_set_text(scan_lbl, "Scan");
    lv_obj_center(scan_lbl);

    network_list = lv_list_create(scr);
    lv_obj_set_size(network_list, 440, 150);
    lv_obj_align(network_list, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_bg_color(network_list, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(network_list, 0, 0);

    // Password Text Area (Floating/Hidden)
    pass_ta = lv_textarea_create(scr);
    lv_obj_set_size(pass_ta, 300, 40);
    lv_obj_align(pass_ta, LV_ALIGN_CENTER, 0, -20);
    lv_textarea_set_password_mode(pass_ta, true);
    lv_textarea_set_placeholder_text(pass_ta, "Enter Password");
    lv_obj_add_flag(pass_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(pass_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // Keyboard
    kb = lv_keyboard_create(scr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, [](lv_event_t* e){
        if(lv_event_get_code(e) == LV_EVENT_READY) {
            if (lv_keyboard_get_textarea(kb) == pass_ta) {
                connect_event_cb(e);
            }
        }
    }, LV_EVENT_ALL, NULL);

    return scr;
}
