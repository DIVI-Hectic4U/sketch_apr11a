#include "display.h"
#include "pin_verify.h"
#include "src/state/app_state.h"
#include "src/ui/ui_manager.h"
#include "src/wifi/wifi_manager.h"
#include "src/state/session_machine.h"
#include "src/api/api_client.h"

void setup() {
    // 1. Serial Initialization
    Serial.begin(115200);
    delay(2000); 
    
    // 2. Hardware Pin Verification
    printPinVerification();

    // 3. Display & Graphics Driver Init
    display_init();

    // 4. Initial Screen (Boot)
    UIManager::getInstance().moveTo(Screen::BOOT);
    Serial.println("System Booting...");

    // 5. WiFi Initialization
    WiFiManagerWrapper::getInstance().init();
    APIClient::getInstance().init();

    // 6. Transition to Home after a short delay (Simulating boot process)
    delay(2000); 
    UIManager::getInstance().moveTo(Screen::HOME);
    
    Serial.println("Setup Complete!");
}

void loop() {
    // Update background tasks
    WiFiManagerWrapper::getInstance().update();
    SessionMachine::getInstance().update();
    APIClient::getInstance().update();
    
    delay(100);
}
