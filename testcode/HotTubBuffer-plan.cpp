/* Hot Tub Buffers Plan:
A new median will be calculated every second by collecting 511 samples (non-blocking)
Medians will be stored in a new buffer that holds 300 data points. (5 min worth of data)
Every 20 sec an average will be taken of the median buffer and a temperature calculated.
*/ 


// Buffer size for data collection.
int sample_size = 511;

// Calculate median at 1 second intervals by filling sample_size buffer.
int sample_interval = 1;
int data_collection_timer = sample_interval * 1000;

// Store 5 min of median values in median_buffer_size.
int avg_minutes = 5;
int median_buffer_size = avg_minutes * 60

// get temperature by averaging median buffer every 20 seconds.
int temp_interval = 20;

// !!!!! PLEASE REMEMBER THAT TRYING TO USE CAP DELAY CAUSES PROBLEMS. IT DOES NOT SEEM TO STABILIZE FOR MORE THAN 0.5 SECONDS.
// SELF-HEATING IS PROBABLY LESS OF A PROBLEM THAN TRYING TO STABILIZE THE CIRCUIT.