#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Simulation variables
int currentSpeed = 0;
bool speedUp = true;
int currentGear = 0; // 0=P, 1=R, 2=N, 3=D
unsigned long lastGearChange = 0;

int leftBarValue = 0;
int rightBarValue = 99;
bool leftBarUp = true;

// ----------------------------------------------------
// RENDERING FUNCTIONS
// ----------------------------------------------------

// Helper: Radial arc drawer for the speedometer
void drawSpeedoArc(int centerX, int centerY, int innerRadius, int outerRadius, float progress) {
    float startAngle = 3.66; // 210 degrees
    float endAngle = -0.52;  // -30 degrees
    float currentAngle = startAngle - (startAngle - endAngle) * progress;

    for (float a = startAngle; a >= currentAngle; a -= 0.05) {
        int x0 = centerX + cos(a) * innerRadius;
        int y0 = centerY - sin(a) * innerRadius;
        int x1 = centerX + cos(a) * outerRadius;
        int y1 = centerY - sin(a) * outerRadius;
        display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
    }
}

// 1. Left Section: Mode label and progress bar (0 to 100)
void drawLeftComponent(const char* modeText, int barValue) {
    // Header text
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 2);
    display.print(modeText);

    // Frame rectangle: X=0 to X=42, Height=10 (Y=22 to Y=32)
    display.drawRect(0, 22, 42, 10, SSD1306_WHITE);

    // Fill from right to left
    int fillWidth = map(barValue, 0, 100, 0, 38);
    if (fillWidth > 0) {
        display.fillRect(41 - fillWidth, 24, fillWidth, 6, SSD1306_WHITE);
    }
}

// 2. Center Section: Speedometer arc and numerical value
void drawCenterComponent(int speed) {
    int centerX = 64;
    int centerY = 24;
    float progress = (float)speed / 260.0; // Normalized 0.0 to 1.0

    // Draw gauge arc
    drawSpeedoArc(centerX, centerY, 18, 21, progress);

    // Speed digits positioning
    display.setTextSize(1);
    if (speed < 10) display.setCursor(61, 10);
    else if (speed < 100) display.setCursor(58, 10);
    else display.setCursor(55, 10);
    display.print(speed);

    // "mph" label
    display.setCursor(55, 20);
    display.print("mph");
}

// 3. Right Section: PRND Selector and right progress bar (0 to 100)
void drawRightComponent(int gearIdx, int barValue) {
    // PRND Text
    display.setTextSize(1);
    display.setCursor(98, 10);
    display.print("PRND");

    // Indicator arrow above selected gear
    int arrowXPositions[] = {100, 106, 112, 118};
    int arrX = arrowXPositions[gearIdx];
    display.drawTriangle(arrX, 2, arrX - 2, 0, arrX + 2, 0, SSD1306_WHITE);
    display.drawFastVLine(arrX, 2, 2, SSD1306_WHITE);

    // Frame rectangle: X=86 to X=128, Height=10 (Y=22 to Y=32)
    display.drawRect(86, 22, 42, 10, SSD1306_WHITE);

    // Fill from right to left
    int fillWidth = map(barValue, 0, 100, 0, 38);
    if (fillWidth > 0) {
        display.fillRect(126 - fillWidth, 24, fillWidth, 6, SSD1306_WHITE);
    }
}

// ----------------------------------------------------
// MAIN ARDUINO CORE FUNCTIONS
// ----------------------------------------------------

void setup() {
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        for(;;);
    }
    display.clearDisplay();
}

void loop() {
    display.clearDisplay();

    // Render components using isolated functions with passed parameters
    drawLeftComponent("4H", leftBarValue);
    drawCenterComponent(currentSpeed);
    drawRightComponent(currentGear, rightBarValue);

    display.display();

    // ----------------------------------------------------
    // DATA SIMULATION LOGIC
    // ----------------------------------------------------
    // Speed simulation
    if (speedUp) {
        currentSpeed += 2;
        if (currentSpeed >= 260) speedUp = false;
    } else {
        currentSpeed -= 3;
        if (currentSpeed <= 0) speedUp = true;
    }

    // Left bar simulation
    if (leftBarUp) {
        leftBarValue += 2;
        if (leftBarValue >= 100) leftBarUp = false;
    } else {
        leftBarValue -= 2;
        if (leftBarValue <= 0) leftBarUp = true;
    }

    // Right bar simulation (inversely proportional to speed)
    rightBarValue = map(currentSpeed, 0, 260, 99, 5);

    // Gear switching simulation every 2 seconds
    if (millis() - lastGearChange > 2000) {
        currentGear = (currentGear + 1) % 4;
        lastGearChange = millis();
    }

    delay(15);
}