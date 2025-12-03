#ifndef BLYNK_MANAGER_H
#define BLYNK_MANAGER_H

/**
 * @file blynk_manager.h
 * @brief 网络与云平台管理模块
 * @details 负责处理 Wi-Fi 连接、重连逻辑以及 Blynk 协议的握手。
 */
// 引入隐私配置文件 (包含密码和 Token)
#include "secrets.h"
// 引入这一行是为了使用 Serial 打印调试信息
#include <Arduino.h>

// 定义 Blynk 的打印输出为串口，方便调试
#define BLYNK_PRINT Serial

// 引入 Wi-Fi 和 Blynk 库
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>


/**
 * @brief 初始化网络连接
 * @details 阻塞式连接。连接成功前不会进入主循环。
 */
void setupBlynk() {
    Serial.begin(115200);
    Serial.println("\n[Network] 正在启动...");

    // 开始连接 Blynk (函数内部会自动连接 Wi-Fi)
    // 使用 secrets.h 中的宏定义
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

    Serial.println("[Network] Blynk 连接成功！");
}

/**
 * @brief 维持网络心跳
 * @details 必须在 loop() 中频繁调用。它负责处理接收到的指令和发送数据。
 */
void runBlynk() {
    Blynk.run();
}

#endif