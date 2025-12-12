#ifndef BLYNK_MANAGER_H
#define BLYNK_MANAGER_H

/**
 * @file blynk_manager.h
 * @brief 网络与云平台管理模块 (MVP Version)
 * @details 负责 Wi-Fi 连接、Blynk 协议握手，以及将 APP 指令转发给状态机。
 */

#include "secrets.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "HoistStateMachine.h" 
#include "SchedulerManager.h"
#include "MaintenanceManager.h"

// 引用主程序中定义的全局对象
extern HoistStateMachine hoist; 
extern SchedulerManager scheduler;
extern MaintenanceManager maintenance;

// 定义 Blynk 的打印输出为串口
#define BLYNK_PRINT Serial

// ------------------------------------
// 1. 连接管理
// ------------------------------------

void setupBlynk() {
    Serial.println("\n[Network] Connecting to WiFi & Blynk...");
    // 阻塞式连接，连接失败会一直卡在这里（MVP策略）
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
    Serial.println("[Network] Connected!");
}

void runBlynk() {
    Blynk.run();
}

// ------------------------------------
// 2. 指令回调 (App -> Device)
// ------------------------------------

// V0: 紧急停止 (最高优先级)
BLYNK_WRITE(V1) {
    int val = param.asInt();
    if (val == 1) {
        Serial.println("[Blynk] 🚨 EMERGENCY STOP Triggered!");
        hoist.emergencyStop();
    }
}

// V20: 楼层选择 (综合控制)
// 0=无, 1=底, 2=中, 3=顶
BLYNK_WRITE(V20) {
    int floor = param.asInt();
    Serial.printf("[Blynk] Floor Select: %d\n", floor);
    switch (floor) {
        case 1: hoist.commandGoBottom(); break;
        case 2: hoist.commandGoMiddle(); break;
        case 3: hoist.commandGoTop(); break;
        default: break;
    }
}

// V21: 去底层
BLYNK_WRITE(V21) {
    if (param.asInt() == 1) {
        Serial.println("[Blynk] CMD: Go Bottom");
        hoist.commandGoBottom();
    }
}

// V22: 去中层
BLYNK_WRITE(V22) {
    if (param.asInt() == 1) {
        Serial.println("[Blynk] CMD: Go Middle");
        hoist.commandGoMiddle();
    }
}

// V23: 去顶层 (校准)
BLYNK_WRITE(V23) {
    if (param.asInt() == 1) {
        Serial.println("[Blynk] CMD: Go Top");
        hoist.commandGoTop();
    }
}

// V10: 定时上升 (Time Input widget sends seconds)
BLYNK_WRITE(V10) {
    long startTimeInSecs = param[0].asLong();
    scheduler.setScheduleUp(startTimeInSecs);
}

// V11: 定时下降 (Time Input widget sends seconds)
BLYNK_WRITE(V11) {
    long startTimeInSecs = param[0].asLong();
    scheduler.setScheduleDown(startTimeInSecs);
}

// ------------------------------------
// 3. 状态推送 (Device -> App)
// ------------------------------------

// 辅助函数：更新APP上的状态文字
void updateAppStatus(const char* statusStr) {
    Blynk.virtualWrite(V3, statusStr);
}

// 辅助函数：更新维护数据 (AI 数据)
void updateAppMaintenanceData(long lastDurationMs, double slope) {
    Blynk.virtualWrite(V0, (int)lastDurationMs); // 单次耗时
    Blynk.virtualWrite(V4, slope);              // 老化斜率
}

#endif