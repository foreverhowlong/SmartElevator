#ifndef HOIST_STATE_MACHINE_H_
#define HOIST_STATE_MACHINE_H_
#include "Config.h"
#include "hardware_controller.h" // 引入硬件接口
#include <Arduino.h>

// 注意：这里我们不 include blynk_manager.h，避免循环引用。
// 如果状态机需要发数据给Blynk，通常用回调或者简单的全局标志，
// 但为了MVP方便，我们假设 updateAppStatus 在外部有定义或暂时不用。

class HoistStateMachine {
private:
    SystemState _currentState;
    long _currentPositionMs;
    long _targetPositionMs;
    unsigned long _lastUpdateTimestamp;
    unsigned long _runStartTime; // 记录动作开始时间，用于 AI 统计

    // --- 硬件控制封装 (现在调用 HAL 接口) ---
    
    void motorStopWrapper() {
        stopMotor(); // 调用 hardware_controller 的函数
    }

    void motorUpWrapper() {
        // 使用 Config.h 里定义的 PWM 值
        motorGoUp(PWM_SPEED_UP); 
    }

    void motorDownWrapper() {
        motorGoDown(PWM_SPEED_DOWN);
    }

    bool checkTopSensor() {
        return isTopLimitPressed(); // 调用 hardware_controller 的函数
    }

public:
    void begin() {
        _currentState = STATE_POS_UNKNOWN;
        _currentPositionMs = -1;
        _lastUpdateTimestamp = millis();
        motorStopWrapper();
    }

    void update() {
        unsigned long now = millis();
        long deltaTime = now - _lastUpdateTimestamp;
        _lastUpdateTimestamp = now;

        // 1. 全局安全检查：撞顶保护
        // 只有在非下降状态下检测到撞顶，才认为是需要强制停止的紧急情况。
        // 下降时如果撞顶，可能是还没完全离开开关，不应视为紧急停止。
        if (checkTopSensor() && _currentState != STATE_MOVING_DOWN) {
            // 除了 IDLE 和 CALIBRATING (这两种状态下撞顶是正常情况或预期校准完成)
            if (_currentState != STATE_IDLE && _currentState != STATE_CALIBRATING) {
                motorStopWrapper();
                _currentState = STATE_ERROR; // 标记为错误状态，需要人工干预
                Serial.println("⚠️ Limit Hit! Force Stop (Unexpected).");
            }
            _currentPositionMs = 0; // 只要撞顶，物理位置就是0
        }

        // 2. 状态机逻辑
        switch (_currentState) {
            case STATE_POS_UNKNOWN:
                // 等待校准指令，不做任何事
                break;

            case STATE_CALIBRATING:
                if (checkTopSensor()) {
                    motorStopWrapper();
                    _currentState = STATE_IDLE;
                    _currentPositionMs = 0;
                    Serial.println("✅ Calibration Done.");
                } else {
                    motorUpWrapper();
                }
                break;

            case STATE_MOVING_DOWN:
                // 软限位：到了虚拟底部？
                if (_currentPositionMs >= TIME_TO_BOTTOM_MS) {
                    motorStopWrapper();
                    _currentState = STATE_IDLE;
                    Serial.println("🛑 Virtual Bottom Reached.");
                } 
                // 到了目标？
                else if (_currentPositionMs >= _targetPositionMs) {
                    motorStopWrapper();
                    _currentState = STATE_IDLE;
                    Serial.println("✅ Target Reached (Down).");
                } 
                else {
                    motorDownWrapper();
                    _currentPositionMs += deltaTime; // 积分
                }
                break;

            case STATE_MOVING_UP:
                // 到了目标？
                if (_currentPositionMs <= _targetPositionMs) {
                    motorStopWrapper();
                    _currentState = STATE_IDLE;
                    Serial.println("✅ Target Reached (Up).");
                } 
                else {
                    motorUpWrapper();
                    _currentPositionMs -= deltaTime; // 积分
                    if (_currentPositionMs < 0) _currentPositionMs = 0;
                }
                break;

            case STATE_IDLE:
            case STATE_ERROR:
                motorStopWrapper();
                break;
        }
    }

    // --- 指令接口 ---

    void commandGoTop() {
        _targetPositionMs = 0;
        _currentState = STATE_CALIBRATING; 
        Serial.println("CMD: Calibrating (Go Top)...");
    }

    void commandGoMiddle() {
        if (_currentState == STATE_POS_UNKNOWN) return;
        _targetPositionMs = TIME_TO_MIDDLE_MS;
        decideDirection();
    }

    void commandGoBottom() {
        if (_currentState == STATE_POS_UNKNOWN) return;
        _targetPositionMs = TIME_TO_BOTTOM_MS;
        decideDirection();
    }
    
    void emergencyStop() {
        _currentState = STATE_ERROR;
        motorStopWrapper();
    }

    // --- 辅助方法 ---

    void decideDirection() {
        long diff = _targetPositionMs - _currentPositionMs;
        if (abs(diff) < 200) {
            _currentState = STATE_IDLE;
        } else if (diff > 0) {
            _currentState = STATE_MOVING_DOWN;
        } else {
            _currentState = STATE_MOVING_UP;
        }
    }

    const char* getStateName() {
        switch(_currentState) {
            case STATE_IDLE: return "IDLE";
            case STATE_MOVING_UP: return "UP";
            case STATE_MOVING_DOWN: return "DOWN";
            case STATE_POS_UNKNOWN: return "UNKNOWN";
            case STATE_CALIBRATING: return "CALIB";
            case STATE_ERROR: return "ERROR";
            default: return "???";
        }
    }
    
    long getCurrentPosition() { return _currentPositionMs; }
};
#endif HOIST_STATE_MACHINE_H_