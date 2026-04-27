#define LV_CONF_SKIP
#include "screen_wifi.h"
#include "../../state/app_state.h"
#include "../ui_manager.h"
#include "../../wifi/wifi_manager.h"
#include "../../api/api_client.h"

static lv_obj_t* kb;
static lv_obj_t* pass_ta;
static lv_obj_t* network_list;
static lv_timer_t* scan_timer = NULL;
static lv_timer_t* pair_timer = NULL;
static lv_obj_t* pair_lbl;
static lv_obj_t* pair_btn;

static String selected_ssid = "";
static String pairing_code = "";

static void back_event_cb(lv_event_t * e) {
    if (scan_timer) { lv_timer_del(scan_timer); scan_timer = NULL; }
    if (pair_timer) { lv_timer_del(pair_timer); pair_timer = NULL; }
    UIManager::getInstance().moveTo(Screen::HOME);
}

static void update_network_list() {
    lv_obj_clean(network_list);
    AppState& state = AppState::getInstance();
    
    if (state.scannedNetworks.empty()) {
        lv_list_add_text(network_list, "No networks found.");
        return;
    }

    for (size_t i = 0; i < state.scannedNetworks.size(); ++i) {
        const auto& net = state.scannedNetworks[i];
        String label = String(net.isOpen ? "🔓" : "🔒") + " " + net.ssid + " (" + String(net.rssi) + "dBm)";
        lv_obj_t * btn = lv_list_add_btn(network_list, NULL, label.c_str());
        
        lv_obj_add_event_cb(btn, [](lv_event_t* ev){
            int idx = lv_obj_get_index((lv_obj_t*)lv_event_get_target(ev));
            if (idx >= 0 && idx < AppState::getInstance().scannedNetworks.size()) {
                ScannedNetwork& sel = AppState::getInstance().scannedNetworks[idx];
                selected_ssid = sel.ssid;
                
                if (sel.isOpen) {
                    WiFiManagerWrapper::getInstance().connectTo(selected_ssid, "");
                } else {
                    lv_obj_remove_flag(pass_ta, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
                    lv_keyboard_set_textarea(kb, pass_ta);
                }
            }
        }, LV_EVENT_CLICKED, NULL);
    }
}

static void scan_timer_cb(lv_timer_t * timer) {
    if (!WiFiManagerWrapper::getInstance().isScanning()) {
        lv_timer_del(scan_timer);
        scan_timer = NULL;
        update_network_list();
    }
}

static void scan_btn_cb(lv_event_t * e) {
    lv_obj_clean(network_list);
    lv_list_add_text(network_list, "Scanning...");
    
    WiFiManagerWrapper::getInstance().scanNetworksAsync();
    
    if (!scan_timer) {
        scan_timer = lv_timer_create(scan_timer_cb, 500, NULL);
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
    }
}

static void pair_timer_cb(lv_timer_t* t) {
    AppState& state = AppState::getInstance();
    if (state.deviceToken.length() > 0) {
        lv_label_set_text(pair_lbl, "Paired!");
        lv_obj_set_style_bg_color(pair_btn, lv_color_hex(0x10B981), 0);
        lv_timer_del(pair_timer);
        pair_timer = NULL;
    }
}

static void pair_btn_cb(lv_event_t* e) {
    AppState& state = AppState::getInstance();
    if (state.deviceToken.length() > 0) {
        // Already paired, allow reset
        state.deviceToken = "";
        state.save();
        lv_label_set_text(pair_lbl, "Start Pairing");
        lv_obj_set_style_bg_color(pair_btn, lv_color_hex(0x3B82F6), 0);
        return;
    }

    // Generate random 6-digit code
    pairing_code = String(random(100000, 999999));
    lv_label_set_text(pair_lbl, ("Code: " + pairing_code).c_str());
    
    APIClient::getInstance().pairDevice(pairing_code);
    
    if (!pair_timer) {
        pair_timer = lv_timer_create(pair_timer_cb, 1000, NULL);
    }
}

lv_obj_t* create_screen_wifi() {
    AppState& state = AppState::getInstance();

    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Header
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 480, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 80, 30);
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

    // WiFi List Section
    network_list = lv_list_create(scr);
    lv_obj_set_size(network_list, 260, 260);
    lv_obj_align(network_list, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_set_style_bg_color(network_list, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(network_list, 0, 0);
    
    // Scan Button
    lv_obj_t* scan_btn = lv_btn_create(scr);
    lv_obj_set_size(scan_btn, 80, 35);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_RIGHT, -10, 50);
    lv_obj_set_style_bg_color(scan_btn, lv_color_hex(0x3B82F6), 0);
    lv_obj_add_event_cb(scan_btn, scan_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* scan_lbl = lv_label_create(scan_btn);
    lv_label_set_text(scan_lbl, "Scan");
    lv_obj_center(scan_lbl);
    
    // Status
    lv_obj_t* status_lbl = lv_label_create(scr);
    lv_label_set_text(status_lbl, ("Status: " + state.currentSSID).c_str());
    lv_obj_set_style_text_color(status_lbl, lv_color_white(), 0);
    lv_obj_align(status_lbl, LV_ALIGN_TOP_LEFT, 280, 100);
    
    // Device ID
    lv_obj_t* dev_lbl = lv_label_create(scr);
    lv_label_set_text(dev_lbl, ("ID: " + state.deviceId).c_str());
    lv_obj_set_style_text_color(dev_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(dev_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(dev_lbl, LV_ALIGN_TOP_LEFT, 280, 130);

    // Pairing Section
    pair_btn = lv_btn_create(scr);
    lv_obj_set_size(pair_btn, 190, 40);
    lv_obj_align(pair_btn, LV_ALIGN_TOP_LEFT, 280, 160);
    lv_obj_set_style_bg_color(pair_btn, state.deviceToken.length() > 0 ? lv_color_hex(0x10B981) : lv_color_hex(0x3B82F6), 0);
    lv_obj_add_event_cb(pair_btn, pair_btn_cb, LV_EVENT_CLICKED, NULL);
    pair_lbl = lv_label_create(pair_btn);
    lv_label_set_text(pair_lbl, state.deviceToken.length() > 0 ? "Paired (Tap to reset)" : "Start Pairing");
    lv_obj_center(pair_lbl);

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

    update_network_list();

    return scr;
}
