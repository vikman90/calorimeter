/* @file calorimeter.c
 * @brief Calorimeter routines
 * @date August 18th, 2015
 */

#include "calorimeter.h"

static int devNanovolt;
static int devSource;
static LARGE_INTEGER frequency;

// Initialize devices

void initDevices() {
	char STR0[] = "*RST";
	char STR1[] = ":SENS:FUNC 'VOLT'";
	char STR2[] = ":SENS:CHAN 1";
	char STR3[] = ":SENS:VOLT:CHAN1:RANG:AUTO ON";
	char STR4[] = "SOUR:CLE";
	char STR5[] = "SOUR:CURR:RANG:AUTO ON";
	
	QueryPerformanceFrequency(&frequency);
	
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
	ibwrt(devSource, STR4, strlen(STR4));
	ibwrt(devSource, STR5, strlen(STR5));
}

// Read voltage

double readVoltage() {
	char STR[] = ":READ?";
	char buffer[BUFFER_LEN];
	
	ibwrt(devNanovolt, STR, strlen(STR));
	ibrd(devNanovolt, buffer, BUFFER_LEN);
	
	return atof(buffer);
}

// Measure voltage and time to read

int measureVoltage(double *voltage) {
	LARGE_INTEGER counter0, counter1;
	
	QueryPerformanceCounter(&counter0);
	*voltage = readVoltage();
	QueryPerformanceCounter(&counter1);
	
	return (int)((double)(counter1.QuadPart - counter0.QuadPart) / frequency.QuadPart * 1000);
}

// Set intensity

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

// Shutdown source

void shutdownSource() {
	char STR[] = "OUTP OFF";
	ibwrt(devSource, STR, strlen(STR));
}

// Ask number of inyections from stdin

int askInyections() {
	int ninyections = 0;
	
	while (ninyections <= 0) {
		printf("Number of inyections: ");
		scanf("%d", &ninyections);
	}
	
	fflush(stdin);
	return ninyections;
}

// Ask latency from stdin

int askLatency() {
	int latency = 0;
	
	while (latency <= 0) {
		printf("Latency (sec): ");
		scanf("%d", &latency);
	}
	
	fflush(stdin);
	return latency;
}

// Ask time to establish baseline from stdin

int askTimeBase(int latency) {
	int tbase;
	
	do {
		printf("Time for baseline (sec): ");
		scanf("%d", &tbase);
	} while (tbase / latency * latency != tbase);
	
	fflush(stdin);
	return tbase;
}

// Ask maximum deviation from stdin

double askMaxDeviation() {
	double maxDev = 0;
	
	while (maxDev <= 0) {
		printf("Maximum deviation: ");
		scanf("%lf", &maxDev);
	}
	
	fflush(stdin);	
	return maxDev;
}

// Ask energy (millijoules)

double askEnergy() {
	double energy = -1.0;
	
	while (energy < 0.0) {
		printf("Energy (mJ): ");
		scanf("%lf", &energy);
	}
	
	fflush(stdin);
	return energy / 1000;
}

// Ask gain parameter

double askGainParam() {
	double param = 0.0;
	
	while (param <= 0.0) {
		printf("Gain param: ");
		scanf("%lf", &param);
	}
	
	fflush(stdin);
	return param;
}

// Fill an array with reads

void fillVoltages(double *voltages, int size, int latency, FILE *file) {
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

// Get mean from a set of data

double getMean(const double data[], int size) {
	int i;
	double accum = 0.0;
	
	for (i = 0; i < size; i++)
		accum += data[i];
	
	return accum / size;
}

// Get deviation from a set of data

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

// Get deviation from a set of data

double getMax(const double data[], int size) {
	int i;
	double max = data[0];
	
	for (i = 1; i < size; i++)
		if (data[i] > max)
			max = data[i];
	
	return max;
}

// Get minimum value from an array

double getMin(const double data[], int size) {
	int i;
	double min = data[0];
	
	for (i = 1; i < size; i++)
		if (data[i] < min)
			min = data[i];
	
	return min;
}
