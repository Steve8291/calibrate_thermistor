#include <Arduino.h>
#include <algorithm>
#include <AiEsp32RotaryEncoder.h>   // https://github.com/igorantolic/ai-esp32-rotary-encoder
#include <OneWire.h>                // https://github.com/PaulStoffregen/OneWire
// Reduce code needed for DallasTemperature.h
#define REQUIRESNEW false
#define REQUIRESALARMS false
#include <DallasTemperature.h>      // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <sdios.h>                  // Header from SdFat repo
#include <SdFat.h>                  // https://github.com/greiman/SdFat
#include <SPI.h>                    // From ArduinoCore-avr library
#include <VectorStats.h>            // https://github.com/Steve8291/VectorStats
#include <MillisChronoTimer.h>      // https://github.com/Steve8291/MillisChronoTimer
#include "preferences.h"
#define FILE_TRUNC_WRITE (O_WRITE | O_CREAT | O_TRUNC | O_AT_END)

// Set max_buffer_size to largest value in buffer_sizes array.
const int buffer_array_length = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);
const int max_buffer_size = *std::max_element(buffer_sizes, buffer_sizes + buffer_array_length);

const int TEMP_CONVERSION_TIME = 750;  // Wait time for DS18B20 to convert temp. 12-bit resolution.

int sample_size;  // Holds current sample_size
int buffer_index;  // Holds index of sample_size selected from buffer_sizes[]
bool fetching_ADC_data = false;
bool std_dev_ready = false;

enum class ButtonSelect {
    SAMPLE_SIZE,
    PRINT_BUFFER,
    TEST_MODE,
    RECORD_DATA,
    STANDBY_MODE
};

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ENCODER_B_PIN, ENCODER_A_PIN, ENCODER_BUTTON_PIN, ENCODER_VCC_PIN, ENCODER_STEPS);
ButtonSelect button_select = ButtonSelect::STANDBY_MODE;
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress calibration_probe_addr;  // Array to hold the calibration probe address.
SdFs SD;  // FAT16/FAT32/exFAT filesystems
FsFile dataFile;
MillisChronoTimer data_interval_timer(data_interval);
MillisChronoTimer temp_request_timer(TEMP_CONVERSION_TIME);
MillisChronoTimer end_temp_timer(END_TEMP_TIME);
VectorStats<int16_t> ADC_probe(max_buffer_size);  // Holds analog readings from ADC
VectorStats<int16_t> std_dev_buffer_mdn(std_dev_sample_size);  // Holds median values from ADC_probe.
VectorStats<int16_t> std_dev_buffer_avg(std_dev_sample_size);  // Holds average values from ADC_probe.

void IRAM_ATTR readEncoderISR() {
    rotaryEncoder.readEncoder_ISR();
}

// Set initial sample size lowest value.
void setInitialSampleSize() {
    std::sort(buffer_sizes, buffer_sizes + buffer_array_length);
    buffer_index = 0;
    sample_size = buffer_sizes[buffer_index];
    ADC_probe.resize(sample_size);
}


void resetBuffers() {
    ADC_probe.zeroBuffer();
    std_dev_buffer_mdn.zeroBuffer();
    std_dev_buffer_avg.zeroBuffer();
    fetching_ADC_data = false;
    std_dev_ready = false;
}

void readRotaryEncoder() {
    // .encoderChanged only triggers if encoder rotates and gets a different value.
    if (rotaryEncoder.encoderChanged()) {
        if (button_select != ButtonSelect::RECORD_DATA && button_select != ButtonSelect::STANDBY_MODE) {
            buffer_index = rotaryEncoder.readEncoder();
            sample_size = buffer_sizes[buffer_index];
            ADC_probe.resize(buffer_sizes[buffer_index]);
            Serial.print("SAMPLE_SIZE: ");
            Serial.println(sample_size);
            resetBuffers();
        }
    }
}


void handleRotaryButton() {
    if (rotaryEncoder.isEncoderButtonClicked()) {
        int button_code = static_cast<int>(button_select);
        if (button_code < static_cast<int>(ButtonSelect::STANDBY_MODE)) {
            button_select = static_cast<ButtonSelect>(button_code + 1);
        } else button_select = static_cast<ButtonSelect>(0);
    }
}

void runPrintBuffer() {
    resetBuffers();

    while (button_select == ButtonSelect::PRINT_BUFFER){
        readRotaryEncoder();
        handleRotaryButton();
        if (data_interval_timer.expired()) {
            ADC_probe.add(analogRead(THERMISTOR_INPUT_PIN));
        }

        if (ADC_probe.bufferFull()) {
                for (int i = 0; i < ADC_probe.size(); ++i) {
                    Serial.print(ADC_probe.getElement(i));
                    Serial.print(" ");
                    readRotaryEncoder();
                    handleRotaryButton();
                }
            
            Serial.print("\nSAMPLE_SIZE: ");
            Serial.println(sample_size);
            Serial.println();
            data_interval_timer.reset();
            ADC_probe.setBufferFullFalse();
        }
    }
}

// End RECORD_DATA if END_TEMP is reached for length of end_temp_timer.
void checkCompletion(float tempF) {
    if (tempF > END_TEMP) {
        end_temp_timer.reset();
    } else if (end_temp_timer.expired()) {
        dataFile.close();
        Serial.println("Data Collection Completed!!!");
        button_select = ButtonSelect::STANDBY_MODE;
    }
}

float calculateTemp(int ADC_raw) {
    int x = ADC_raw;
    float cal_tempF;
    // Note: a lower value corresponds to higher temp.
    if (ADC_raw <= upper_cutoff) {
        cal_tempF = uA+(uB*x)+(uC*pow(x,2))+(uD*pow(x,3));
    } else {
        cal_tempF = A+(B*x)+(C*pow(x,2))+(D*pow(x,3));
    }
    return cal_tempF;
}


void runRecordData() {
    uint32_t run_count = 0;  // Approx up to 39480
    if (dataFile) {
        resetBuffers();
        dataFile.truncate();
        dataFile.println("\"Data Set: ADC Reading\",\"Data Set: Temp F\"");
        Serial.println("\nRECORD_DATA");
    } else {
        Serial.println("\n!!!!!!!!!!!!!! Error opening .csv file !!!!!!!!!!!!!!!!!!");
        button_select = ButtonSelect::STANDBY_MODE;
    }

    while (button_select == ButtonSelect::RECORD_DATA) {

        if (data_interval_timer.expired()) {
            data_interval_timer.reset();
            sensors.requestTemperatures();  // Request new async temp reading from 1-wire sensor
            fetching_ADC_data = true;
        }

        if (fetching_ADC_data) {
            ADC_probe.add(analogRead(THERMISTOR_INPUT_PIN));
        }

        if (ADC_probe.bufferFull()) {
            fetching_ADC_data = false;
            run_count++;
            int raw;
            if (use_median) {
                raw = ADC_probe.getMedian();
            } else {
                raw = round(ADC_probe.getAverage());
            }
            float tempF = static_cast<float>(DEVICE_DISCONNECTED_F);
            while (data_interval_timer.elapsed()< TEMP_CONVERSION_TIME) {
                // Wait for conversion to be ready.
                handleRotaryButton();
            }
            tempF = sensors.getTempF(calibration_probe_addr);
            if (tempF == static_cast<float>(DEVICE_DISCONNECTED_F)) {
                Serial.println("Error: Could not read temp data from 1-wire sensor!");
                button_select = ButtonSelect::STANDBY_MODE;
            } else {
                Serial.print("SampleSize: ");
                Serial.print(sample_size);
                if (use_median) {
                    Serial.print("   Median: ");
                } else {
                    Serial.print("   Average: ");
                }
                Serial.print(raw);
                Serial.print("   TempF: ");
                Serial.print(tempF, 4);
                Serial.print("   EndTemp: ");
                Serial.print(END_TEMP);
                Serial.print("   count: ");
                Serial.print(run_count);
                Serial.println();
        
                // Save to dataFile
                dataFile.print(raw);
                dataFile.print(",");
                dataFile.println(tempF, 4); // DS18B20 has resolution of 0.1125°F
                checkCompletion(tempF);  // Exits to STANDBY_MODE if done.

                if (data_interval_timer.expired()) {
                    Serial.println("WARNING: Data Collection taking longer than data_interval.");
                    Serial.println("Either decrease SAMPLE_SIZE or increase data_interval.");
                    button_select = ButtonSelect::STANDBY_MODE;
                }
            }
        }
        handleRotaryButton();
    }
}


void runSampleSize() {
    resetBuffers();

    while (button_select == ButtonSelect::SAMPLE_SIZE){
        if (data_interval_timer.expired()) {
            fetching_ADC_data = true;
            data_interval_timer.reset();
            if (std_dev_buffer_mdn.bufferFull()) {
                std_dev_ready = true;  // Keeps true after first full.
            }
        }

        if (fetching_ADC_data) {
            ADC_probe.add(analogRead(THERMISTOR_INPUT_PIN));
        }

        if (ADC_probe.bufferFull()) {
            fetching_ADC_data = false;
            float slope = ADC_probe.getSlope();
            int median = ADC_probe.getMedian();
            int average = static_cast<int>(std::round(ADC_probe.getAverage()));
            std_dev_buffer_mdn.add(median);
            std_dev_buffer_avg.add(average);
            Serial.print("SAMPLE_SIZE: ");
            Serial.print(sample_size);

            Serial.print("   Median: ");
            Serial.print(median);
            Serial.print("   Average: ");
            Serial.print(average);
            Serial.print("   Time(ms): ");
            Serial.print(data_interval_timer.elapsed());
            if (slope < 0) {
                Serial.print("   Slope: ");
            } else {
                Serial.print("   Slope:  ");
            }
            Serial.print(slope);
            
            if (std_dev_ready) {
                Serial.print("\t\tMedianStdDev: ");
                Serial.print(std_dev_buffer_mdn.getStdDev());
                Serial.print("\t\tAverageStdDev: ");
                Serial.print(std_dev_buffer_avg.getStdDev());
            }

            Serial.println("\n");

            if (data_interval_timer.expired()) {
                Serial.println("WARNING: Data Collection taking longer than data_interval.");
                Serial.println("Either decrease SAMPLE_SIZE or increase data_interval.");
            }
            
        }
        readRotaryEncoder();
        handleRotaryButton();
    }
}

void runTestMode() {
    resetBuffers();
    Serial.println("\nTEST_MODE");

    while (button_select == ButtonSelect::TEST_MODE) {
        if (data_interval_timer.expired()) {
            data_interval_timer.reset();
            sensors.requestTemperatures();  // Request new async temp reading from 1-wire sensor
            fetching_ADC_data = true;
        }

        if (fetching_ADC_data) {
            ADC_probe.add(analogRead(THERMISTOR_INPUT_PIN));
        }

        if (ADC_probe.bufferFull()) {
            fetching_ADC_data = false;
            float slope = ADC_probe.getSlope();
            int raw;
            if (use_median) {
                raw = ADC_probe.getMedian();
            } else {
                raw = round(ADC_probe.getAverage());
            }
            float ADC_tempF = calculateTemp(raw);
            float tempF = static_cast<float>(DEVICE_DISCONNECTED_F);
            while (data_interval_timer.elapsed()< TEMP_CONVERSION_TIME) {
                // Wait for conversion to be ready.
                handleRotaryButton();
                readRotaryEncoder();
            }
            tempF = sensors.getTempF(calibration_probe_addr);
            if (tempF == static_cast<float>(DEVICE_DISCONNECTED_F)) {
                Serial.println("Error: Could not read temp data from 1-wire sensor!");
                button_select = ButtonSelect::STANDBY_MODE;
            } else {
                Serial.print("SampleSize: ");
                Serial.print(sample_size);
                if (use_median) {
                    Serial.print("   Median: ");
                } else {
                    Serial.print("   Average: ");
                }
                Serial.print(raw);
                Serial.print("   ADC_TempF: ");
                Serial.print(ADC_tempF);
                Serial.print("   TempF: ");
                Serial.print(tempF, 4);
                if (slope < 0) {
                    Serial.print("   Slope: ");
                } else {
                    Serial.print("   Slope:  ");
                }
                Serial.print(slope);
                Serial.println();
        
                if (data_interval_timer.expired()) {
                    Serial.println("WARNING: Data Collection taking longer than data_interval.");
                    Serial.println("Either decrease SAMPLE_SIZE or increase data_interval.");
                    button_select = ButtonSelect::STANDBY_MODE;
                }
            }
        }
        handleRotaryButton();
        readRotaryEncoder();
    }
}


void setup() {
    Serial.begin(BAUD_RATE);
    delay(2000);

    pinMode(THERMISTOR_INPUT_PIN, INPUT);

    // Initialize Rotary Encoder
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);

    setInitialSampleSize();  // For ADC readings

    int last_index = buffer_array_length - 1;
    rotaryEncoder.setBoundaries(0, last_index, false); // minValue, maxValue, circleValues
    rotaryEncoder.setEncoderValue(buffer_index);

    /*
    *  Setup DS18B20 temperature sensor.
    *  Using a device address to get temp reading is faster.
    *  Store the hex address of sensor in calibration_probe_add at index 0.
    */
    sensors.begin();
    if (!sensors.getAddress(calibration_probe_addr, 0)) {
        Serial.println("Unable to find address for Device 0");
    }
    sensors.setWaitForConversion(false);  // makes it async

    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
    }

    dataFile = SD.open(FILE_NAME, FILE_TRUNC_WRITE);  // Will overwrite contents of file.
}

void loop() {
    handleRotaryButton();
    readRotaryEncoder();

    switch (button_select) {
        case ButtonSelect::SAMPLE_SIZE:
            runSampleSize();
            break;
        case ButtonSelect::PRINT_BUFFER:
            runPrintBuffer();
            break;
        case ButtonSelect::TEST_MODE:
            runTestMode();
            break;
        case ButtonSelect::RECORD_DATA:
            runRecordData();
            break;
        case ButtonSelect::STANDBY_MODE:
            Serial.println("\nSTANDBY_MODE");
            while(button_select == ButtonSelect::STANDBY_MODE) {
                handleRotaryButton();
            }
            break;
    }
}


