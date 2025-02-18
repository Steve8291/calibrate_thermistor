# Calibrate-Thermistor
I wrote this to help calibrate a thermistor for my hot tub. So the goal was to find the smallest buffer that would provide the smoothest data so as to not have my heater continually turning on and off. The thermistor I was using did not have a data sheet and I wanted to get the most accurate temperatures possible. Using a custom calibration curve provides a much more accurate temperature than the Steinhart-Hart, β model or other methods.

This program is designed to do 3 things:
1) Aid in selecting an appropriate sized buffer for oversampling.
2) Determine whether using median or average on that buffer provides the smoothest data.
3) Record data points for creating a custom temperature curve that can be used in a cubic equation. Extremely accurate callibration!

I was using an ESP32 board but this should work equally well on any Arduino, etc. Below you will find my notes on what I discoverd and how to use this program.

## Use 3.3V on Arduino
When using an Arduino you have to option of using a different reference voltage. By using the 3.3V output instead of default 5V as a reference voltage your readings will be much less noisy. To do this you need to connect the AREF pin to the 3.3V pin and add the following line of code:
```cpp
void setup() {
  analogReference(EXTERNAL);
}
```

## Choosing A Fixed Resistor For The Voltage Divider
Thermistors experience self-heating as current flows through them. The fixed resistor in your circuit helps with some of this heat dissipation.

- To effectively limit self-heating, the fixed resistor should have a similar resistance to the thermistor at the operating temperature range, ensuring a balanced voltage division.
- Use a multimeter to measure the resistance of your isolated thermistor in the middle of the operating temp range.
  - Example: For my hot tub I dropped the waterproof thermistor in at operating temp and connected the two leads to a multimeter (15kΩ).
  While this is more towards the upper end of what I might need to measure, it is the temp I care most about.

## Smoothing Readings With A Bypass Capacitor
Use a low value capacitor to create a low pass filter and reject high frequency noise.

1) A 0.1µF ceramic bypass capacitor is added so that one leg is as close as possible to the pin being used to power the voltage divider. Yes, getting it as close as possible does make a difference! Do some tests or watch a video if you don't believe it.
2) If you are using some sort of a proto-shield or other pcb board for your build, finish all your fabrication now. This will allow for better calibration results.
3) Beware that using a bypass capacitor in your circuit will cause the first raw ADC readings to be inaccurate (low for an NTC thermistor) until the capacitor is fully charged. Although it's unlikely this will matter for most projects, introducing a slight delay will fix your initial readings.

## Limiting Self-heating By Power Cycling The Thermistor
As current flows through a thermistor it heats up and consequently its readings can drift over time. One method of mitigating this effect is to power the thermistor on, take some readings, then power it off to cool.

There is debate about how much of an effect this actually has and it is probably different for different thermistors in different scenarios. This should be fairly simple to handle by powering your thermistor with a GPIO pin as an output. However, if you introduced a capacitor into the circuit, capacitors need time to charge. While cap charge times are usually in the 15-100ms range, this will really mess up your data if you are just turning power on and off to the circuit. Initially I designed this program to handle calculating the length of time a capacitor needed to fully charge "capacitor charge delay." Through testing I found that the slight power fluctuations from the GPIO pin caused more problems than just using the dedicated 3.3v pin. 

The dedicated 3.3V pin is generally going to be more stable than the output on a GPIO pin because the dedicated pin directly draws power from the regulated power supply, while the GPIO pin can experience slight voltage fluctuations depending on the load it's driving, making the dedicated 3.3V pin a better choice for powering sensitive components that require a very stable voltage

If you want to explore this approach, I think the best method would be to use some sort of external regulated power supply that can be switched with a GPIO pin and P-channel MOSFET.

I've included a separate program called `./extras/cap_delay/cap_delay.cpp` that can help determine how long it takes to charge your bypass-capacitor.

### Calculating Capacitor Charge Time Delay
#### Using an online calculator
 https://www.omnicalculator.com/physics/capacitor-charge-time

The above website will give you acceptable results.

1) Enter the resistance of your fixed resistor making sure the units are correct.
2) Enter the Capacitance of your bypass cap 0.1µF (again check units).
3) Scroll to the bottom of the output where it says "Charging Time" and change the units to "ms".
4) This is the minimum value for charge time. To be on the safe side I would suggest doubling it and adding 5ms to that value. Also, round up any decimal value.

#### Using the Cap Delay Program
This will allow you to find that actual charging time for your capacitor in the real world.

1) Make sure your circuit is completely fabricated so that the cap is soldered in place.
2) Set out a thermos of water and let it come to room temperature overnight with the waterproof thermistor in it. I find a large Yeti insulated mug works great. It takes longer than you would think to reach room temp!
3) Wire your circuit as shown in the `./extras/cap_delay/cap_delay_wiring_diagram.png`
4) I find it easiest to start with a high CAP_DELAY value and then decrease it by 5 each time. This allows you to see just how low the Slope value is going to get when the cap is definitely charged. Rotate the rotary encoder to bring the "CAPACITOR_DELAY" up to like 50.
5) Note the Slope value. It should be fluctuating between 0.00 and 0.01 with occasional negative values.
6) Begin slowly decreasing the delay value by about 5 evey few readings. At some point will stop seeing negative values all together. This is important because you should see an occasional negative unless you are always getting a few initial values that are low due to the capacitor not being charged.
7) Also be mindful of the Skew value. If there are values appearing that are less than -1 that's an indication that you are still getting a few initial low values and cap is not charged.
8) Now you can start moving the rotary encoder by steps of 1 to dial in the charge delay. I would add a few milliseconds (maybe 5) to your final value.
9) Pushing the rotary button will cause the entire data set to be printed out if you want to see it.
10) As an example, for a 0.1µF cap I found a value of 14ms gave me consistent Skew of (-1 to 0) and Slope of (-0.00 to 0.01). Adding 6ms I get a safe 20ms delay time. Using the website listed above I get a calculated value of 7.5ms charging time. That's why I recommend doubling that number and maybe even adding a little to it if you use the calculation method.

## Calibrate Thermistor - SAMPLE_SIZE
Use this mode to accomplish two things: determine the optimal buffer size for your analog readings and decide whether to use median or average for oversampling.

### Smoothing data through oversampling:
If you were to plot the data from a thermistor sitting at a stable temp you would see a jagged line as the readings jump above and below the true value. Oversampling is a way to increase the resolution of your data by taking many samples and averaging them. I have found that with thermistors, using the median rather than the average provides a much more stable reading. The median is less sensitive to sporadic spikes or dips when your data contains a significant number of outliers. The average is better suited for data with normally distributed noise where most points are clustered around the center.

For my hot tub controller I use one buffer to take lots of readings and get a median. Then I store the median readings in another, smaller buffer that does a running average on those.

### SAMPLE_SIZE Mode:
When in SAMPLE_SIZE mode this program will allow you to see the effects of increasing and decreasing the sample size. You want to find a sample size that doesn't take up too much memory but still allows you to get enough readings to find a good median or average.
1) Make sure your circuit is completely fabricated so that the cap is soldered in place.
2) Set out a thermos of water and let it come to room temperature overnight with the waterproof thermistor in it. I find a large Yeti insulated mug works great. It takes longer than you would think to reach room temp!
3) Wire your circuit as shown in the `wiring_diagram.png`
4) The program defaults to SAMPLE_SIZE mode but you can also push the rotary button to cycle through the modes.
5) Examine the readings on the lowest sample size. At a very small sample size (default) the slope will have a very high value. Slope is the slope of a line drawn using lineear regression of the data in your buffer. I find it is a great indicator of when you have enough data.
5) Turn the rotary encoder to increase the sample size while allowing enough time for the data to stabilize. You will see the slope eventually reach zero. At this point using a larger buffer is not going to gain you anything. You are oversampling enough. Note: you may see an occasional fluctuation of +/- 00.01 in your slope, the order of your readings is random so that is to be expected.
6) Now look at MedianStdDev and AverageStdDev. These values show how stable your readings are using the two different methods. You can also watch the raw readings of Median and Slope. Whichever method gives you the most consistent readings and lowest standard deviation is the one you want to use in your program.

## Calibrate Thermistor - PRINT_BUFFER
This mode simply prints the buffer each time readings are taken. You need to be in SAMPLE_SIZE or TEST_MODE to change the sample size.

## Calibrate Thermistor - RECORD_DATA
This mode allows you to collect data for creating your own thermistor temperature curves. It is the primary reason for this program. It saves data to an SD card. So your microcontroller will need to have one. It also makes use of a DS18B20 Waterproof Temperature Probe.

This mode uses the previously selected sample size and by default calculates the median. You can change this behavior in the `preferences.h` file. By default, every second this mode will take n samples from the thermistor returning the median. It will then take 1 sample from the DS18B21 1-Wire Probe. These 2 values are saved in a .csv file on the MicroSD card and printed to the serial terminal. Later the data can be analyzed using a graphing program like Vernier LoggerPro. The sample rate is limited primarily by the 750ms rate of the DS18B20.
1) Make sure your circuit is completely fabricated so that the cap is soldered in place.
2) Wire your circuit as shown in the `wiring_diagram.png`
3) Fill a thermos with hot water that is above the max temp you want to be able to measure but not too hot or you will destroy your thermistor. I use my hot tap water just hot enough that I can still hold my fingers in it. Again, I find a large Yeti insulated mug works great.
4) With the program in TEST_MODE, zip-tie your thermistor together with your DS18B21 probe and drop them in the water. If the water is too hot the probe will immediately readout TempF: as an error code. If that happens pull out the probes and let the water cool some.
5) While in TEST_MODE allow the probes to stabilize in the new water temp. You should see stdDev drop to somewhere below 0.75 and TempF stop rising and begin falling.
5) Press the rotary encoder button to switch to RECORD_DATA mode. Data recording will begin immediately. Make sure your computer will not go to sleep.
6) Allow your water to cool down to the END_TEMP in the `preferences.h` file. This can take many hours. And if it can't get down low enough you can pour cold water or an ice cube in to finish data collection.
7) Next you need a graphing program that can apply curved fits to your data. Open the .csv file and delete any data points that look incorrect from the top and bottom of your data line.
8) Apply a best fit curve using a "cubic formula" to the data set. The cubic formula looks like this: `A+Bx+C*x^2+D*x^3`. The variables A, B, C, and D are what you want to find. Then using that formula you can calculate your temperature from the raw median values `x` output from the ADC. Examine the code in this program to see how it works.

## Calibrate Thermistor - TEST_MODE
This mode allow you to test your temperature curves. The values you calculated in your graphing program need to be put into `preferences.h`. If you want the most accurate calculations for a specific temperature range you can create two different curves. For example, say you really care mose about the range between 100-107°F. You could use your graphing software to fit a cubic formula to only the data in that range. Fill those values in for A, B, C, D in `preferences.h`. Then calculate a second fit for the remaining bottom portion of your data and fill that into uA, uB, uC, uD. The value for `upper_cutoff` should be whatever raw value you used to fit the upper range of data. In the example above it would be the raw ADC reading that corresponds to 100°F.