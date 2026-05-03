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
static lv_obj_t* status_lbl = NULL;
static lv_obj_t* active_screen_obj = NULL;

static String selected_ssid = "";
static String pairing_code = "";

static void back_event_cb(lv_event_t * e) {
    if (scan_timer) { lv_timer_del(scan_timer); scan_timer = NULL; }
    if (pair_timer) { lv_timer_del(pair_timer); pair_timer = NULL; }
    UIManager::getInstance().moveTo(Screen::HOME);
}

static void update_network_list() {
    if (!network_list || !lv_obj_is_valid(network_list)) return;
    lv_obj_clean(network_list);
    AppState& state = AppState::getInstance();
    
    if (state.scannedNetworks.empty()) {
        lv_list_add_text(network_list, "No networks found.");
        return;
    }

    for (size_t i = 0; i < state.scannedNetworks.size(); ++i) {
        const auto& net = state.scannedNetworks[i];
        String label = String(net.isOpen ? LV_SYMBOL_WIFI : LV_SYMBOL_SETTINGS) + " " + net.ssid;
        lv_obj_t * btn = lv_list_add_btn(network_list, NULL, label.c_str());
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(btn, lv_color_hex(0x0F172A), 0);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, 0);
        
        lv_obj_add_event_cb(btn, [](lv_event_t* ev){
            int idx = lv_obj_get_index((lv_obj_t*)lv_event_get_target(ev));
            // Subtract text labels if any
            if (idx >= 0 && idx < (int)AppState::getInstance().scannedNetworks.size()) {
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
        if (network_list && lv_obj_is_valid(network_list)) {
            update_network_list();
        }
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
    // Pairing success — token received
    if (state.deviceToken.length() > 0) {
        lv_label_set_text(pair_lbl, "Paired!");
        lv_obj_set_style_bg_color(pair_btn, lv_color_hex(0x10B981), 0);
        lv_timer_del(pair_timer);
        pair_timer = NULL;
        pairing_code = ""; // clear the pending code
        return;
    }
    // If WS just connected and we still have a pending code, resend pair_init
    if (pairing_code.length() > 0 && APIClient::getInstance().isConnected) {
        Serial.println("Pair: WS connected, sending pair_init...");
        APIClient::getInstance().pairDevice(pairing_code);
    }
}

static void pair_btn_cb(lv_event_t* e) {
    AppState& state = AppState::getInstance();
    
    // Revoke if already paired
    if (state.deviceToken.length() > 0) {
        state.deviceToken = "";
        state.save();
        pairing_code = "";
        lv_label_set_text(pair_lbl, "Start Pairing");
        lv_obj_set_style_bg_color(pair_btn, lv_color_hex(0x3B82F6), 0);
        APIClient::getInstance().forceReconnect();
        return;
    }

    // Generate and display the pairing code immediately — no WS needed for this step.
    // pair_timer will send pair_init as soon as WS connects.
    if (pairing_code.length() == 0) {
        pairing_code = String(random(100000, 999999));
    }
    String display = "Code: " + pairing_code;
    lv_label_set_text(pair_lbl, display.c_str());
    lv_obj_set_style_bg_color(pair_btn, lv_color_hex(0x3B82F6), 0);

    // If already connected, send pair_init right away
    if (APIClient::getInstance().isConnected) {
        APIClient::getInstance().pairDevice(pairing_code);
    } else {
        // Kick the WS to reconnect (it may have stopped after repeated failures)
        APIClient::getInstance().forceReconnect();
        Serial.println("Pair: code generated, reconnecting WS to send pair_init...");
    }

    if (!pair_timer) {
        pair_timer = lv_timer_create(pair_timer_cb, 1000, NULL);
    }
}

void refresh_screen_wifi() {
    if (!active_screen_obj || !lv_obj_is_valid(active_screen_obj)) return;
    
    AppState& state = AppState::getInstance();
    if (status_lbl) lv_label_set_text(status_lbl, ("WiFi: " + state.currentSSID).c_str());
    
    if (pair_btn && pair_lbl) {
        bool paired = state.deviceToken.length() > 0;
        if (paired) {
            lv_label_set_text(pair_lbl, "Paired");
            lv_obj_set_style_bg_color(pair_btn, lv_color_hex(0x10B981), 0);
        } else if (pairing_code.length() > 0) {
            // Code already generated — show it (don't overwrite with "Connecting")
            String display = "Code: " + pairing_code;
            lv_label_set_text(pair_lbl, display.c_str());
            lv_obj_set_style_bg_color(pair_btn, lv_color_hex(0x3B82F6), 0);
        } else {
            // No code yet — simple ready state
            lv_label_set_text(pair_lbl, "Start Pairing");
            lv_obj_set_style_bg_color(pair_btn, lv_color_hex(0x3B82F6), 0);
        }
    }
}

lv_obj_t* create_screen_wifi() {
    AppState& state = AppState::getInstance();

    lv_obj_t* scr = lv_obj_create(NULL);
    active_screen_obj = scr;
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Header
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 480, 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 35);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    lv_obj_set_style_shadow_width(back_btn, 0, 0);
    lv_obj_add_event_cb(back_btn, back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0x0F172A), 0);
    lv_obj_center(back_lbl);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi & Identity");
    lv_obj_set_style_text_color(title, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // WiFi List Section
    network_list = lv_list_create(scr);
    lv_obj_set_size(network_list, 260, 240);
    lv_obj_align(network_list, LV_ALIGN_TOP_LEFT, 10, 60);
    lv_obj_set_style_bg_color(network_list, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(network_list, 1, 0);
    lv_obj_set_style_border_color(network_list, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_radius(network_list, 8, 0);
    
    // Scan Button
    lv_obj_t* scan_btn = lv_btn_create(scr);
    lv_obj_set_size(scan_btn, 80, 35);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_RIGHT, -120, 60);
    lv_obj_set_style_bg_color(scan_btn, lv_color_hex(0x3B82F6), 0);
    lv_obj_set_style_radius(scan_btn, 8, 0);
    lv_obj_add_event_cb(scan_btn, scan_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* scan_lbl = lv_label_create(scan_btn);
    lv_label_set_text(scan_lbl, "Scan");
    lv_obj_set_style_text_font(scan_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(scan_lbl);
    
    // Status
    status_lbl = lv_label_create(scr);
    lv_label_set_text(status_lbl, ("WiFi: " + state.currentSSID).c_str());
    lv_obj_set_style_text_color(status_lbl, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_text_font(status_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(status_lbl, LV_ALIGN_TOP_LEFT, 280, 100);
    
    // Device ID
    lv_obj_t* dev_lbl = lv_label_create(scr);
    lv_label_set_text(dev_lbl, ("ID: " + state.deviceId).c_str());
    lv_obj_set_style_text_color(dev_lbl, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(dev_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(dev_lbl, LV_ALIGN_TOP_LEFT, 280, 130);

    // Pairing Section
    pair_btn = lv_btn_create(scr);
    lv_obj_set_size(pair_btn, 180, 40);
    lv_obj_align(pair_btn, LV_ALIGN_TOP_LEFT, 280, 160);
    lv_obj_set_style_bg_color(pair_btn, state.deviceToken.length() > 0 ? lv_color_hex(0x10B981) : lv_color_hex(0x3B82F6), 0);
    lv_obj_set_style_radius(pair_btn, 8, 0);
    lv_obj_add_event_cb(pair_btn, pair_btn_cb, LV_EVENT_CLICKED, NULL);
    pair_lbl = lv_label_create(pair_btn);
    lv_label_set_text(pair_lbl, state.deviceToken.length() > 0 ? "Paired" : "Start Pairing");
    lv_obj_set_style_text_font(pair_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(pair_lbl);

    // Password Text Area (Floating/Hidden)
    pass_ta = lv_textarea_create(scr);
    lv_obj_set_size(pass_ta, 300, 40);
    lv_obj_align(pass_ta, LV_ALIGN_CENTER, 0, -20);
    lv_textarea_set_password_mode(pass_ta, true);
    lv_textarea_set_placeholder_text(pass_ta, "Password");
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
        if(lv_event_get_code(e) == LV_EVENT_CANCEL) {
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(pass_ta, LV_OBJ_FLAG_HIDDEN);
        }
    }, LV_EVENT_ALL, NULL);

    update_network_list();

    return scr;
}
