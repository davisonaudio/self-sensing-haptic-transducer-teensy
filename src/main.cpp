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
AudioInputUSB            usb_in;           //xy=331,160
AudioInputI2SQuad        i2s_quad_in;      //xy=336,370
AudioRecordQueue         queue_inR;         //xy=487,179
AudioRecordQueue         queue_inL;         //xy=488,146
AudioPlayQueue           queue_outR;         //xy=653,182
AudioPlayQueue           queue_outL;         //xy=654,147
AudioOutputUSB           usb_out;           //xy=758,386
AudioOutputI2S           i2s_out;           //xy=790,159
AudioConnection          patchCord1(usb_in, 0, queue_inL, 0);
AudioConnection          patchCord2(usb_in, 1, queue_inR, 0);
AudioConnection          patchCord3(i2s_quad_in, 2, usb_out, 0);
AudioConnection          patchCord4(i2s_quad_in, 3, usb_out, 1);
AudioConnection          patchCord5(queue_outR, 0, i2s_out, 1);
AudioConnection          patchCord6(queue_outL, 0, i2s_out, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=494,473
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
    queue_inL.begin();
    queue_inR.begin();
}


int curSample = 0;
int curBlock = 0;

double buf_L[CIRCULAR_BUFFER_LENGTH] = {0.0};
double buf_R[CIRCULAR_BUFFER_LENGTH] = {0.0};

void loop() {
    short *bp_L, *bp_R;

    // Wait for left and right input channels to have content
    while (!queue_inL.available() && !queue_inR.available());

    bp_L = queue_inL.readBuffer();
    bp_R = queue_inR.readBuffer();

    int startIndex = curBlock * AUDIO_BLOCK_SAMPLES;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        curSample = startIndex + i;
        buf_L[curSample] = (double)bp_L[i];
        buf_R[curSample] = (double)bp_R[i];
    }

    queue_inL.freeBuffer();
    queue_inR.freeBuffer();

    // Get pointers to "empty" output buffers
    bp_L = queue_outL.getBuffer();
    bp_R = queue_outR.getBuffer();

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        bp_L[i] = (short)buf_L[i];
        bp_R[i] = (short)buf_R[i];
        curBlock++;
        curBlock &= MAX_SAMPLE_BLOCKS;

        // and play them back into the audio queues
        queue_outL.playBuffer();
        queue_outR.playBuffer();
    }
}