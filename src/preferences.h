#ifndef PREFERENCES_H
#define PREFERENCES_H


/* Buffer sizes loosely based on powers of 2
 * The program will cycle through these when turning rotary encoder.
 * You can add or change the values here as needed.
 * Values are intentionally odd-parity to make median calculation faster.
*/
int buffer_sizes[] = {15, 31, 65, 129, 255, 511, 1025, 2049, 4095};


/* How many times the sample_buffer will be filled to get median and average.
 * Only used in SAMPLE_SIZE mode.
 * This is the number of median values that will be used to get std deviation.
 * Also the number of average values that will be used to get std deviation.
 */
const int std_dev_sample_size = 32;


/* Delay time in milliseconds to wait between sample collections.
 * Used in all modes.
 * An interval of 1000ms would give you a data frequency of 1 median and average per second.
 * A 2000ms interval would mean data collection and calculation every 2 seconds.
*/
const int data_interval = 1000;


/* Use median for RECORD_DATA and TEST_MODE.
 * If set to false, average will be used.
*/
const bool use_median = true;


/* Temp (°F) and time (ms) to terminate RECORD_DATA.
 * Exits when END_TEMP or lower is reached for END_TEMP_TIME.
*/
const float END_TEMP = 40.0;
const int END_TEMP_TIME = 30000;  // 30 sec.


/* Cubic Formula Constants for calculating temperatures for thermistor.
 * TempF = A+Bx+C*x^2+D*x^3
   * x = ADC Reading
 * These must be calculated with a separate graphing program.
 * I use "Graphical Analysis" by Vernier.
 * 
*/
const float A = 233.2;
const float B = -0.09784;
const float C = 2.401E-05;
const float D = -3.491E-09;

/* Upper Raw Cutoff Value:
 * Any value at or below this number will use the upper temp formula values.
 * Lower values correspond to higher temps for NTC thermistors.
 * Setting to 0 effectively disables these values.
 * This setting needs to be determined using a separate graphing program.
*/
const int16_t upper_cutoff = 2019;  // Temp 102F

// Upper Temp Formula
const float uA = 233.2;
const float uB = -0.09784;
const float uC = 2.401E-05;
const float uD = -3.491E-09;


// Name of the SD card file to save data to.
const char *FILE_NAME = "probe_calibration.csv";

// Baud rate for serial communication.
const unsigned long BAUD_RATE = 115200;

// GPIO pin for thermistor input
const int THERMISTOR_INPUT_PIN = GPIO_NUM_4;

// GPIO pin for DS18B20 temperature sensor.
const int ONE_WIRE_BUS_PIN = GPIO_NUM_2;

// Rotary Encoder Settings:
const int ENCODER_A_PIN = GPIO_NUM_14;       // CLK
const int ENCODER_B_PIN = GPIO_NUM_15;       // DT
const int ENCODER_BUTTON_PIN = GPIO_NUM_16;  // SW
const int ENCODER_VCC_PIN = -1;              // VCC Connected to 3.3V
const int ENCODER_STEPS = 4;                 // If encoder not working correctly try (1, 2, 3, 4)

// SD card chip select pin for SdFat library
const uint8_t SD_CS_PIN = SS;


#endif // PREFERENCES_H