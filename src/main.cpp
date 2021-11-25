#include <Arduino.h>
#include <FastLED.h>
#define DATA_PIN 9
#define CLOCK_PIN 21
#define BLINKER_LEFT_PIN 51
#define BLINKER_RIGHT_PIN 50
#define BRAKE_PIN 52
#define NUM_LEDS 31
#define COLOR_ORDER BGR
#define GLOBAL_BRIGHTNESS 255
#define FRAME_TIME 8
#define BLINKER_WIDTH 15
#define CENTER_POSITION NUM_LEDS-1
#define NUMBER_OF_GLIDERS 200 
#define BLINKER_PULSE_COOLDOWN 500

CRGB leds[NUM_LEDS];
uint32_t prevFrameTime;
uint32_t startupAnimationProgress;
int32_t leftBlinkerPulseCooldown;
int32_t rightBlinkerPulseCooldown;
uint8_t glidersIndex = 0;

enum GliderType {
    None,
    Brake,
    Blinker
};

struct Glider {
    uint8_t position;
    GliderType type;
};

Glider gliders [NUMBER_OF_GLIDERS] = { }; // TODO: Figure out a reasonable max limit?

// the setup routine runs once when you press reset:
void setup() {
    Serial.begin(9600);
    FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);

    pinMode(50, INPUT);

    startupAnimationProgress = 0;
    leftBlinkerPulseCooldown = 0;
    rightBlinkerPulseCooldown = 0;

    // Clear LEDs
    FastLED.clear();
    FastLED.show();

    // TODO: Remove this when done, this is just to test the startup animation
    delay(200);

    for(uint32_t i = 0; i < NUMBER_OF_GLIDERS; i++) { 
        Glider nullGlider { .position = 0, .type = None };
        gliders[i] = nullGlider;
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
void updateBrakeGliders(uint32_t deltaTime, bool reverse) {
    CRGB colors [NUM_LEDS];
    for (int i = 0; i < NUM_LEDS; i++) {
        colors[i] = CRGB::Black;
    }
    CRGB color = CRGB::Red;
    CRGB fadedColor = CRGB(128, 0, 0); //color.fadeToBlackBy(100);
    for (int i = 0; i < NUMBER_OF_GLIDERS; i++) {
        Glider glider = gliders[i];
        if (glider.type != Brake) {
            continue;
        }

        if (glider.position > (NUM_LEDS-1)*2) { continue; }
        if (glider.position < 0) { continue; }
        if (glider.position == CENTER_POSITION && reverse) { 
            gliders[i].type = None;
        }

        //if (reverse && led == centerLed) {
        //    led = NUM_LEDS+1;
        //    continue;
        //}
        //

        if (glider.position % 2 == 0) {
            colors[glider.position/2] = color;
        } else {
            uint8_t pos = glider.position/2;
            if (colors[pos].getLuma() == 0) {
                colors[pos] = fadedColor;
            }

            if (colors[pos+1].getLuma() == 0) {
                colors[pos+1] = fadedColor;
            }
        }

        uint8_t speed = 1;
        if (glider.position > CENTER_POSITION) { 
            gliders[i].position += (reverse ? -1 : 1) * speed;
        } else if (glider.position < CENTER_POSITION) {
            gliders[i].position += (reverse ? 1 : -1) * speed;
        }
    }

    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        if (colors[i].getLuma() == 0) { continue; }
        leds[i] = colors[i];
    }
}

#define BLINKERS_LIGHT_ANIMATION_SPEED 10
void updateBlinkerGliders(uint32_t deltaTime) {
    CRGB colors [NUM_LEDS];
    for (int i = 0; i < NUM_LEDS; i++) {
        colors[i] = CRGB::Black;
    }
    CRGB color = CRGB::OrangeRed;
    CRGB fadedColor = color.fadeToBlackBy(100);
    for (int i = 0; i < NUMBER_OF_GLIDERS; i++) {
        Glider glider = gliders[i];
        if (glider.type != Blinker) {
            continue;
        }

        if (glider.position < 0 || glider.position > (NUM_LEDS-1)*2) { 
            gliders[i].type = None;
            continue; 
        }

        if (glider.position % 2 == 0) {
            colors[glider.position/2] = color;
        } else {
            uint8_t pos = glider.position/2;
            if (colors[pos].getLuma() == 0) {
                colors[pos] = fadedColor;
            }

            if (colors[pos+1].getLuma() == 0) {
                colors[pos+1] = fadedColor;
            }
        }

        uint8_t speed = 1;
        if (glider.position > CENTER_POSITION) { 
            gliders[i].position += (1 * speed);
        } else if (glider.position < CENTER_POSITION) {
            gliders[i].position += (-1 * speed);
        }
    }

    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        if (colors[i].getLuma() == 0) { continue; }
        leds[i] = colors[i];
    }
}

void spawnGlider(uint8_t position, GliderType type) {
    Glider newGlider { .position = position, .type = type };
    gliders[glidersIndex] = newGlider;
    glidersIndex++;

    if (glidersIndex > NUMBER_OF_GLIDERS-1) {
        glidersIndex = 0;
    }
}

// the loop routine runs over and over again forever:
void loop() {
    uint32_t deltaTime = millis() - prevFrameTime;
    Serial.println(1000 / deltaTime);

    FastLED.setBrightness(GLOBAL_BRIGHTNESS);

    startupAnimation(deltaTime);

    if (!digitalRead(BRAKE_PIN)) {
        spawnGlider(CENTER_POSITION-1, Brake);
        spawnGlider(CENTER_POSITION, Brake);
        spawnGlider(CENTER_POSITION+1, Brake);
        updateBrakeGliders(deltaTime, false);
    } else {
        updateBrakeGliders(deltaTime, true);
    }




    CRGB blinkerColor = CRGB::OrangeRed;

    if (!digitalRead(BLINKER_LEFT_PIN) && leftBlinkerPulseCooldown <= 0) {
        leftBlinkerPulseCooldown = BLINKER_PULSE_COOLDOWN;
    }

    if (!digitalRead(BLINKER_RIGHT_PIN) && rightBlinkerPulseCooldown <= 0) {
        rightBlinkerPulseCooldown = BLINKER_PULSE_COOLDOWN;
    }
    if (leftBlinkerPulseCooldown > 0) { leftBlinkerPulseCooldown -= deltaTime; }
    if (rightBlinkerPulseCooldown > 0) { rightBlinkerPulseCooldown -= deltaTime; }

    if (leftBlinkerPulseCooldown > BLINKER_PULSE_COOLDOWN * 0.8) {
        spawnGlider(BLINKER_WIDTH, Blinker);
    }

    if (rightBlinkerPulseCooldown > BLINKER_PULSE_COOLDOWN * 0.8) {
        spawnGlider(((NUM_LEDS-1)*2)-BLINKER_WIDTH, Blinker);
    }

    if (rightBlinkerPulseCooldown > 0) {
        uint8_t maxPos = (NUM_LEDS-1)*2;
        for (uint8_t i = (maxPos-BLINKER_WIDTH); i <= maxPos; i++) {
            leds[i/2] = CRGB::OrangeRed;
            leds[i/2].fadeToBlackBy(250);
        }
    }

    if (leftBlinkerPulseCooldown > 0) {
        for (uint8_t i = 0; i < (BLINKER_WIDTH/2)+2; i++) {
            leds[i] = CRGB::OrangeRed;
            leds[i].fadeToBlackBy(250);
        }
    }

    updateBlinkerGliders(deltaTime);

    FastLED.show(); 

    prevFrameTime = millis();
    delay(FRAME_TIME); 
}
