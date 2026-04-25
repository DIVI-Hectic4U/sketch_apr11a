# ESP32-S3 Companion Device — Full Firmware Implementation Prompt
**Hardware:** WT32-SC01 Plus (ESP32-S3-WROVER-N16R2, 3.5" 480×320 IPS, FT6336U capacitive touch, 8-bit parallel display bus)  
**Framework:** Arduino (PlatformIO) + LVGL v8 + LovyanGFX  
**Backend:** Your existing Node.js/Express REST API  
**Role of device:** A dedicated physical focus companion that mirrors and extends the web dashboard — not a replacement.

Read the entire document before writing any code.

---

## Part 0 — Hardware Reference (WT32-SC01 Plus)

Confirm these pin definitions before writing any display/touch initialisation code. These are specific to the **Plus** variant (ESP32-S3), not the original WT32-SC01 (ESP32):

```
Display driver:   ST7701S (8-bit parallel RGB interface — NOT SPI)
Touch controller: FT6336U (I2C)
Display pins (8-bit parallel):
  TFT_D0–D7:  GPIO 39,38,45,48,47,21,14,13
  TFT_WR:     GPIO 12
  TFT_RD:     GPIO 46 (tie HIGH, not used for write)
  TFT_RS/DC:  GPIO 0
  TFT_CS:     GPIO –1 (not used)
  TFT_RST:    GPIO –1 (not used)
  Backlight:  GPIO 45 (PWM capable — use for brightness control)
Touch I2C:
  SDA:        GPIO 6
  SCL:        GPIO 5
  INT:        GPIO 7
Resolution:   480 (W) × 320 (H) — landscape default
```

Use **LovyanGFX** with a manual `LGFX_Device` class (do not use AUTODETECT — the Plus variant is not auto-detected correctly). Reference: https://github.com/sukesh-ak/WT32-SC01-PLUS-LVGL-IDF

---

## Part 1 — Project Structure & Dependencies

### 1.1 PlatformIO configuration (`platformio.ini`)

```ini
[env:wt32-sc01-plus]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.mcu = esp32s3
board_build.flash_mode = qio
board_build.psram_type = opi
board_build.arduino.memory_type = qio_opi
monitor_speed = 115200
upload_speed = 921600
build_flags =
  -D ARDUINO_USB_MODE=1
  -D ARDUINO_USB_CDC_ON_BOOT=1
  -D LV_CONF_INCLUDE_SIMPLE
  -D LV_LVGL_H_INCLUDE_SIMPLE
lib_deps =
  lvgl/lvgl@^8.3.0
  lovyan03/LovyanGFX@^1.1.9
  bblanchon/ArduinoJson@^7.0.0
  tzapu/WiFiManager@^2.0.17
  knolleary/PubSubClient@^2.8.0
```

### 1.2 Directory structure

```
src/
├── main.cpp
├── config.h              # pin defs, API base URL, constants
├── ui/
│   ├── ui_manager.h/.cpp # LVGL screen router
│   ├── screen_home.h/.cpp
│   ├── screen_focus.h/.cpp
│   ├── screen_wifi.h/.cpp
│   └── widgets/
│       ├── ring_timer.h/.cpp   # circular progress arc widget
│       ├── stat_card.h/.cpp    # XP / spoon / streak card
│       └── ambient_bg.h/.cpp   # animated background color
├── api/
│   ├── api_client.h/.cpp       # HTTP GET/POST wrapper
│   └── models.h                # C++ structs matching API JSON shapes
├── state/
│   ├── app_state.h             # global state singleton
│   └── session_machine.h/.cpp  # local timer state machine
└── wifi/
    └── wifi_manager.h/.cpp     # connection + storage
```

---

## Part 2 — Display & Touch Initialisation

### 2.1 LovyanGFX device class (`config.h`)

```cpp
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7701S _panel;
  lgfx::Bus_Parallel8 _bus;
  lgfx::Light_PWM     _light;
  lgfx::Touch_FT5x06  _touch;
public:
  LGFX(void) {
    { // Bus
      auto cfg = _bus.config();
      cfg.port = 0;
      cfg.freq_write = 20000000;
      cfg.pin_wr = 12; cfg.pin_rd = 46; cfg.pin_rs = 0;
      cfg.pin_d0 = 39; cfg.pin_d1 = 38; cfg.pin_d2 = 45;
      cfg.pin_d3 = 48; cfg.pin_d4 = 47; cfg.pin_d5 = 21;
      cfg.pin_d6 = 14; cfg.pin_d7 = 13;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Panel
      auto cfg = _panel.config();
      cfg.pin_cs = -1; cfg.pin_rst = -1; cfg.pin_busy = -1;
      cfg.panel_width = 320; cfg.panel_height = 480;
      cfg.offset_rotation = 1; // landscape
      _panel.config(cfg);
    }
    { // Backlight
      auto cfg = _light.config();
      cfg.pin_bl = 45;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    { // Touch
      auto cfg = _touch.config();
      cfg.i2c_port = 0;
      cfg.pin_sda = 6; cfg.pin_scl = 5;
      cfg.pin_int = 7;
      cfg.freq = 400000;
      cfg.x_min = 0; cfg.x_max = 479;
      cfg.y_min = 0; cfg.y_max = 319;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};
```

### 2.2 LVGL display driver flush & touch read

Wire LovyanGFX to LVGL in `main.cpp` using `lv_disp_drv_t` and `lv_indev_drv_t`. Use a double-buffer strategy: allocate two buffers of `480 * 10 * sizeof(lv_color_t)` in PSRAM (`ps_malloc`). The ESP32-S3 has enough PSRAM for this. Tick LVGL from a FreeRTOS task pinned to Core 0 at 5ms intervals.

---

## Part 3 — Screen Architecture

The device has exactly 4 screens managed by `ui_manager`. Screen transitions use `lv_scr_load_anim` with `LV_SCR_LOAD_ANIM_FADE_ON` (200ms). Never hard-switch screens.

```
BOOT (splash, wifi check)
  ↓ connected
HOME (dashboard)
  ├──[tap Focus]──→ FOCUS (timer console)
  ├──[tap WiFi icon]──→ WIFI SETTINGS
  └──[pull down]──→ refresh dashboard data
```

---

## Part 4 — Screen: HOME (Dashboard)

### 4.1 Layout (480×320, landscape)

```
┌──────────────────────────────────────────────────────┐
│  [●] FocusOS          [WiFi icon]  [Refresh icon]    │  ← 40px header bar
├────────────┬────────────┬──────────────┬─────────────┤
│   XP       │  Spoons    │   Streak     │  Level      │  ← 80px stat row (4 cards)
│  2,450 xp  │  8 / 12    │  🔥 14 days │  Lv 4       │
├────────────┴────────────┴──────────────┴─────────────┤
│  Tasks in progress                                   │  ← section label
│  ┌────────────────────────────────────────────────┐  │
│  │ 🔴 Design system audit          [▶ Focus]      │  │  ← task cards, scrollable list
│  │    Subtask: Define color tokens   ████░ 60%    │  │
│  ├────────────────────────────────────────────────┤  │
│  │ 🟡 Write API docs                [▶ Focus]     │  │
│  │    Subtask: Authentication flow  ██░░░ 40%     │  │
│  └────────────────────────────────────────────────┘  │
│                                   [+ New Task]       │  ← 40px footer
└──────────────────────────────────────────────────────┘
```

### 4.2 Stat cards (`stat_card` widget)

Each card is an `lv_obj` with:
- Icon (Unicode symbol from LVGL built-in font or a small PNG stored in flash)
- Value label (large, bold)
- Sub-label (small, muted)
- Color-coded left border: XP = purple, Spoons = teal, Streak = orange, Level = gold

Spoon card specifically must show a mini bar chart under the number: filled segments = spoons used, empty = remaining. Never show a negative count.

### 4.3 Task list

- Use `lv_list` or a custom `lv_obj` container with `lv_label` + `lv_bar` per row.
- Each task row shows: priority dot (colored), task name (truncated), active subtask name, progress bar (0–100%), and a **Focus** button.
- Tapping **Focus** on a task row pre-selects that task+subtask and navigates to the FOCUS screen.
- List is scrollable vertically. Max 10 tasks rendered at once (paginate or lazy-load if more).

### 4.4 Data refresh

On `screen_home` mount: call `api_client.getDashboard(userId)` which hits `GET /api/user/dashboard`. Parse response into `AppState`. Refresh every 60 seconds using an `lv_timer`. Show a spinner during fetch. On network error, show last cached data with a small "Offline" badge on the header.

---

## Part 5 — Screen: FOCUS (Timer Console)

### 5.1 Layout (480×320, landscape)

```
┌──────────────────────────────────────────────────────┐
│  [← Back]    Focus Session              [state pill] │  ← header (state pill shows FOCUS / HYPERFOCUS / BREAK)
├─────────────────────────┬────────────────────────────┤
│                         │  Task: Design system audit │
│   [Ring timer widget]   │  Subtask: Color tokens     │
│      large, centre      │  ────────────────────────  │
│    time remaining       │  Planned:    25 min        │
│     or time elapsed     │  Actual:     --            │
│                         │  Energy cost: -1 spoon     │
│                         │  ────────────────────────  │
│                         │  [▶ Start / ■ Stop]        │
│                         │  [☕ Take a Break]          │
├─────────────────────────┴────────────────────────────┤
│  Spoons today: ████████░░  8/12    Effort bonus: 1.0× │
└──────────────────────────────────────────────────────┘
```

### 5.2 Ring timer widget (`ring_timer` widget)

The ring timer is a custom LVGL widget using `lv_arc`. It consists of three layered arcs:

1. **Background arc** — full circle, dark grey, always visible.
2. **Base arc** — animates from 0° to 360° over `plannedDuration`. Color changes per state:
   - FOCUS: `#3D9A2B` (green)
   - HYPERFOCUS: stays green at 360°, secondary blue arc draws on top
   - BREAK: `#BA7517` (amber), frozen
   - DISENGAGED: `#E24B4A` (red), drains backward
3. **Overflow arc** — second `lv_arc` object, drawn on top, `#1A7AD4` (blue), starts at 0° and grows during HyperFocus. Max = `HYPERFOCUS_CAP_MINUTES / plannedDuration * 360°`.

Timer label inside the ring shows:
- FOCUS: `MM:SS remaining`
- HYPERFOCUS: `+MM:SS over`
- BREAK: `MM:SS break`
- DISENGAGED: `Resume`

Use `lv_anim_t` for smooth arc progression. Do NOT use `lv_timer` to set arc angle directly every tick — use animation with `lv_anim_set_values` and `lv_anim_set_time`.

### 5.3 State machine (local, on-device)

The device runs its own local session state machine in `session_machine.cpp`. This is the **primary timer authority on-device** — it does not poll the backend every second.

States: `IDLE → FOCUS → HYPERFOCUS → BREAK → DISENGAGED → IDLE`

State machine rules:
- On FOCUS start: record `startedAt = millis()`, set `plannedMs = plannedDuration * 60000`.
- Each loop tick: compute `elapsed = millis() - startedAt`. Update ring arc angle = `(elapsed / plannedMs) * 360`.
- On `elapsed >= plannedMs`: automatically transition to HYPERFOCUS (if no user action). Show non-blocking toast.
- HyperFocus: continue elapsed counting. Overflow arc angle = `((elapsed - plannedMs) / (HYPERFOCUS_CAP_MS)) * 360`.
- On HyperFocus cap: transition to BREAK automatically.
- On BREAK end + 30s idle: transition to DISENGAGED. Start decay animation on base arc.
- All transitions POST to backend: `POST /api/sessions/:id/state` with `{ state, timestamp }`.

### 5.4 Ambient background

`ambient_bg.cpp` changes the screen background color on state transitions using `lv_obj_set_style_bg_color` with an animated color interpolation over 800ms. Use `lv_color_mix()` in a timer callback over 16 steps (50ms each). Color targets:

```cpp
FOCUS:       lv_color_hex(0xEAF3DE)  // soft green tint
HYPERFOCUS:  lv_color_hex(0xE6F1FB)  // soft blue tint
BREAK:       lv_color_hex(0xFAEEDA)  // soft amber tint
DISENGAGED:  lv_color_hex(0xFCEBEB)  // soft red tint
IDLE:        lv_color_hex(0x1A1A1A)  // dark neutral
```

### 5.5 Non-blocking toast (HyperFocus prompt)

When the planned timer ends, show a toast at the bottom of the screen (not a modal — do not block touch on the rest of the UI). Toast has two buttons: **Continue** and **Take a Break**. Auto-dismiss after 30 seconds (transitions to HyperFocus automatically if not dismissed). Implement as a fixed `lv_obj` with `LV_LAYER_TOP` alignment.

### 5.6 Session sync with backend

The device does not manage the authoritative session record — that lives on the server. The device's role:
- `POST /api/sessions` on Focus start → receive `sessionId`.
- `PATCH /api/sessions/:id` with `{ state, elapsed, overflowMinutes }` on every state transition.
- `PATCH /api/sessions/:id` with `{ actualDuration, state: "COMPLETE" }` on session end.
- `GET /api/sessions/:id` on device boot/wake to restore an in-progress session (prevents data loss on reset).

All API calls are non-blocking. Use a FreeRTOS task queue: push API call descriptors to a queue from the UI task, process them from a dedicated network task on Core 1.

---

## Part 6 — Screen: WIFI SETTINGS

### 6.1 Layout

```
┌──────────────────────────────────────────────────────┐
│  [← Back]    Wi-Fi Settings                         │
├──────────────────────────────────────────────────────┤
│  Status: ● Connected — HomeNetwork (192.168.1.45)    │
│  ─────────────────────────────────────────────────── │
│  Available networks:                                 │
│  ┌───────────────────────────────────────────────┐   │
│  │ ▲▲▲  HomeNetwork                 [Connected] │   │
│  │ ▲▲░  OfficeWiFi                  [Connect  ] │   │
│  │ ▲░░  Neighbour5G                 [Connect  ] │   │
│  └───────────────────────────────────────────────┘   │
│  [🔍 Scan again]                                     │
│  ─────────────────────────────────────────────────── │
│  API Server URL:  [________________________]         │
│  Device ID:       [________________________]         │
│  [💾 Save]                                           │
└──────────────────────────────────────────────────────┘
```

### 6.2 WiFi scan & connect

Use `WiFi.scanNetworks(async=true)` to avoid blocking the UI. Poll scan status in an `lv_timer`. When a network is tapped:
1. If it's open: connect immediately.
2. If it's secured: show a password entry keyboard (`lv_keyboard` built-in LVGL widget, type = `LV_KEYBOARD_MODE_TEXT_LOWER`). On confirm: `WiFi.begin(ssid, password)`.

Store credentials in NVS (Non-Volatile Storage) using `Preferences` library:
```cpp
Preferences prefs;
prefs.begin("wifi", false);
prefs.putString("ssid", ssid);
prefs.putString("pass", password);
```

On boot: attempt auto-reconnect using stored credentials before showing any screen.

### 6.3 API server URL storage

The device needs to know your backend's IP/hostname. Store in NVS key `api_url`. Default: `http://192.168.1.x:3000`. The WiFi settings screen exposes a text field (using `lv_keyboard`) to update this. All `api_client` calls read from this stored value.

### 6.4 Device ID

Each device needs a persistent identifier to associate it with a logged-in user. Generate from ESP32 MAC address on first boot:
```cpp
String deviceId = "device_" + String((uint32_t)ESP.getEfuseMac(), HEX);
```
Store in NVS. Display read-only in WiFi settings. The user links this device ID to their account via the web app's Settings page (you will need to add a "Linked Devices" section to the web settings — out of scope for this firmware prompt but note it here).

---

## Part 7 — API Client (`api_client.cpp`)

### 7.1 Endpoints used by the device

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/user/dashboard` | Home screen: XP, spoons, streak, level, active tasks |
| GET | `/api/tasks?status=active` | Focus screen: task + subtask list for selection |
| POST | `/api/sessions` | Start a new focus session |
| PATCH | `/api/sessions/:id` | Update session state / elapsed |
| GET | `/api/sessions/active` | Restore in-progress session on device boot |

### 7.2 Request pattern

```cpp
// api_client.cpp — non-blocking via FreeRTOS task
void ApiClient::getDashboard(std::function<void(DashboardData)> callback) {
  xTaskCreate([](void* param) {
    HTTPClient http;
    http.begin(config.apiBaseUrl + "/api/user/dashboard");
    http.addHeader("x-device-id", config.deviceId);
    int code = http.GET();
    if (code == 200) {
      JsonDocument doc;
      deserializeJson(doc, http.getStream());
      DashboardData data = DashboardData::fromJson(doc);
      // push result back to UI task via queue
    }
    http.end();
    vTaskDelete(NULL);
  }, "api_get", 8192, callbackWrapper, 1, NULL);
}
```

All API responses are parsed into typed C++ structs defined in `models.h`. Never parse JSON on the UI task.

### 7.3 Authentication

The device uses a device-scoped auth token rather than a user JWT. On first pairing, the web app generates a `deviceToken` tied to the user's account. The device stores this in NVS. All API requests include header: `x-device-token: <token>`. The backend validates this token and resolves the associated `userId`.

Add a new backend endpoint: `POST /api/devices/pair` — accepts `{ deviceId, pairingCode }` and returns `{ deviceToken, userId }`. The pairing code is a 6-digit code displayed on the device and entered in the web app.

---

## Part 8 — FreeRTOS Task Architecture

To keep LVGL smooth (target: 30fps minimum), all blocking operations must run off the UI task.

```
Core 0 (UI):
  - lvgl_tick_task   (5ms, updates lv_tick_inc)
  - lvgl_handler_task (10ms, calls lv_task_handler)
  - session_machine_task (100ms, updates timer progress)

Core 1 (Network):
  - api_queue_task   (processes API request queue)
  - wifi_monitor_task (monitors connection, reconnects on drop)
```

Use `lvgl_acquire()` / `lvgl_release()` mutex guards whenever a non-UI task needs to update LVGL objects. This is critical — LVGL is not thread-safe.

```cpp
portMUX_TYPE lvgl_mux = portMUX_INITIALIZER_UNLOCKED;
void lvgl_acquire() { taskENTER_CRITICAL(&lvgl_mux); }
void lvgl_release() { taskEXIT_CRITICAL(&lvgl_mux); }
```

---

## Part 9 — Boot Sequence

```
1. Serial init (115200)
2. NVS init — read stored WiFi credentials, API URL, deviceToken
3. Display init (LovyanGFX) — show splash screen immediately (no WiFi needed)
4. LVGL init — register display driver + touch driver
5. WiFi connect attempt (stored credentials)
   - If connected → proceed to step 6
   - If not connected → show WiFi Settings screen
6. GET /api/sessions/active
   - If active session found → restore state, go to FOCUS screen
   - Else → GET /api/user/dashboard, go to HOME screen
7. Start FreeRTOS tasks
```

Show a splash screen (logo + "Connecting…" label) during steps 2–6. The splash must render from flash — do not depend on network for initial render.

---

## Part 10 — Implementation Notes & Constraints

1. **LVGL version:** Use LVGL 8.3.x, not 9.x. The LovyanGFX + WT32-SC01 Plus combination is tested with 8.3.x. Confirm before using any 9.x API.

2. **PSRAM:** The ESP32-S3-WROVER has 8MB OPI PSRAM. Allocate LVGL draw buffers and large JSON documents in PSRAM using `ps_malloc()`, not `malloc()`. ArduinoJson can target PSRAM by providing a custom allocator.

3. **ST7701S parallel bus:** The display is connected via 8-bit parallel RGB, NOT SPI. Do not attempt to use TFT_eSPI — it does not support this bus type on ESP32-S3. LovyanGFX with `Bus_Parallel8` is the correct driver.

4. **Touch interrupt vs polling:** The FT6336U touch controller supports interrupt mode (INT pin = GPIO 7). Use interrupt-driven touch reading, not polling, to avoid I2C bus contention with the display parallel bus.

5. **JSON document sizing:** Use `JsonDocument` (ArduinoJson v7) instead of `DynamicJsonDocument` (v6). Do not hard-code document sizes.

6. **Offline resilience:** Cache the last successful dashboard response in NVS (serialised JSON string). If the device cannot reach the API, render cached data with a stale indicator. The local session state machine continues to run offline — sync resumes when the connection is restored.

7. **Backlight brightness:** Expose brightness in the WiFi/Settings screen as a slider (0–100%). Write to NVS. Apply via LovyanGFX `setBrightness(value)` on the panel light instance.

8. **Screen rotation:** The WT32-SC01 Plus is physically landscape (wider than tall). Set `offset_rotation = 1` in the LGFX config. All LVGL layouts should be designed for 480×320, not 320×480.

9. **Button debounce:** There are no physical buttons on the WT32-SC01 Plus main face. All interaction is capacitive touch. Implement a 150ms debounce on `lv_event_cb` handlers to prevent double-tap ghost events.

10. **No OTA in scope:** Do not implement OTA update in this sprint. Reserve GPIO for future use.

---

## Part 11 — Backend additions needed (Node.js)

These small backend changes are required to support the device. Implement alongside the firmware.

```
POST /api/devices/pair
  Body: { deviceId: string, pairingCode: string }
  Response: { deviceToken: string, userId: string }
  Logic: verify 6-digit code (stored in-memory for 5 min after user initiates pairing from web app),
         generate a signed deviceToken (JWT with { deviceId, userId, type: "device" }),
         store in DeviceToken model.

GET /api/sessions/active
  Auth: x-device-token header
  Response: active Session record for the user, or null
  Logic: find Session where userId matches token AND state NOT IN ["COMPLETE", "IDLE"]

DeviceToken model (Prisma):
  id          String   @id @default(auto()) @map("_id") @db.ObjectId
  deviceId    String   @unique
  userId      String
  token       String
  createdAt   DateTime @default(now())
  lastSeenAt  DateTime?
```

Existing session endpoints (`POST /api/sessions`, `PATCH /api/sessions/:id`) should accept the `x-device-token` header in addition to the existing Clerk JWT header, resolving `userId` from the device token when the Clerk header is absent.

---

*Implement in this order: Part 0 (pin config + display init) → Part 2 (LVGL up and rendering) → Part 6 (WiFi settings, pairing) → Part 4 (Home screen, static layout) → Part 5 (Focus screen + ring widget) → Part 7 (API client live data). Confirm display is rendering at target framerate before wiring any API calls.*