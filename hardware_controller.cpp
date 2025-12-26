/**
 * @file hardware_controller.cpp
 * @brief 硬件抽象层的真实实现 (Real Hardware Implementation)
 * @details 集成了 BTS7960 电机驱动与 HC-SR04 超声波限位逻辑
 */

#include "hardware_controller.h"
#include "Config.h"

// --- 1. 初始化实现 ---

void setupHardware() {
    pinMode(PIN_MOTOR_RPWM, OUTPUT);
    pinMode(PIN_MOTOR_LPWM, OUTPUT);
    digitalWrite(PIN_MOTOR_RPWM, LOW);
    digitalWrite(PIN_MOTOR_LPWM, LOW);

    pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
    pinMode(PIN_ULTRASONIC_ECHO, INPUT);
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

    Serial.println("[硬件] 硬件初始化完成 (真实驱动模式)");
}

// --- 2. 电机控制实现 ---

void motorGoDown(int speed) {
    // stopMotor(); // Optimization: Removed redundant stop to prevent PWM jitter in loop
    // Caller (State Machine) must ensure direction switch safety.

    // 用户代码定义: motorGoUp -> RPWM=HIGH (Up), LPWM=LOW
    // 使用 analogWrite 支持调速
    // 如果 speed 为 255，效果等同于 digitalWrite(HIGH)
    digitalWrite(PIN_MOTOR_LPWM, LOW);
    analogWrite(PIN_MOTOR_RPWM, speed); 
    
    // Serial.printf("[硬件] 电机下降 (Speed: %d)\n", speed);
}

void motorGoUp(int speed) {
    // stopMotor(); // Optimization: Removed redundant stop to prevent PWM jitter in loop

    // 用户代码定义: motorGoUp -> RPWM=LOW, LPWM=HIGH (Down)
    digitalWrite(PIN_MOTOR_RPWM, LOW);
    analogWrite(PIN_MOTOR_LPWM, speed);
    
    // Serial.printf("[硬件] 电机上升 (Speed: %d)\n", speed);
}

void stopMotor() {
    // 强制拉低两端
    digitalWrite(PIN_MOTOR_RPWM, LOW);
    digitalWrite(PIN_MOTOR_LPWM, LOW);
    // 此外对于 PWM 引脚，最好显式 write 0 以关闭 PWM 计时器
    analogWrite(PIN_MOTOR_RPWM, 0);
    analogWrite(PIN_MOTOR_LPWM, 0);
}

// --- 3. 传感器读取实现 ---

// 移除 Mock 相关的变量和函数
void setMockTopLimit(bool pressed) {
    // 真实硬件模式下，此函数无效
}

bool isTopLimitPressed() {
    // 发送触发信号
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

    // 读取回声信号 
    // Optimization: Reduced timeout to 6000us (~100cm range). 
    // Target is 42.5cm, so 100cm is sufficient. 30ms (5m) was too blocking.
    long duration = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, 6000); 

    if (duration == 0) {
        // 超时或读取失败，通常意味着距离很远（没挡住），或者传感器故障
        // 保守起见，如果没有回波，假设没有到达顶部（或者视为故障停止？）
        // 这里暂时假设未到达
        return false; 
    }

    // 计算距离（cm）
    float distance = duration * 0.034 / 2;


    // 
    // 正确逻辑: 如果距离 <= 给定距离，说明到达顶部限位，应返回 true
    if (distance > 0 && distance <= SENSOR_DISTANCE_LIMIT) {
        // Serial.printf("[硬件] 顶部限位触发! 距离: %.2f cm\n", distance);
        return true; 
    }
    
    return false;
}