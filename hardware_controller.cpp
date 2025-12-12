/**
* @file hardware_controller.cpp
 * @brief [MOCK 版本] 硬件抽象层的模拟实现
 * * @details
 * 因为硬件队友还没写完驱动，我们这里用 Serial.print 来模拟硬件行为。
 * 这让逻辑开发人员可以独立测试业务逻辑。
 */

#include "hardware_controller.h"

// --- 1. 初始化实现 ---

void setupHardware() {
    // 假装初始化了引脚
    Serial.println("[Mock硬件] 硬件初始化完成 (虚拟模式)");
}

// --- 2. 电机控制实现 (只打印，不转动) ---

static unsigned long lastPrintTime = 0;

void motorGoUp(int speed) {
    // 限制打印频率，避免刷屏
    if (millis() - lastPrintTime > 1000) {
        Serial.printf("[Mock硬件] 电机正在上升... 速度: %d\n", speed);
        lastPrintTime = millis();
    }
}

void motorGoDown(int speed) {
    // 限制打印频率，避免刷屏
    if (millis() - lastPrintTime > 1000) {
        Serial.printf("[Mock硬件] 电机正在下降... 速度: %d\n", speed);
        lastPrintTime = millis();
    }
}

void stopMotor() {
    // 假装电机停了
    if (millis() - lastPrintTime > 1000) {
        Serial.println("[Mock硬件] 电机已停止");
        lastPrintTime = millis();
    }
    
}

// --- 3. 传感器读取实现 (手动控制返回值) ---

static bool _mockLimitState = false; // 内部变量，记录模拟开关状态

void setMockTopLimit(bool pressed) {
    _mockLimitState = pressed;
    if (pressed) {
        Serial.println("[Mock硬件] 👆 模拟限位开关: 已按下 (PRESSED)");
    } else {
        Serial.println("[Mock硬件] 👇 模拟限位开关: 已松开 (RELEASED)");
    }
}

bool isTopLimitPressed() {
    return _mockLimitState;
}
