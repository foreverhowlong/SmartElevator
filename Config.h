#ifndef CONFIG_H
#define CONFIG_H

// ==========================
// 1. 引脚定义 (Pin Definitions)
// ==========================
// BTS7960 Motor Driver
#define PIN_MOTOR_R_PWM    16  // 右旋 PWM (下降)
#define PIN_MOTOR_L_PWM    17  // 左旋 PWM (上升)
#define PIN_MOTOR_R_EN     18  // 右旋使能 (可用于急停)
#define PIN_MOTOR_L_EN     19  // 左旋使能

// Sensors
#define PIN_LIMIT_TOP      21  // 顶部物理限位开关 (常闭/常开需确认)

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

// ==========================
// 4. 系统状态枚举
// ==========================
enum SystemState {
    STATE_IDLE,             // 待机
    STATE_CALIBRATING,      // 正在归零（向上找开关）
    STATE_MOVING_UP,        // 正常上升
    STATE_MOVING_DOWN,      // 正常下降
    STATE_ERROR,            // 故障/急停
    STATE_POS_UNKNOWN       // 位置未知（刚开机，必须先归零）
};

#endif