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

// Demo state variables
int speedVal = 0;
bool speedUp = true;
int gearIndex = 0; // 0=P, 1=R, 2=N, 3=D
unsigned long lastGearChange = 0;

// Progress bars state
int leftBarVal = 0;
int rightBarVal = 99;
bool leftBarUp = true;

void setup() {
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        for(;;);
    }
    display.clearDisplay();
}

// Helper function to draw the speedometer arc using polar coordinates
void drawSpeedoArc(int centerX, int centerY, int innerRadius, int outerRadius, float progress) {
    // Progress is a float from 0.0 to 1.0
    // The arc spans from ~150 degrees to ~30 degrees (in standard math terms)
    // Let's map 0.0-1.0 to angles from 3.66 rad (210°) to -0.52 rad (-30°) counter-clockwise
    float startAngle = 3.49; // ~200 degrees
    float endAngle = -0.35;  // -20 degrees
    float currentAngle = startAngle - (startAngle - endAngle) * progress;

    // Draw the arc lines
    for (float a = startAngle; a >= currentAngle; a -= 0.05) {
        int x0 = centerX + cos(a) * innerRadius;
        int y0 = centerY - sin(a) * innerRadius; // Invert Y for screen coordinates
        int x1 = centerX + cos(a) * outerRadius;
        int y1 = centerY - sin(a) * outerRadius;
        display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
    }
}

void loop() {
    display.clearDisplay();

    // ----------------------------------------------------
    // 1. LEFT SECTION: "4H" Mode & Left Progress Bar
    // ----------------------------------------------------
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(24, 2);
    display.print("4H");

    // Left progress bar container (slanted look approximation)
    display.drawLine(10, 24, 55, 20, SSD1306_WHITE);
    display.drawLine(10, 24, 15, 30, SSD1306_WHITE);
    display.drawLine(15, 30, 55, 30, SSD1306_WHITE);

    // Fill left progress bar (grows from right to left as requested)
    int leftBarWidth = map(leftBarVal, 0, 100, 0, 40);
    if (leftBarWidth > 0) {
        for(int i = 0; i < leftBarWidth; i++) {
            int currX = 55 - i;
            // Linear interpolation for slanted height top boundary
            int topY = 20 + (55 - currX) * 4 / 45;
            display.drawFastVLine(currX, topY, 30 - topY, SSD1306_WHITE);
        }
    }

    // ----------------------------------------------------
    // 2. CENTER SECTION: Speedometer Arc & Digits
    // ----------------------------------------------------
    int centerX = 64;
    int centerY = 34; // Center is pushed down off-screen to make a nice top arc
    float speedProgress = (float)speedVal / 260.0;

    // Draw the animated arc (double layer for thickness)
    drawSpeedoArc(centerX, centerY, 20, 24, speedProgress);

    // Speed value text (Scaled x2 for prominent look)
    display.setTextSize(2);
    if (speedVal < 10) display.setCursor(58, 2);
    else if (speedVal < 100) display.setCursor(52, 2);
    else display.setCursor(46, 2);
    display.print(speedVal);

    // "mph" label
    display.setTextSize(1);
    display.setCursor(55, 18);
    display.print("mph");

    // ----------------------------------------------------
    // 3. RIGHT SECTION: PRND Gear Selector & Right Bar
    // ----------------------------------------------------
    display.setTextSize(1);
    display.setCursor(92, 10);
    display.print("PRND");

    // Jumping arrow indicator above PRND
    int arrowXPositions[] = {94, 100, 106, 112}; // Aligning with P, R, N, D
    int arrX = arrowXPositions[gearIndex];
    display.drawTriangle(arrX, 2, arrX - 2, 0, arrX + 2, 0, SSD1306_WHITE);
    display.drawFastVLine(arrX, 2, 3, SSD1306_WHITE);

    // Right progress bar container (slanted on the right)
    display.drawLine(75, 20, 118, 24, SSD1306_WHITE);
    display.drawLine(118, 24, 123, 30, SSD1306_WHITE);
    display.drawLine(75, 30, 123, 30, SSD1306_WHITE);

    // Right numeric value (e.g., 99)
    display.setCursor(110, 20);
    display.print(rightBarVal);

    // Fill right progress bar (grows/shrinks from right to left)
    int rightBarWidth = map(rightBarVal, 0, 100, 0, 35);
    if (rightBarWidth > 0) {
        for(int i = 0; i < rightBarWidth; i++) {
            int currX = 110 - i; // Fills from right side boundary inwards
            if (currX < 75) break;
            int topY = 20 + (currX - 75) * 4 / 43;
            display.drawFastVLine(currX, topY, 30 - topY, SSD1306_WHITE);
        }
    }

    // Render everything to the physical OLED panel
    display.display();

    // ----------------------------------------------------
    // 4. ANIMATION LOGIC (Data Simulation)
    // ----------------------------------------------------
    // Fake speed acceleration/deceleration
    if (speedUp) {
        speedVal += 3;
        if (speedVal >= 260) speedUp = false;
    } else {
        speedVal -= 4;
        if (speedVal <= 0) speedUp = true;
    }

    // Fake left progress bar bouncing
    if (leftBarUp) {
        leftBarVal += 2;
        if (leftBarVal >= 100) leftBarUp = false;
    } else {
        leftBarVal -= 2;
        if (leftBarVal <= 0) leftBarUp = true;
    }

    // Fake right progress bar (tied backwards to speed for dynamic look)
    rightBarVal = map(speedVal, 0, 260, 99, 10);

    // Cycle through PRND every 2.5 seconds
    if (millis() - lastGearChange > 2500) {
        gearIndex = (gearIndex + 1) % 4;
        lastGearChange = millis();
    }

    delay(10); // Smooth frame update pacing
}