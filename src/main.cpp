#include <Arduino.h>
#include <i2c_device.h>
#include "max98389.h"
#include <Audio.h>

#define CIRCULAR_BUFFER_LENGTH AUDIO_BLOCK_SAMPLES*4
#define MAX_SAMPLE_BLOCKS (CIRCULAR_BUFFER_LENGTH / AUDIO_BLOCK_SAMPLES)-1
//Audio library macro, AUDIO_BLOCK_SAMPLES = 128

// GUItool: begin automatically generated code
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputUSB            usb1;           //xy=331,160
AudioInputI2SQuad        i2s_quad1;      //xy=335,380
AudioRecordQueue         queue3;         //xy=487,179
AudioRecordQueue         queue1;         //xy=488,146
AudioPlayQueue           queue4;         //xy=653,182
AudioPlayQueue           queue2;         //xy=654,147
AudioOutputUSB           usb2;           //xy=758,386
AudioOutputI2S           i2s2;           //xy=775,154
AudioConnection          patchCord1(usb1, 0, queue1, 0);
AudioConnection          patchCord2(usb1, 1, queue3, 0);
AudioConnection          patchCord3(i2s_quad1, 2, usb2, 0);
AudioConnection          patchCord4(i2s_quad1, 3, usb2, 1);
AudioConnection          patchCord5(queue4, 0, i2s2, 1);
AudioConnection          patchCord6(queue2, 0, i2s2, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=494,473
// GUItool: end automatically generated code


// GUItool: end automatically generated code


bool configured = false;
IntervalTimer myTimer;
int ledState = LOW;

void blinkLED() {
  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }
  digitalWrite(LED_BUILTIN, ledState);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    myTimer.begin(blinkLED, 1000000);  // blinkLED to run every 1 seconds

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("Started");
    max98389 max;
    max.master.set_internal_pullups(InternalPullup::disabled);
    max.begin(400 * 1000U);
    // Check that we can see the sensor and configure it.
    configured = max.configure();
    if (configured) {
        Serial.println("Configured");
    } else {
        Serial.println("Not configured");
    }
    AudioMemory(128);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.5);
    queue1.begin();
    queue3.begin();
}


int curSample = 0;
int curBlock = 0;

double buf_L[CIRCULAR_BUFFER_LENGTH] = {0.0};
double buf_R[CIRCULAR_BUFFER_LENGTH] = {0.0};

void loop() {
    short *bp_L, *bp_R;

    // Wait for left and right input channels to have content
    while (!queue1.available() && !queue3.available());

    bp_L = queue1.readBuffer();
    bp_R = queue3.readBuffer();

    int startIndex = curBlock * AUDIO_BLOCK_SAMPLES;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        curSample = startIndex + i;
        buf_L[curSample] = (double)bp_L[i];
        buf_R[curSample] = (double)bp_R[i];
    }

    queue1.freeBuffer();
    queue3.freeBuffer();

    // Get pointers to "empty" output buffers
    bp_L = queue2.getBuffer();
    bp_R = queue4.getBuffer();

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        bp_L[i] = (short)buf_L[i];
        bp_R[i] = (short)buf_R[i];
        curBlock++;
        curBlock &= MAX_SAMPLE_BLOCKS;

        // and play them back into the audio queues
        queue2.playBuffer();
        queue4.playBuffer();
    }
}