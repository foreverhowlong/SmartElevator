#ifndef HOIST_STATE_MACHINE_H_
#define HOIST_STATE_MACHINE_H_
#include "Config.h"
#include "hardware_controller.h" // 引入硬件接口
#include "MaintenanceManager.h"  // 引入维护管理器
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
    bool _isFullRunMeasuring;    // 标记是否为“全程运行”（从底到顶），只有这种情况才记录数据
    
    MaintenanceManager* _maintenanceMgr = nullptr; // 维护管理器指针

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
    void bindMaintenanceManager(MaintenanceManager* mgr) {
        _maintenanceMgr = mgr;
    }

    void begin() {
        _currentState = STATE_POS_UNKNOWN;
        _currentPositionMs = -1;
        _isFullRunMeasuring = false;
        _lastUpdateTimestamp = millis();
        motorStopWrapper();
    }

    void update() {
        unsigned long now = millis();
        long deltaTime = now - _lastUpdateTimestamp;
        _lastUpdateTimestamp = now;

        // 1. 全局安全检查：撞顶保护
        // 只有在非下降状态下检测到撞顶，才认为是需要强制停止的紧急情况。
        if (checkTopSensor() && _currentState != STATE_MOVING_DOWN) {
            // 除了 IDLE(这种状态下撞顶是正常情况)
            if (_currentState != STATE_IDLE ) {
                motorStopWrapper();
                _currentState = STATE_ERROR; // 标记为错误状态，需要人工干预
                
                static unsigned long lastErrorPrintTime = 0;
                if (millis() - lastErrorPrintTime > 1000) {
                    Serial.println("⚠️ Limit Hit! Force Stop (Unexpected).");
                    lastErrorPrintTime = millis();
                }
            }
            _currentPositionMs = 0; // 只要撞顶，物理位置就是0
        }

        // 2. 状态机逻辑
        switch (_currentState) {
            case STATE_POS_UNKNOWN:
                // 等待校准指令，不做任何事
                break;

            case STATE_CALIBRATING:
                // Safety: Calibration Timeout
                if (now - _runStartTime > MAX_SAFE_POSITION_MS) {
                     motorStopWrapper();
                     _currentState = STATE_ERROR;
                     Serial.println("⚠️ Calibration Timeout! Sensor failure likely. Force Stop.");
                     return;
                }

                // 维护检查：短期异常 (Acute Check) - 仅限全程运行
                if (_maintenanceMgr && _isFullRunMeasuring) {
                   long runDuration = now - _runStartTime;
                   if (_maintenanceMgr->checkAcuteAnomaly(runDuration)) {
                       motorStopWrapper();
                       _currentState = STATE_ERROR;
                       Serial.printf("⚠️ Acute Anomaly! Duration: %ld ms. Force Stop.\n", runDuration);
                       return;
                   }
                }

                if (checkTopSensor()) {
                    motorStopWrapper();
                    
                    // 记录运行数据 (仅在全程且成功时)
                    if (_maintenanceMgr && _isFullRunMeasuring) {
                        long duration = now - _runStartTime;
                        _maintenanceMgr->recordRun(duration);
                        Serial.printf("📊 Maintenance: Full Run recorded (%ld ms)\n", duration);
                    } else if (_isFullRunMeasuring) {
                        // 理论上不会进这里，除非逻辑有误
                    } else {
                        Serial.println("ℹ️ Calibration Done (Partial run, no stats recorded).");
                    }

                    _isFullRunMeasuring = false; // 结束测量
                    _currentState = STATE_IDLE;
                    _currentPositionMs = 0;
                } else {
                    motorUpWrapper();
                }
                break;

            case STATE_MOVING_DOWN:
                // Safety: Max Position Limit
                if (_currentPositionMs >= (long)MAX_SAFE_POSITION_MS) {
                    motorStopWrapper();
                    _currentState = STATE_ERROR;
                    Serial.println("⚠️ Max Safe Position Exceeded! Force Stop.");
                    return;
                }

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
                // 维护检查：短期异常 (Acute Check) - 仅限全程运行
                if (_maintenanceMgr && _isFullRunMeasuring) {
                   long runDuration = now - _runStartTime;
                   if (_maintenanceMgr->checkAcuteAnomaly(runDuration)) {
                       motorStopWrapper();
                       _currentState = STATE_ERROR;
                       Serial.printf("⚠️ Acute Anomaly! Duration: %ld ms. Force Stop.\n", runDuration);
                       return;
                   }
                }

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
        _runStartTime = millis(); // Always reset start time for safety timeout check
        
        // 逻辑修正：只在从底部出发时，才开始计时统计
        // 判断当前是否在底部 (允许 500ms 误差)
        if (_currentPositionMs >= (TIME_TO_BOTTOM_MS - 500)) {
            _isFullRunMeasuring = true;
            Serial.println("CMD: Go Top (FULL RUN - Stats Enabled)");
        } else {
            _isFullRunMeasuring = false;
            Serial.println("CMD: Go Top (Partial Run - Stats Ignored)");
        }
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
    
    SystemState getState() {
        return _currentState;
    }

    // --- 辅助方法 ---

    void decideDirection() {
        // 普通移动指令不参与全程统计
        _isFullRunMeasuring = false;

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