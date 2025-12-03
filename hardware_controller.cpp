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

void motorGoUp(int speed) {
    // 假装电机在转
    Serial.printf("[Mock硬件] 电机正在上升... 速度: %d\n", speed);
}

void motorGoDown(int speed) {
    // 假装电机在转
    Serial.printf("[Mock硬件] 电机正在下降... 速度: %d\n", speed);
}

void stopMotor() {
    // 假装电机停了
    Serial.println("[Mock硬件] 电机已停止");
}

// --- 3. 传感器读取实现 (手动控制返回值) ---

// 你可以在这里修改返回值来测试"撞到限位"的情况
// true = 撞到了 (会触发停止), false = 没撞到 (安全)

bool isTopLimitPressed() {
    // 默认返回 false (没撞到)，这样你的电机才能动起来
    // 如果你想测试"到顶停止"的逻辑，把这里改成 true 再烧录一次
    return false;
}

bool isBottomLimitPressed() {
    return false;
}