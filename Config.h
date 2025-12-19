#ifndef CONFIG_H
#define CONFIG_H

// ==========================
// 1. 引脚定义 (Pin Definitions)
// ==========================
// BTS7960 Motor Driver
// 注意：根据最新硬件驱动，RPWM负责上升，LPWM负责下降 (或者反之，取决于接线)
// 用户代码: RPWM=18(Up), LPWM=19(Down)
#define PIN_MOTOR_RPWM    18  // 对应用户代码的 RPWM_PIN (Up)
#define PIN_MOTOR_LPWM    19  // 对应用户代码的 LPWM_PIN (Down)

// Ultrasonic Sensor (HC-SR04)
#define PIN_ULTRASONIC_TRIG  27
#define PIN_ULTRASONIC_ECHO  26

// ==========================
// 2. 机械参数 (Mechanical Params)
// ==========================
// 这里的单位是“毫秒”，代表电机全速运行的时间
// 假设从顶到底（2米）需要运行 8秒 (8000ms)

const unsigned long TIME_TO_MIDDLE_MS = 4000; // 顶层 -> 中层 耗时
const unsigned long TIME_TO_BOTTOM_MS = 8000; // 顶层 -> 底层 (虚拟底部) 耗时

// 安全冗余：允许的最大下降时间（防止过卷）
// 既然绳子有5-6米，设为 10000ms 比较安全，超过这个值软件强制锁死
const unsigned long MAX_SAFE_POSITION_MS = 10000; 

// ==========================
// 3. 速度控制 (PWM)
// ==========================
const int PWM_SPEED_UP   = 200; // 0-255，上升通常需要更大扭矩
const int PWM_SPEED_DOWN = 150; // 下降利用重力，速度可以小一点

// 4. 系统状态枚举
enum SystemState {
    STATE_IDLE,             // 待机
    STATE_CALIBRATING,      // 正在归零（向上找开关）
    STATE_MOVING_UP,        // 正常上升
    STATE_MOVING_DOWN,      // 正常下降
    STATE_ERROR,            // 故障/急停
    STATE_POS_UNKNOWN       // 位置未知（刚开机，必须先归零）
};

// ==========================
// 5. 维护与安全参数
// ==========================
// 维护基准时间 (平均上升时间) - 用于短期异常检测
// 暂时设为 TIME_TO_BOTTOM_MS (最坏情况), 实际应更短
const unsigned long MAINTENANCE_BASELINE_MS = TIME_TO_BOTTOM_MS;
const double SENSOR_DISTANCE_LIMIT = 42.5;

#endif