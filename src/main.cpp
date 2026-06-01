#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The OLED reset pin is not used in this setup (-1)
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C ///< 0x3C is common for 128x32 OLEDs

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Animation variables
int boxX = 10;
int boxY = 10;
int boxSize = 8;
int moveSpeed = 2;

void setup() {
    Serial.begin(115200);

    // Initialize I2C on RP2040-Zero (GP0 = SDA, GP1 = SCL)
    // Default Wire pins on Pico core are usually GP4/GP5, so we force GP0/GP1
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    // Initialize the OLED display
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    // Clear the buffer and show initial screen
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("RP2040-Zero Ready!");
    display.display();
    delay(1500);
}

void loop() {
    display.clearDisplay();

    // Draw static header
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Demo Target: ");
    display.print(boxX);

    // Draw animated bouncing box
    display.fillRect(boxX, boxY, boxSize, boxSize, SSD1306_WHITE);

    // Draw a static frame border
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

    // Render buffer to screen
    display.display();

    // Update animation state
    boxX += moveSpeed;

    // Bounce off the screen edges
    if (boxX >= (SCREEN_WIDTH - boxSize - 2) || boxX <= 2) {
        moveSpeed = -moveSpeed;
    }

    // Maintain stable frame rate (~60 FPS target)
    delay(32);
}