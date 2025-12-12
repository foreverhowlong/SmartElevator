#ifndef MAINTENANCE_MANAGER_H
#define MAINTENANCE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

// Max history size for long-term analysis
#define MAX_HISTORY_SIZE 10
// NVS Namespace
#define PREF_NAMESPACE "smart_elevator"

class MaintenanceManager {
private:
    Preferences prefs;
    long history[MAX_HISTORY_SIZE];
    int historyIndex = 0;
    int historyCount = 0;

    // Baseline for short-term check (Standard Full Rise Time)
    // In a real scenario, this might be dynamic. For now, we use the config max time.
    const long BASELINE_DURATION = TIME_TO_BOTTOM_MS; 
    const float ACUTE_THRESHOLD_RATIO = 1.3f; // +30%

public:
    void begin() {
        prefs.begin(PREF_NAMESPACE, false);
        // Load history count and index if needed, or just start fresh/circular in RAM
        // For simplicity and robustness, we can just load/save the array index.
        // But for MVP, let's keep it in RAM and maybe load only necessary stats.
        // Actually directive says: "Must store history... to Flash".
        
        historyIndex = prefs.getInt("h_idx", 0);
        historyCount = prefs.getInt("h_cnt", 0);
        
        // Load array
        if (historyCount > 0) {
            prefs.getBytes("history", history, sizeof(history));
        } else {
            memset(history, 0, sizeof(history));
        }
        
        Serial.println("[Maintenance] System Initialized.");
        Serial.printf("[Maintenance] History Count: %d\n", historyCount);
    }

    /**
     * @brief Record a run duration into history and NVS
     * @param durationMs Time taken to reach top
     */
    void recordRun(long durationMs) {
        history[historyIndex] = durationMs;
        historyIndex = (historyIndex + 1) % MAX_HISTORY_SIZE;
        if (historyCount < MAX_HISTORY_SIZE) historyCount++;

        // Save to NVS
        prefs.putInt("h_idx", historyIndex);
        prefs.putInt("h_cnt", historyCount);
        prefs.putBytes("history", history, sizeof(history));
        
        Serial.printf("[Maintenance] Recorded Run: %ld ms. History Size: %d\n", durationMs, historyCount);
    }

    /**
     * @brief Short-term check: Is the current run taking too long?
     * @param currentDurationMs Current elapsed time of the movement
     * @return true if anomalous (jammed), false otherwise
     */
    bool checkAcuteAnomaly(long currentDurationMs) {
        long threshold = BASELINE_DURATION * ACUTE_THRESHOLD_RATIO;
        if (currentDurationMs > threshold) {
            return true;
        }
        return false;
    }

    /**
     * @brief Long-term check: Calculate Linear Regression Slope
     * @return Slope value (ms per run). >0 means getting slower.
     */
    double calculateSlope() {
        if (historyCount < 2) return 0.0;

        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        int n = historyCount;

        // X axis is simply 0, 1, 2... (Run index)
        // Y axis is Duration (history value)
        // We need to iterate chronologically.
        // The oldest item is at (historyIndex - historyCount + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE?
        // Simpler: Just treat the buffer as the dataset regardless of order? 
        // No, order matters for trend.
        
        int startIdx = (historyCount < MAX_HISTORY_SIZE) ? 0 : historyIndex; 
        // If full, historyIndex is the insertion point for NEXT, so it's also the Oldest item.
        
        for (int i = 0; i < n; i++) {
            int bufferIdx = (startIdx + i) % MAX_HISTORY_SIZE;
            long y = history[bufferIdx];
            double x = i; // 0 is oldest, n-1 is newest

            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
        }

        double numerator = (n * sumXY) - (sumX * sumY);
        double denominator = (n * sumX2) - (sumX * sumX);

        if (denominator == 0) return 0.0;
        
        return numerator / denominator;
    }

    long getLastRunDuration() {
        if (historyCount == 0) return 0;
        // historyIndex points to the NEXT slot, so (historyIndex - 1) is the latest.
        int lastIdx = (historyIndex - 1 + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE;
        return history[lastIdx];
    }

    // --- Demo / Presentation Features ---
    
    // Temporary buffer to hold the pre-generated demo scenario
    long demoBuffer[MAX_HISTORY_SIZE];

    /**
     * @brief Prepares the demo scenario but DOES NOT write to main history yet.
     * This allows us to "inject" data one by one during replay to simulate real-time updates.
     */
    void generateDemoData() {
        // Base: 8000ms, increasing by ~80ms each run with noise.
        long base = BASELINE_DURATION; 
        
        Serial.println("[Maintenance] Generating Demo Scenario (in buffer)...");
        for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
            // Trend: i * 80ms
            // Noise: random(-20, 20)
            demoBuffer[i] = base + (i * 80) + random(-20, 21);
        }
        Serial.println("[Maintenance] Demo Scenario Ready. Waiting for replay injection.");
    }
    
    /**
     * @brief Clears the real history. Call this before starting demo replay.
     */
    void resetHistory() {
        historyCount = 0;
        historyIndex = 0;
        // Optional: clear NVS if you want persistence to be wiped too
        // prefs.putInt("h_idx", 0); ...
    }

    /**
     * @brief Injects a single point from the demo buffer into the real history.
     * @param index The index from the demo buffer [0..9]
     * @return The value that was injected
     */
    long injectDemoData(int index) {
        if (index < 0 || index >= MAX_HISTORY_SIZE) return 0;
        long val = demoBuffer[index];
        recordRun(val); // Actually write to history and NVS
        return val;
    }

    /**
     * @brief Access history items safely for visualization replay
     */
    int getHistoryCount() { return historyCount; }
    
    // getHistoryItem is no longer needed for Scheme B because we use the return value of injectDemoData
    // but we keep it compatible if needed.
    long getHistoryItem(int i) {
        // Legacy support if needed, or mapping logic
        return 0; 
    }
};

#endif
