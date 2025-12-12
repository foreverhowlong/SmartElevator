/*
 * 智能载物机 (Smart Hoist) - MVP Firmware
 * 平台：ESP32 NodeMCU-32S
 * 架构：Layered Architecture (Hardware -> Logic -> Network)
 */

// 1. 引入各层模块
#include "Config.h"               // 配置参数
#include "hardware_controller.h"  // 硬件抽象层
#include "HoistStateMachine.h"    // 业务逻辑层
#include "blynk_manager.h"        // 网络通信层

// 2. 全局对象实例化
// 状态机实例，blynk_manager.h 中通过 'extern' 访问它
HoistStateMachine hoist;

// ------------------------------------------------
// Setup: 系统初始化
// ------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n>>> Smart Hoist System Booting...");

    // A. 初始化硬件层 (GPIO, PWM)
    // 这一步必须最先执行，确保电机引脚处于安全状态
    setupHardware();
    Serial.println(" - Hardware Layer: OK");

    // B. 初始化网络层 (Wi-Fi, Blynk)
    // 注意：Blynk.begin() 是阻塞的，连不上网会卡在这里
    setupBlynk();
    Serial.println(" - Network Layer: OK");

    // C. 初始化业务逻辑层 (StateMachine)
    hoist.begin();
    Serial.println(" - Logic Layer: OK");
    
    // D. 自动开始归零
    Serial.println(">>> System Ready. Auto-Calibrating...");
    updateAppStatus("🔄 Auto-Calibrating...");
    hoist.commandGoTop(); // <--- 加上这一行
}

// ------------------------------------------------
// Loop: 主循环 (不要使用 delay)
// ------------------------------------------------
void loop() {
    // 1. 处理网络通信 (心跳、接收指令)
    runBlynk();

    // 2. 运行核心状态机 (高频调用，处理运动控制)
    hoist.update();

    // 3. 串口调试输出 (每500ms打印一次)
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 1000) {
        // 打印当前状态名和估算位置
        Serial.printf("[State: %s] Pos: %ld ms | Limit: %s\n", 
                      hoist.getStateName(), 
                      hoist.getCurrentPosition(),
                      isTopLimitPressed() ? "HIT" : "OPEN");
        lastLog = millis();
    }

    // 4. 串口指令控制 (调试神器)
    if (Serial.available()) {
        char cmd = Serial.read();
        // 忽略换行符
        if (cmd == '\n' || cmd == '\r') return;

        switch (cmd) {
            case 't': hoist.commandGoTop(); break;
            case 'm': hoist.commandGoMiddle(); break;
            case 'b': hoist.commandGoBottom(); break;
            case 's': hoist.emergencyStop(); break;
            case 'p': setMockTopLimit(true); break;  // 按下开关
            case 'r': setMockTopLimit(false); break; // 松开开关
            default: Serial.printf("Unknown command: %c\n", cmd); break;
        }
    }
}