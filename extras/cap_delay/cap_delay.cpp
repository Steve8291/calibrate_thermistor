#include <Arduino.h>
#include <algorithm>
#include <AiEsp32RotaryEncoder.h>   // https://github.com/igorantolic/ai-esp32-rotary-encoder
#include <VectorStats.h>            // https://github.com/Steve8291/VectorStats
#include <MillisChronoTimer.h>      // https://github.com/Steve8291/MillisChronoTimer

/*
 * After much work on this program I came to realize that the effects of thermistor self-heating
 * on an ESP32 were far less of a problem than the voltage variations encountered when using 
 * a GPIO pin to power the thermistor. Using the dedicated 3.3v pin allows the thermistor 
 * to be powered directly from the voltage regulator and seems to provide much more stable readings.
 * Turning on and off the capacitor would probably be a useful approach if you are using a 
 * separate voltage source. 
*/

/* Standard Deviations to Count for Skew
 * Counts the number of initial values in the buffer that are outside of the normal data trend.
 * You can think of this value as way to control sensitivity of outlier collection.
 *   1 = Will possibly count an occasional good value as an outlier.
 *   2 = Default value.
 *   3 = Probably going to start ignoring bad data and show zero outliers.
*/
const uint8_t skew_deviations = 2;

// Number of samples to take. The default below is more than sufficient.
const int sample_size = 511;

/* Delay time in milliseconds to wait between sample collections.
 * Provides cooldown time for thermistor.
*/
const int data_interval = 1000;

// Baud rate for serial communication.
const unsigned long BAUD_RATE = 115200;

// GPIO pin for thermistor input
const int THERMISTOR_INPUT_PIN = GPIO_NUM_4;
const int THERMISTOR_POWER_PIN = GPIO_NUM_3;

// Rotary Encoder Settings:
const int ENCODER_A_PIN = GPIO_NUM_14;       // CLK
const int ENCODER_B_PIN = GPIO_NUM_15;       // DT
const int ENCODER_BUTTON_PIN = GPIO_NUM_16;  // SW
const int ENCODER_VCC_PIN = -1;              // VCC Connected to 3.3V
const int ENCODER_STEPS = 4;                 // If encoder not working correctly try (1, 2, 3, 4)

// SD card chip select pin for SdFat library
const uint8_t SD_CS_PIN = SS;

int cap_time_delay = 0; // Holds selected time in ms to charge bypass capacitor
bool fetching_ADC_data = false;
bool print_buffer = false;


AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ENCODER_B_PIN, ENCODER_A_PIN, ENCODER_BUTTON_PIN, ENCODER_VCC_PIN, ENCODER_STEPS);
MillisChronoTimer cap_charge_timer(cap_time_delay);
MillisChronoTimer data_interval_timer(data_interval);
VectorStats<int16_t> ADC_probe(sample_size);  // Holds analog readings from ADC

void IRAM_ATTR readEncoderISR() {
    rotaryEncoder.readEncoder_ISR();
}


void resetBuffers() {
    ADC_probe.zeroBuffer();
    data_interval_timer.reset();
    digitalWrite(THERMISTOR_POWER_PIN, LOW);
    fetching_ADC_data = false;
}


void readRotaryEncoder() {
    // .encoderChanged only triggers if encoder rotates and gets a different value.
    if (rotaryEncoder.encoderChanged()) {
        int value = rotaryEncoder.readEncoder();
        cap_time_delay = value;
        cap_charge_timer.modify(cap_time_delay);
        Serial.print("CAP_DELAY: ");
        Serial.println(value);
        resetBuffers();
    }
}


// Button only toggles printing of buffer elements.
void handleRotaryButton() {
    if (rotaryEncoder.isEncoderButtonClicked()) {
        print_buffer = !print_buffer;
    }
}


void printBuffer() {
    Serial.print("Samples: ");
    for (int i = 0; i < ADC_probe.size(); ++i) {
        Serial.print(ADC_probe.getElement(i));
        Serial.print(" ");
    }
    Serial.println("\n");
}


void setup() {
    Serial.begin(BAUD_RATE);
    pinMode(THERMISTOR_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_POWER_PIN, OUTPUT);

    // Initialize Rotary Encoder
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.setBoundaries(0, 500, false); // minValue, maxValue, circleValues
    rotaryEncoder.setEncoderValue(cap_time_delay);  // Time in ms to charge bypass capacitor
    resetBuffers();

}


void loop() {
    if (data_interval_timer.expired()) {
        digitalWrite(THERMISTOR_POWER_PIN, HIGH);
        cap_charge_timer.reset();
        data_interval_timer.reset();
        fetching_ADC_data = true;
    }

    if (cap_charge_timer.expired() && fetching_ADC_data) {
        ADC_probe.add(analogRead(THERMISTOR_INPUT_PIN));
    }

    if (ADC_probe.bufferFull()) {
        digitalWrite(THERMISTOR_POWER_PIN, LOW);
        fetching_ADC_data = false;
        float slope = ADC_probe.getSlope();
        int skew = ADC_probe.getLeftSkew(skew_deviations);
        if (print_buffer) {
            printBuffer();
        }
        Serial.print("CAP_DELAY: ");
        Serial.print(cap_time_delay);
        if (slope < 0) {
            Serial.print("   Slope: ");
        } else {
            Serial.print("   Slope:  ");
        }
        Serial.print(slope);
        if (skew < 0) {
            Serial.print("   Skew: ");
        } else {
            Serial.print("   Skew:  ");
        }
        Serial.print(skew);
        Serial.print("   StdDev: ");
        Serial.print(ADC_probe.getStdDev());
        Serial.print("   Median: ");
        Serial.print(ADC_probe.getMedian());
        Serial.print("   Time(ms): ");
        Serial.print(data_interval_timer.elapsed());
        Serial.println();

        if (data_interval_timer.expired()) {
            Serial.println("WARNING: Data Collection taking longer than data_interval.");
            Serial.println("Increase data_interval if you need a longer cap delay.");
        }
    }
    readRotaryEncoder();
    handleRotaryButton();
}


