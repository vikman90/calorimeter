/* @file main.c
 * @brief Calorimeter
 * @date August 7th, 2014
 */

#include <windows.h>
#include <IEEE_32.H>
#include <ieee-c.h>
#include <GPIB.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DEFAULT_LEN 80
#define BUFFER_LEN 4096

#define BOARD_INDEX 0
#define ADDRESS_NANOVOLT 7
#define ADDRESS_SOURCE 16
#define FLAGS 1
#define EOS 0

#define BASELINE_VARIATION 1e-7
#define MAX_INTENSITY 0.03
#define TIME_PULSE 20.0
#define RESISTANCE 324
#define INJECTION_VARIATION 1e-5
#define K 7.846

// Absolute value of x
#define ABS(x) ((x > 0) ? x : -x)

#define OUT_FILE "output.txt"

static int devNanovolt;
static int devSource;
static int warningShown = 0;
static LARGE_INTEGER frequency;
static FILE *file;

/**
 * @brief Initialize devices
 */
void initDevices() {
	char STR0[] = "*RST";
	char STR1[] = ":SENS:FUNC 'VOLT'";
	char STR2[] = ":SENS:CHAN 1";
	char STR3[] = ":SENS:VOLT:CHAN1:RANG:AUTO ON";
	
	devNanovolt = ibdev(BOARD_INDEX, ADDRESS_NANOVOLT, 0, T10s, FLAGS, EOS);
	devSource = ibdev(BOARD_INDEX, ADDRESS_SOURCE, 0, T10s, FLAGS, 0);
	
	if (devNanovolt < 0) {
		fprintf(stderr, "Error connecting to nanovoltmeter\n");
		exit(EXIT_FAILURE);
	}
	
	if (devSource < 0) {
		fprintf(stderr, "Error connecting to source\n");
		exit(EXIT_FAILURE);
	}
	
	ibwrt(devNanovolt, STR0, strlen(STR0));
	ibwrt(devNanovolt, STR1, strlen(STR1));
	ibwrt(devNanovolt, STR2, strlen(STR2));
	ibwrt(devNanovolt, STR3, strlen(STR3));
}

/**
 * @brief Read voltage
 * @return Voltage (volts)
 */
double readVoltage() {
	char STR[] = ":READ?";
	char buffer[BUFFER_LEN];
	
	ibwrt(devNanovolt, STR, strlen(STR));
	ibrd(devNanovolt, buffer, BUFFER_LEN);
	
	return atof(buffer);
}

/**
 * @brief Measure voltage and time to read
 * @param voltage Pointer to voltage (volts)
 * @return Time spent to read data (milliseconds)
 */
int measureVoltage(double *voltage) {
	LARGE_INTEGER counter0, counter1;
	
	QueryPerformanceCounter(&counter0);
	*voltage = readVoltage();
	QueryPerformanceCounter(&counter1);
	
	return (int)((double)(counter1.QuadPart - counter0.QuadPart) / frequency.QuadPart * 1000);
}

/**
 * @brief Set intensity
 * @param amps Intensity (amps)
 */
void setIntensity(double amps) {
	static int active = 0;
	char STR0[] = "OUTP ON";
	char str1[DEFAULT_LEN];
	
	if (amps > 0.0) {
		sprintf(str1, "SOUR:CURR %f", amps);
		ibwrt(devSource, str1, strlen(str1));

		if (!active) {
			ibwrt(devSource, STR0, strlen(STR0));
			active = 1;
		}
	}	
}

/**
 * @brief Shutdown source
 */
void shutdownSource() {
	char STR[] = "OUTP OFF";
	ibwrt(devSource, STR, strlen(STR));
}

/**
 * @brief Ask number of inyections from stdin
 * @post Inyections positive
 * @return Input value
 */
int askInyections() {
	int ninyections = 0;
	
	while (ninyections <= 0) {
		printf("Number of inyections: ");
		scanf("%d", &ninyections);
	}
	
	fflush(stdin);
	return ninyections;
}

/**
 * @brief Ask latency from stdin
 * @post Latency positive
 * @return Input value (seconds)
 */
int askLatency() {
	int latency = 0;
	
	while (latency <= 0) {
		printf("Latency (sec): ");
		scanf("%d", &latency);
	}
	
	fflush(stdin);
	return latency;
}

/**
 * @brief Ask time to establish baseline from stdin
 * @post Time multiply of latency
 * @return Input value
 */
int askTimeBase(int latency) {
	int tbase;
	
	do {
		printf("Time for baseline (sec): ");
		scanf("%d", &tbase);
	} while (tbase / latency * latency != tbase);
	
	fflush(stdin);
	return tbase;
}

/**
 * @brief Ask maximum deviation from stdin
 * @post Deviation positive
 * @return Input value
 */
double askMaxDeviation() {
	double maxDev = 0;
	
	while (maxDev <= 0) {
		printf("Maximum deviation: ");
		scanf("%lf", &maxDev);
	}
	
	fflush(stdin);	
	return maxDev;
}

/**
 * @brief Ask energy (millijoules)
 * @return Energy (joules)
 */
double askEnergy() {
	double energy = -1.0;
	
	while (energy < 0.0) {
		printf("Energy (mJ): ");
		scanf("%lf", &energy);
	}
	
	fflush(stdin);
	return energy / 1000;
}

/**
 * @brief Ask gain parameter
 * @return Gain parameter
 */
double askGainParam() {
	double param = 0.0;
	
	while (param <= 0.0) {
		printf("Gain param: ");
		scanf("%lf", &param);
	}
	
	fflush(stdin);
	return param;
}

/**
 * @brief Fill an array with reads
 * @param voltages Array of doubles as output parameter
 * @param size Length of the array
 * @param latency Time of waiting between readings (seconds)
 */
void fillVoltages(double *voltages, int size, int latency) {
	int i;
	int timespan;
	int interval;
	
	for (i = 0; i < size; i++) {
		timespan = measureVoltage(&voltages[i]);
		interval = latency * 1000 - timespan;
		
		if (interval < 0)
			fprintf(stderr, "Warning: latency too small. Query time: %d ms.\n", timespan);
		
		Sleep(interval);
		printf("\tRead: %.9f volts.\n", voltages[i]);
		fprintf(file, "%.9f\n", voltages[i]);
	}
}

/**
 * @brief Get mean from a set of data
 * @param data Array of data
 * @param size Count of data
 * @return Mean of the data
 */
double getMean(const double data[], int size) {
	int i;
	double accum = 0.0;
	
	for (i = 0; i < size; i++)
		accum += data[i];
	
	return accum / size;
}

/**
 * @brief Get deviation from a set of data
 * @param data Array of data
 * @param size Count of data
 * @return Standard deviation of the data
 */
double getDeviation(const double data[], int size) {
	double mean = getMean(data, size);
	double accum = 0.0;
	int i;
	
	for (i = 0; i < size; i++) {
		double diff = data[i] - mean;
		accum += diff * diff;
	}
	
	return sqrt(accum / size);
}

/**
 * Get maximum value from an array
 * @param data Array of values
 * @param size Length of the array
 * @return Maximum value found
 */
double getMax(const double data[], int size) {
	int i;
	double max = data[0];
	
	for (i = 1; i < size; i++)
		if (data[i] > max)
			max = data[i];
	
	return max;
}

/**
 * Get minimum value from an array
 * @param data Array of values
 * @param size Length of the array
 * @return Minimum value found
 */
double getMin(const double data[], int size) {
	int i;
	double min = data[0];
	
	for (i = 1; i < size; i++)
		if (data[i] < min)
			min = data[i];
	
	return min;
}

/**
 * @brief Main function
 * @return Exit code
 */
int main() {
	int ninyections = askInyections();
	int latency = askLatency();       // s
	int tbase = askTimeBase(latency); // s
	int size = tbase / latency;
	int timespan;                     // ms
	int interval;                     // ms
	int i, j, n;
	double mean;                      // v
	double min, max;         		  // v
	double prevMax, prevMin;		  // v
	double baseline;                  // v
	double voltage;                   // v
	double areaPartial;               // v*s
	double *voltages = (double*)malloc(sizeof(double) * size);
	double *areas = (double*)malloc(sizeof(double) * ninyections);
	
	QueryPerformanceFrequency(&frequency);
	atexit((void(*)())getchar);
	initDevices();
	file = fopen(OUT_FILE, "w");
	
	if (file == NULL)
		fprintf(stderr, "Couldn't open %s\n", OUT_FILE);
	
	// Extract base line
	
	printf("Waiting to reach baseline...\n");	
	fillVoltages(voltages, size, latency);
	mean = getMean(voltages, size);
	min = getMin(voltages, size);
	max = getMax(voltages, size);
	printf("Mean: %.5e volts. Min: %.5e volts. Max: %.5e volts.\n", mean, min, max);
	
	do {
		prevMin = min;
		prevMax = max;
		fillVoltages(voltages, size, latency);
		mean = getMean(voltages, size);
		min = getMin(voltages, size);
		max = getMax(voltages, size);
		printf("Mean: %.5e volts. Min: %.5e volts. Max: %.5e volts.\n", mean, min, max);
		printf("Wait range: [%.5e - %.5e]\n", prevMin - BASELINE_VARIATION, prevMax + BASELINE_VARIATION);
	} while (mean <= prevMin - BASELINE_VARIATION || mean >= prevMax + BASELINE_VARIATION);
	
	baseline = mean;
	printf("Baseline reached: %.5e volts.\n", baseline);
	
	for (n = 0; n < ninyections; n++) {
		areas[n] = 0.0;
	
		// Waiting injection
		
		printf("\aYou can start the injection...\n");
		printf("Waiting [%.5e - %.5e]\n", baseline + INJECTION_VARIATION, baseline - INJECTION_VARIATION);
		timespan = measureVoltage(&voltage);
		
		while (ABS(voltage - baseline) < INJECTION_VARIATION) {
			Sleep(latency * 1000);
			timespan = measureVoltage(&voltage);
			printf("\tRead: %.9f volts.\n", voltage);
			fprintf(file, "%.9f\n", voltage);
		}
		
		// Area and compensation
		
		printf("\a\nMeasuring area...\n\n");
		fprintf(file, "\nMeasuring peak area %d\n\n", n + 1);
		
		// We'll take two measuring stages in order to avoid the first peak
		
		for (n = 0; n < 2; n++) {
			for (i = 0; i < size; i++) {
				fprintf(file, "%.9f\n", voltage);
				areaPartial = (voltage - baseline) * latency;
				areas[n] += ABS(areaPartial);
				voltages[i] = voltage;
				printf("\tRead: %.9f volts.\n", voltage);
				interval = latency * 1000 - timespan;
				
				if (interval < 0 && !warningShown) {
					fprintf(stderr, "Warning: latency too small.\n");
					warningShown = 1;
				}
				
				Sleep(interval);
				timespan = measureVoltage(&voltage);
			}
		}
		
		mean = getMean(voltages, size);
		min = getMin(voltages, size);
		max = getMax(voltages, size);
		
		printf("Mean: %.5e volts. Min: %.5e volts. Max: %.5e volts.\n", mean, min, max);
		
		do {
			prevMin = min;
			prevMax = max;
			
			for (i = 0; i < size; i++) {
				fprintf(file, "%.9f\n", voltage);
				areaPartial = (voltage - baseline) * latency;
				areas[n] += ABS(areaPartial);
				voltages[i] = voltage;
				printf("\tRead: %.9f volts.\n", voltage);
				interval = latency * 1000 - timespan;
				
				if (interval < 0 && !warningShown) {
					fprintf(stderr, "Warning: latency too small.\n");
					warningShown = 1;
				}
				
				Sleep(interval);
				timespan = measureVoltage(&voltage);
			}
			
			mean = getMean(voltages, size);
			min = getMin(voltages, size);
			max = getMax(voltages, size);
			printf("Mean: %.5e volts. Min: %.5e volts. Max: %.5e volts.\n", mean, min, max);
			printf("Wait range: [%.5e - %.5e]\n", prevMin - BASELINE_VARIATION, prevMax + BASELINE_VARIATION);
		} while (mean <= prevMin - BASELINE_VARIATION || mean >= prevMax + BASELINE_VARIATION);
		
		baseline = mean;
		printf("\aArea: %.9e V*s\n", areas[n]);
		printf("\aEnergy: %.9e J\n\n", areas[n] * K);
		fprintf(file, "\tArea: %.9f\n", areas[n]);
		fprintf(file, "\tEnergy: %.9f\n\n", areas[n] * K);
	}
	
	for (n = 0; n < ninyections; n++) {
		printf("\aArea %d: %.9e V*s\n", n + 1, areas[n]);
		printf("\aEnergy %d: %.9e J\n\n", n + 1, areas[n] * K);
	}
	
	fclose(file);
	free(voltages);
	return EXIT_SUCCESS;
}
