/**
 * @file hardware_controller.h
 * @brief 硬件抽象层 (Hardware Abstraction Layer - HAL) API
 * @details 定义逻辑层与驱动层的交互契约。
 * @version 1.2 (MVP Clean Version)
 */

#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include <Arduino.h>

// --- 常量定义 ---


// 在开环控制（无编码器）系统中，无法让 Speed 直接匹配物理速度 (m/s)。
// 同样是 PWM 200，空载时可能 0.5m/s，满载(30kg)时可能只有 0.2m/s。
// 建议：在 MVP 阶段保持 PWM 控制。未来如果加入 AI，可以通过 AI 模型拟合 "PWM + 电流 -> 速度" 的关系。
constexpr int MAX_MOTOR_SPEED = 255;
constexpr int MIN_MOTOR_SPEED = 80;

// --- 1. 初始化 ---

/**
 * @brief 初始化所有硬件引脚
 * 配置 PWM, Limit Switch 以及电流读取引脚
 */
void setupHardware();

// --- 2. 电机控制 ---

/**
 * @brief 电机上升
 * @param pwm_val PWM占空比 (0-255). 
 */
void motorGoUp(int pwm_val = MAX_MOTOR_SPEED);

/**
 * @brief 电机下降
 * @param pwm_val PWM占空比 (0-255). 
 */
void motorGoDown(int pwm_val = MAX_MOTOR_SPEED);

/**
 * @brief 主动刹车
 * 无论当前在做什么，强制将 H 桥输出拉低或断开使能。
 */
void stopMotor();

// --- 3. 传感器读取 ---

/**
 * @brief 顶部限位状态
 * @return true = 撞到了顶部 (危险/校准点)
 */
bool isTopLimitPressed();

// [已删除] isBottomLimitPressed()
// 原因：2025-12-03 变更：移除底部物理开关，改为基于时间的软限位。

// --- 4. 遥测与 AI 数据接口 (预留) ---

// [暂时移除] 2025-12-03: MVP阶段不需要电流读取，保持接口简洁。
// ⚠️ 重要硬件提醒：
// 虽然这里代码注释掉了，但在硬件接线时，请务必将 BTS7960 的 R_IS 和 L_IS 引脚
// 连接到 ESP32 的 ADC 引脚（并预留分压电阻），以免未来升级 AI 功能时需要重新焊接！
// int getMotorCurrentRaw();

#endif // HARDWARE_CONTROLLER_H