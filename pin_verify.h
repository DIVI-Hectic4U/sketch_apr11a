#ifndef PIN_VERIFY_H
#define PIN_VERIFY_H

#include <Arduino.h>

/**
 * Hardware Pin Verification Step
 * Prints the GPIO configuration for the WT32-SC01 Plus to Serial.
 */
inline void printPinVerification() {
    Serial.println("\n========================================");
    Serial.println("  WT32-SC01 PLUS PIN VERIFICATION");
    Serial.println("========================================");
    Serial.println("Display (8-bit Parallel MCU8080):");
    Serial.println("  D0-D7: 9, 46, 3, 8, 18, 17, 16, 15");
    Serial.println("  WR   : 47");
    Serial.println("  RS   : 0 (Data/Command)");
    Serial.println("  RST  : 4 (Shared with Touch)");
    Serial.println("  BL   : 45 (PWM)");
    Serial.println("\nTouch (I2C1):");
    Serial.println("  SDA  : 6");
    Serial.println("  SCL  : 5");
    Serial.println("  INT  : 7");
    Serial.println("========================================\n");
}

#endif // PIN_VERIFY_H
