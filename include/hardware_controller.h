/**
 * @file hardware_controller.h
 * @brief 硬件抽象层 (Hardware Abstraction Layer - HAL) API
 * * @details
 * 这个文件定义了所有与物理硬件（电机、限位开关）交互的“契约”。
 * 它是“网络逻辑层” (main.ino) 和“物理实现层” (hardware_controller.cpp) 之间的桥梁。
 * * 我们的设计哲学是：
 * 1. **抽象化:** `main.ino` 不应该知道什么是 'analogWrite' 或 'GPIO 25'。
 * 2. **可测试性:** 我们可以编写一个单独的 'test_hardware.ino' 来独立测试这个文件中的所有功能。
 * 3. **可扩展性:** 如果未来更换电机驱动（例如从 BTS7960 换成 Pololu），
 * 我们只需要修改 .cpp 文件，这个 .h 文件和 main.ino 可以保持不变。
 * * @version 1.0
 * @date 2025-11-14
 */

#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include <Arduino.h> // 引入 Arduino 核心库

// --- 常量定义 ---

/**
 * @brief 定义电机的最大PWM速度值 (0-255)。
 * 这是一个可配置点，用于限制最大速度，增加安全性。
 */
 //焦仁川：这里有没有可能让speed直接匹配物理速度？
constexpr int MAX_MOTOR_SPEED = 255;

/**
 * @brief 定义电机的最小启动PWM速度值。
 * 小于此值电机可能因摩擦力无法启动。 (可扩展功能)
 */
constexpr int MIN_MOTOR_SPEED = 80;


// --- 1. 初始化 ---

/**
 * @brief 初始化所有硬件引脚。
 * @details
 * 必须在 `setup()` 函数中第一个调用。
 * 它会配置所有电机和传感器引脚的模式 (INPUT/OUTPUT)。
 */
void setupHardware();


// --- 2. 电机控制 (可扩展的 API) ---

/**
 * @brief 命令电机上升（拉起重物）。
 * @param speed 速度值 (0-255)。函数内部会将其限制在 [0, MAX_MOTOR_SPEED] 范围内。
 */
void motorGoUp(int speed = MAX_MOTOR_SPEED);

/**
 * @brief 命令电机下降（放下重物）。
 * @param speed 速度值 (0-255)。
 */
void motorGoDown(int speed = MAX_MOTOR_SPEED);

/**
 * @brief 停止电机 (主动刹车)。
 * @details
 * 这是“主动刹车”(Brake)模式，非“滑行”(Coast)。
 * 它会将两个PWM引脚都设置为LOW。
 */
void stopMotor();


// --- 3. 传感器读取 (安全层) ---

/**
 * @brief 检查“顶部限位开关”是否被触发。
 * @return true 如果顶部开关被按下 (触发)。
 * @return false 如果开关未被按下 (安全)。
 */
bool isTopLimitPressed();

/**
 * @brief 检查“底部限位开关”是否被触发。
 * @return true 如果底部开关被按下 (触发)。
 * @return false 如果开关未被按下 (安全)。
 */
bool isBottomLimitPressed();

#endif // HARDWARE_CONTROLLER_H