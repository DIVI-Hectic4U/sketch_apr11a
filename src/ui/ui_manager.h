#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <lvgl.h>

/**
 * Screen identifiers for the UI Manager router.
 */
enum class Screen {
    BOOT,
    HOME,
    FOCUS,
    WIFI
};

/**
 * UIManager
 * Handles screen transitions and global UI state.
 */
class UIManager {
public:
    static UIManager& getInstance() {
        static UIManager instance;
        return instance;
    }

    /**
     * Navigates to a specific screen with a fade animation.
     */
    void moveTo(Screen screen);

    /**
     * Gets the current active screen.
     */
    Screen getCurrentScreen() const { return currentScreen; }

private:
    UIManager() : currentScreen(Screen::BOOT) {}
    
    Screen currentScreen;
    
    // Internal screen creation functions (stubs for now)
    lv_obj_t* createBootScreen();
    lv_obj_t* createHomeScreen();
    lv_obj_t* createFocusScreen();
    lv_obj_t* createWifiScreen();
};

#endif // UI_MANAGER_H
