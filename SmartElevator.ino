/*
 * 智能载物机 (Smart Hoist) - MVP Firmware
 * 平台：ESP32 NodeMCU-32S
 * 架构：Layered Architecture (Hardware -> Logic -> Network)
 */

// 1. 引入各层模块
#include "Config.h"               // 配置参数
#include "hardware_controller.h"  // 硬件抽象层
#include "HoistStateMachine.h"    // 业务逻辑层
#include "MaintenanceManager.h"   // 维护管理模块
#include "SchedulerManager.h"     // 定时调度模块
#include "blynk_manager.h"        // 网络通信层

// 2. 全局对象实例化
// 状态机实例，blynk_manager.h 中通过 'extern' 访问它
HoistStateMachine hoist;
MaintenanceManager maintenance;
SchedulerManager scheduler;

// ------------------------------------------------
// Setup: 系统初始化
// ------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n>>> Smart Hoist System Booting...");

    // A. 初始化硬件层 (GPIO, PWM)
    setupHardware();
    Serial.println(" - Hardware Layer: OK");

    // B. 初始化管理模块 (NVS, NTP)
    maintenance.begin();
    scheduler.begin();
    // 绑定维护管理器到状态机
    hoist.bindMaintenanceManager(&maintenance);
    Serial.println(" - Managers: OK");
    
    // 初始化随机种子 (用于 Demo 数据生成)
    randomSeed(analogRead(0));

    // C. 初始化网络层 (Wi-Fi, Blynk)
    setupBlynk();
    Serial.println(" - Network Layer: OK");

    // D. 初始化业务逻辑层 (StateMachine)
    hoist.begin();
    Serial.println(" - Logic Layer: OK");
    
    // E. 自动开始归零
    Serial.println(">>> System Ready. Auto-Calibrating...");
    updateAppStatus("🔄 Auto-Calibrating...");
    hoist.commandGoTop(); 
}

// ------------------------------------------------
// Loop: 主循环 (不要使用 delay)
// ------------------------------------------------
void loop() {
    // 1. 处理网络通信 (心跳、接收指令)
    runBlynk();

    // 2. 运行核心状态机 (高频调用，处理运动控制)
    hoist.update();
    
    // 3. 运行调度器检查 (Auto-Run)
    int schedAction = scheduler.checkTrigger();
    if (schedAction == 1) { // Auto-Up
        // 仅在空闲且未在顶端时执行
        if (hoist.getState() == STATE_IDLE && !isTopLimitPressed()) {
             Serial.println("[Scheduler] ⏰ Auto-UP Triggered!");
             hoist.commandGoTop();
        }
    } else if (schedAction == 2) { // Auto-Down
        if (hoist.getState() == STATE_IDLE) {
             Serial.println("[Scheduler] ⏰ Auto-DOWN Triggered!");
             hoist.commandGoBottom();
        }
    }

    // 4. 定时任务 (状态上报 & 调试日志 & Demo回放)
    static unsigned long lastLog = 0;
    
    // --- Demo 模式变量 ---
    static bool isDemoPlaying = false;
    static int demoPlayIndex = 0;
    static unsigned long lastDemoStep = 0;

    // A. Demo 数据回放逻辑 (每 500ms 推送一个历史点，绘制图表)
    if (isDemoPlaying && millis() - lastDemoStep > 1000) {
        if (demoPlayIndex < maintenance.getHistoryCount()) {
            long val = maintenance.getHistoryItem(demoPlayIndex);
            
            // 推送单次耗时 (绘制波形)
            Blynk.virtualWrite(V5, (int)val);
            // 同时也稍微推一下斜率 (为了让斜率图也动起来，虽然斜率是基于整体的)
            // 这里我们直接推送当前的真实计算斜率，因为 generateDemoData 已经更新了数组，斜率会很高
            Blynk.virtualWrite(V4, maintenance.calculateSlope());

            Serial.printf("[Demo] Replaying history [%d]: %ld ms\n", demoPlayIndex, val);
            demoPlayIndex++;
            lastDemoStep = millis();
        } else {
            isDemoPlaying = false;
            Serial.println("[Demo] Playback finished.");
            updateAppStatus("✅ Demo Replay Done");
        }
    }

    if (millis() - lastLog > 1000) {
        // A. 串口打印
        Serial.printf("[State: %s] Pos: %ld ms | Limit: %s\n", 
                      hoist.getStateName(), 
                      hoist.getCurrentPosition(),
                      isTopLimitPressed() ? "HIT" : "OPEN");
        
        // B. APP 状态文字更新
        String statusStr = "✅ " + String(hoist.getStateName());
        if (isDemoPlaying) statusStr = "📊 Demo Mode: Uploading..."; // Demo 状态提示
        else if (hoist.getState() == STATE_ERROR) statusStr = "⚠️ ERROR: Check Logs";
        else if (hoist.getState() == STATE_MOVING_UP) statusStr = "⬆️ Moving Up...";
        else if (hoist.getState() == STATE_MOVING_DOWN) statusStr = "⬇️ Moving Down...";
        else if (hoist.getState() == STATE_CALIBRATING) statusStr = "🔄 Calibrating...";
        updateAppStatus(statusStr.c_str());

        // C. APP 图表数据更新 (非 Demo 模式下正常推送)
        if (!isDemoPlaying) {
             updateAppMaintenanceData(maintenance.getLastRunDuration(), maintenance.calculateSlope());
        }

        lastLog = millis();
    }

    // 5. 串口指令控制 (调试神器)
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
            case 'D': // [New] Demo Mode
                Serial.println(">>> Starting Demo Mode: Generating Data...");
                maintenance.generateDemoData();
                isDemoPlaying = true;
                demoPlayIndex = 0;
                break;
            case 'x': 
                // 模拟一个异常长的运行 (调试用)
                Serial.println("Simulating jammed run...");
                // 实际很难直接注入，只能依赖手动不按开关让它超时
                break; 
            default: Serial.printf("Unknown command: %c\n", cmd); break;
        }
    }
}
