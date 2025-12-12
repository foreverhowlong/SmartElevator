#ifndef SCHEDULER_MANAGER_H
#define SCHEDULER_MANAGER_H

#include <Arduino.h>
#include <time.h>

class SchedulerManager {
private:
    long scheduleUpSeconds = -1;   // -1 means disabled
    long scheduleDownSeconds = -1; // -1 means disabled
    
    // Helper to get current seconds since midnight
    long getCurrentSecondsOfDay() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return -1;
        }
        return (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;
    }

public:
    void begin() {
        // Init NTP (China Pool)
        configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org", "time.nist.gov");
        Serial.println("[Scheduler] NTP Initialized.");
    }

    void setScheduleUp(long seconds) {
        scheduleUpSeconds = seconds;
        Serial.printf("[Scheduler] Up Timer set to: %ld s\n", seconds);
    }

    void setScheduleDown(long seconds) {
        scheduleDownSeconds = seconds;
        Serial.printf("[Scheduler] Down Timer set to: %ld s\n", seconds);
    }

    // Returns: 0=None, 1=Trigger Up, 2=Trigger Down
    int checkTrigger() {
        long current = getCurrentSecondsOfDay();
        if (current < 0) return 0; // Time not set yet

        // Simple trigger logic: check if current second matches target
        // To avoid multiple triggers, we could store last triggered day or use a window
        // For MVP, strictly == is fine if loop is fast enough, but a 1-sec tolerance is safer.
        // Actually, Blynk sends seconds.
        
        static long lastCheckedTime = -1;
        if (current == lastCheckedTime) return 0; // Already checked this second
        lastCheckedTime = current;

        if (scheduleUpSeconds != -1 && current == scheduleUpSeconds) {
            return 1;
        }
        
        if (scheduleDownSeconds != -1 && current == scheduleDownSeconds) {
            return 2;
        }

        return 0;
    }
};

#endif
