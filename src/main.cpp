#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

constexpr int kScreenWidth = 128;
constexpr int kScreenHeight = 32;
constexpr int kOledReset = -1;
constexpr uint8_t kScreenAddress = 0x3C;
constexpr int kMaxSpeed = 260;
constexpr int kLeftBarMin = 0;
constexpr int kLeftBarMax = 100;
constexpr int kRightBarMin = 5;
constexpr int kRightBarMax = 99;
constexpr unsigned long kGearChangeIntervalMs = 3000;
constexpr unsigned long kSignalChangeIntervalMs = 4000;
constexpr unsigned long kBlinkIntervalMs = 400;

Adafruit_SSD1306 display(kScreenWidth, kScreenHeight, &Wire, kOledReset);

enum class TurnSignalState : uint8_t {
    Off = 0,
    Left = 1,
    Right = 2,
    Hazard = 3,
};

enum class GearPosition : uint8_t {
    Park = 0,
    Reverse = 1,
    Neutral = 2,
    Drive = 3,
};

int currentSpeed = 0;
bool speedUp = true;
GearPosition currentGear = GearPosition::Park;
unsigned long lastGearChange = 0;

int leftBarValue = 0;
int rightBarValue = kRightBarMax;
bool leftBarUp = true;

TurnSignalState turnSignalState = TurnSignalState::Off;
unsigned long lastSignalChange = 0;

template <typename T>
T clampValue(T value, T minValue, T maxValue) {
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

int mapClamped(int x, int inMin, int inMax, int outMin, int outMax) {
    return map(clampValue(x, inMin, inMax), inMin, inMax, outMin, outMax);
}

// ----------------------------------------------------
// RENDERING FUNCTIONS
// ----------------------------------------------------

void drawSpeedoArc(int centerX, int centerY, int innerRadius, int outerRadius, float progress) {
    constexpr float kStartAngle = 3.66f;
    constexpr float kEndAngle = -0.52f;
    progress = clampValue(progress, 0.0f, 1.0f);
    float currentAngle = kStartAngle - (kStartAngle - kEndAngle) * progress;

    for (float a = kStartAngle; a >= currentAngle; a -= 0.05f) {
        int x0 = centerX + cos(a) * innerRadius;
        int y0 = centerY - sin(a) * innerRadius;
        int x1 = centerX + cos(a) * outerRadius;
        int y1 = centerY - sin(a) * outerRadius;
        display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
    }
}

void drawTurnSignals(TurnSignalState state) {
    bool blinkVisible = ((millis() / kBlinkIntervalMs) % 2) == 0;
    if (state == TurnSignalState::Off || !blinkVisible) {
        return;
    }

    if (state == TurnSignalState::Left || state == TurnSignalState::Hazard) {
        int lx = 38;
        int ly = 3;
        display.drawTriangle(lx, ly, lx + 4, ly - 3, lx + 4, ly + 3, SSD1306_WHITE);
        display.drawFastHLine(lx + 4, ly, 3, SSD1306_WHITE);
    }

    if (state == TurnSignalState::Right || state == TurnSignalState::Hazard) {
        int rx = 86;
        int ry = 3;
        display.drawTriangle(rx, ry, rx - 4, ry - 3, rx - 4, ry + 3, SSD1306_WHITE);
        display.drawFastHLine(rx - 7, ry, 3, SSD1306_WHITE);
    }
}

// 1. Left Section: Mode label and progress bar
void drawProgressBar(int x, int y, int width, int height, int value, bool rightAligned = false) {
    display.drawRect(x, y, width, height, SSD1306_WHITE);
    int fillWidth = mapClamped(value, 0, 100, 0, width - 4);
    if (fillWidth <= 0) {
        return;
    }

    int fillX = rightAligned ? x + width - 2 - fillWidth : x + 2;
    display.fillRect(fillX, y + 2, fillWidth, height - 4, SSD1306_WHITE);
}

void drawLeftComponent(const char* modeText, int barValue) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 2);
    display.print(modeText);
    drawProgressBar(0, 22, 42, 10, barValue, false);
}

// 2. Center Section: Speedometer arc and numerical value
void drawCenterComponent(int speed) {
    int centerX = 64;
    int centerY = 24;
    float progress = static_cast<float>(clampValue(speed, 0, kMaxSpeed)) / kMaxSpeed;

    drawSpeedoArc(centerX, centerY, 18, 21, progress);

    display.setTextSize(1);
    int numberX = speed < 10 ? 61 : (speed < 100 ? 58 : 55);
    display.setCursor(numberX, 10);
    display.print(speed);

    display.setCursor(55, 20);
    display.print("mph");
}

// 3. Right Section: PRND Selector and right progress bar
void drawRightComponent(uint8_t gearIdx, int barValue) {
    static const uint8_t arrowXPositions[] = {100, 106, 112, 118};
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(98, 10);
    display.print("PRND");

    if (gearIdx < 4) {
        int arrX = arrowXPositions[gearIdx];
        display.drawTriangle(arrX, 2, arrX - 2, 0, arrX + 2, 0, SSD1306_WHITE);
        display.drawFastVLine(arrX, 2, 2, SSD1306_WHITE);
    }

    drawProgressBar(86, 22, 42, 10, barValue, true);
}

// ----------------------------------------------------
// MAIN ARDUINO CORE FUNCTIONS
// ----------------------------------------------------

void setup() {
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, kScreenAddress)) {
        while (true) {
            delay(100);
        }
    }
    display.clearDisplay();
}

void loop() {
    unsigned long now = millis();
    display.clearDisplay();

    // Render components
    drawLeftComponent("4H", leftBarValue);
    drawCenterComponent(currentSpeed);
    drawRightComponent(static_cast<uint8_t>(currentGear), rightBarValue);

    // Render the new turn signals component
    drawTurnSignals(turnSignalState);

    display.display();

    // ----------------------------------------------------
    // DATA SIMULATION LOGIC
    // ----------------------------------------------------
    // Speed simulation
    if (speedUp) {
        currentSpeed += 2;
        if (currentSpeed >= kMaxSpeed) {
            currentSpeed = kMaxSpeed;
            speedUp = false;
        }
    } else {
        currentSpeed -= 3;
        if (currentSpeed <= 0) {
            currentSpeed = 0;
            speedUp = true;
        }
    }

    // Bars simulation
    if (leftBarUp) {
        leftBarValue += 2;
        if (leftBarValue >= kLeftBarMax) {
            leftBarValue = kLeftBarMax;
            leftBarUp = false;
        }
    } else {
        leftBarValue -= 2;
        if (leftBarValue <= kLeftBarMin) {
            leftBarValue = kLeftBarMin;
            leftBarUp = true;
        }
    }
    rightBarValue = mapClamped(currentSpeed, 0, kMaxSpeed, kRightBarMax, kRightBarMin);

    // Gear switching simulation (every 3 seconds)
    if (now - lastGearChange >= kGearChangeIntervalMs) {
        currentGear = static_cast<GearPosition>((static_cast<uint8_t>(currentGear) + 1) % 4);
        lastGearChange = now;
    }

    // Turn signals simulation: cycle through states every 4 seconds
    // 0: Off -> 1: Left -> 2: Right -> 3: Hazard
    if (now - lastSignalChange >= kSignalChangeIntervalMs) {
        turnSignalState = static_cast<TurnSignalState>((static_cast<uint8_t>(turnSignalState) + 1) % 4);
        lastSignalChange = now;
    }

    delay(15);
}