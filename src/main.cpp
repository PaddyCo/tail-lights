#include <Arduino.h>
#include <FastLED.h>
#define DATA_PIN 9
#define CLOCK_PIN 21
#define BLINKER_LEFT_PIN 51
#define BLINKER_RIGHT_PIN 50
#define BRAKE_PIN 52
#define NUM_LEDS 30
#define COLOR_ORDER BGR
#define GLOBAL_BRIGHTNESS 255
#define FRAME_TIME 16

CRGB leds[NUM_LEDS];
uint32_t prevFrameTime;
uint32_t startupAnimationProgress;
uint32_t brakeLightAnimationProgress;
uint8_t brakeLightIndex = 0;
uint8_t brakeLightLed [200] = { }; // TODO: Figure out a reasonable max limit?

// the setup routine runs once when you press reset:
void setup() {
    Serial.begin(9600);
    FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);

    pinMode(50, INPUT);

    brakeLightAnimationProgress = 0;
    startupAnimationProgress = 0;

    // Clear LEDs
    FastLED.clear();
    FastLED.show();

    // TODO: Remove this when done, this is just to test the startup animation
    delay(1000);

    for(int i = 0; i < sizeof(brakeLightLed); i++) { 
        brakeLightLed[i] = NUM_LEDS+1;
    }

    prevFrameTime = millis();
}

#define STARTUP_ANIMATION_SPEED 20
void startupAnimation(uint32_t deltaTime) {
    startupAnimationProgress += deltaTime;

    CRGB color = CRGB::Red;
    uint8_t centerLed = NUM_LEDS/2;

    for(int dot = 0; dot < NUM_LEDS; dot++) { 
        uint8_t offset = abs(centerLed - dot);
        if (offset < startupAnimationProgress/STARTUP_ANIMATION_SPEED) {
            leds[dot] = color; 
            leds[dot].fadeToBlackBy(245);
        }
    }
}

#define BRAKE_LIGHT_ANIMATION_SPEED 10
void brakeLightAnimation(uint32_t deltaTime) {
    uint8_t centerLed = NUM_LEDS/2;
    CRGB color = CRGB::Red;
    for (int i = 0; i < sizeof(brakeLightLed); i++) {
        uint8_t led = brakeLightLed[i];
        if (led < 0 || led > NUM_LEDS) {
            continue;
        }

        leds[led] = color;

        if (led >= centerLed) { 
            brakeLightLed[i] += 1;
        } else if (led < centerLed) {
            brakeLightLed[i] -= 1;
        }
    }
    //if (digitalRead(BRAKE_PIN)) {
    //    brakeLightAnimationProgress = 0;
    //    return;
    //}

    //brakeLightAnimationProgress += deltaTime;

    //CRGB color = CRGB::Red;
    //uint8_t centerLed = NUM_LEDS/2;

    //for(int dot = 0; dot < NUM_LEDS; dot++) { 
    //    uint8_t offset = abs(centerLed - dot);
    //    if (offset < brakeLightAnimationProgress/BRAKE_LIGHT_ANIMATION_SPEED) {
    //        leds[dot] = color; 
    //    }
    //}
}

// the loop routine runs over and over again forever:
void loop() {

    uint32_t deltaTime = millis() - prevFrameTime;

    FastLED.setBrightness(GLOBAL_BRIGHTNESS);

    if (!digitalRead(BRAKE_PIN)) {
        uint8_t centerLed = NUM_LEDS/2;
        
        brakeLightLed[brakeLightIndex] = centerLed;
        brakeLightIndex++;
        brakeLightLed[brakeLightIndex] = centerLed-1;
        brakeLightIndex++;

        if (brakeLightIndex > sizeof(brakeLightLed)-1) {
            brakeLightIndex = 0;
        }
    }

    startupAnimation(deltaTime);
    brakeLightAnimation(deltaTime);
    CRGB blinkerColor = CRGB::Yellow;

    if (!digitalRead(BLINKER_LEFT_PIN)) {
        leds[0] = blinkerColor;
        leds[1] = blinkerColor;
        leds[2] = blinkerColor;
    }

    if (!digitalRead(BLINKER_RIGHT_PIN)) {
        leds[NUM_LEDS-1] = blinkerColor;
        leds[NUM_LEDS-2] = blinkerColor;
        leds[NUM_LEDS-3] = blinkerColor;
    }

    FastLED.show(); 

    prevFrameTime = millis();
    delay(FRAME_TIME); 
}
