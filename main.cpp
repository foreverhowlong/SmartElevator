#include "hardware_controller.h"
#include "blynk_manager.h"

// ================= 配置区域 =================

// 关键参数：从"底楼"运行到"正中间"需要的毫秒数
// 你需要拿秒表实测一次：比如从底楼拉到中间需要 12 秒
const long MIDDLE_POSITION_MS = 12000;

// 估算的全程时间 (仅用于虚拟位置计算，不需要特别精确)
const long FULL_HEIGHT_MS = 24000;

// ================= 状态机定义 =================

enum ElevatorTarget {
  TARGET_NONE,
  TARGET_BOTTOM, // 目标：底楼 (1)
  TARGET_MIDDLE, // 目标：中间 (2)
  TARGET_TOP     // 目标：顶楼 (3)
};

enum ElevatorState {
  IDLE,           // 空闲
  MOVING_UP,      // 上升中
  MOVING_DOWN     // 下降中
};

ElevatorState currentState = IDLE;
ElevatorTarget currentTarget = TARGET_NONE;

// 虚拟位置系统 (用于计算何时到达中间)
// 0 = 纯底楼, FULL_HEIGHT_MS = 纯顶楼
long currentPositionMs = 0;
unsigned long lastLoopTime = 0;

void setup() {
  Serial.begin(115200);
  setupHardware();
  setupBlynk();

  Serial.println("=== 智能吊装系统 (三态版) 启动 ===");
  Serial.println("逻辑模式：顶/底靠限位，中间靠时间。");
}

void loop() {
  runBlynk();

  unsigned long now = millis();
  long deltaTime = now - lastLoopTime;
  lastLoopTime = now;

  // --- 1. 虚拟位置更新 (核心) ---
  // 无论我们在干什么，都要实时估算我们在哪，以便"去中间"时能判断停不停

  if (currentState == MOVING_UP) {
    currentPositionMs += deltaTime;
  } else if (currentState == MOVING_DOWN) {
    currentPositionMs -= deltaTime;
  }

  // --- 2. 物理校准 (最高优先级) ---
  // 只要碰到开关，立即修正虚拟位置，消除所有误差

  if (isBottomLimitPressed()) {
    currentPositionMs = 0; // 物理归零
    if (currentTarget == TARGET_BOTTOM || currentState == MOVING_DOWN) {
      stopMotor();
      currentState = IDLE;
      currentTarget = TARGET_NONE;
      Serial.println("✅ 已到达底部 (限位触发)。");
    }
  }

  if (isTopLimitPressed()) {
    currentPositionMs = FULL_HEIGHT_MS; // 物理校准到最大值
    if (currentTarget == TARGET_TOP || currentState == MOVING_UP) {
      stopMotor();
      currentState = IDLE;
      currentTarget = TARGET_NONE;
      Serial.println("✅ 已到达顶部 (限位触发)。");
    }
  }

  // --- 3. 中间楼层停车逻辑 (仅在去中间时生效) ---

  if (currentTarget == TARGET_MIDDLE) {

    // 如果是向上走，且超过了中间线
    if (currentState == MOVING_UP && currentPositionMs >= MIDDLE_POSITION_MS) {
      stopMotor();
      currentState = IDLE;
      currentTarget = TARGET_NONE;
      Serial.println("✅ 已到达中间 (时间控制)。");
    }

    // 如果是向下走，且低于了中间线
    if (currentState == MOVING_DOWN && currentPositionMs <= MIDDLE_POSITION_MS) {
      stopMotor();
      currentState = IDLE;
      currentTarget = TARGET_NONE;
      Serial.println("✅ 已到达中间 (时间控制)。");
    }
  }

  // --- 4. 调试输出 ---
  static unsigned long lastPrint = 0;
  if (now - lastPrint > 2000) {
    lastPrint = now;
    Serial.printf("[状态] 动作:%d 目标:%d | 位置: %ld ms\n", currentState, currentTarget, currentPositionMs);
  }
}

// --- Blynk 交互逻辑 ---

// V20: 三态选择器 (Segmented Switch: 1=底, 2=中, 3=顶)
BLYNK_WRITE(V20) {
  // [修复点] 变量名从 request 改为 target，避免和 Blynk 内部参数重名
  int target = param.asInt();

  // 1. 目标是底部
  if (target == 1) {
    Serial.println("指令：去底部");
    if (!isBottomLimitPressed()) {
      currentTarget = TARGET_BOTTOM;
      currentState = MOVING_DOWN;
      motorGoDown();
    } else {
      Serial.println("错误：本来就在底部");
    }
  }

  // 2. 目标是顶部
  else if (target == 3) {
    Serial.println("指令：去顶部");
    if (!isTopLimitPressed()) {
      currentTarget = TARGET_TOP;
      currentState = MOVING_UP;
      motorGoUp();
    } else {
      Serial.println("错误：本来就在顶部");
    }
  }

  // 3. 目标是中间 (最复杂)
  else if (target == 2) {
    Serial.println("指令：去中间");
    currentTarget = TARGET_MIDDLE;

    // 判断是该往上还是往下
    // 给 2000ms 的容差，防止在这个位置附近反复横跳
    if (abs(currentPositionMs - MIDDLE_POSITION_MS) < 2000) {
      Serial.println("就在中间附近，不动。");
      currentState = IDLE;
      stopMotor();
      return;
    }

    if (currentPositionMs < MIDDLE_POSITION_MS) {
      currentState = MOVING_UP;
      motorGoUp();
    } else {
      currentState = MOVING_DOWN;
      motorGoDown();
    }
  }
}

// V0: 紧急停止
BLYNK_WRITE(V0) {
  if (param.asInt()) {
    stopMotor();
    currentState = IDLE;
    currentTarget = TARGET_NONE;
    Serial.println("!!! 紧急停止 !!!");
  }
}