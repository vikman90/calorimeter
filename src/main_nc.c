/* @file main.c
 * @brief Calorimeter
 * @date August 7th, 2014
 */

#include "calorimeter.h"

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
	int warningShown = 0;
	FILE *file;
	
	atexit((void(*)())getchar);
	initDevices();
	file = fopen(OUT_FILE, "w");
	
	if (file == NULL)
		fprintf(stderr, "Couldn't open %s\n", OUT_FILE);
	
	// Extract base line
	
	printf("Waiting to reach baseline...\n");	
	fillVoltages(voltages, size, latency, file);
	mean = getMean(voltages, size);
	min = getMin(voltages, size);
	max = getMax(voltages, size);
	printf("Mean: %.5e volts. Min: %.5e volts. Max: %.5e volts.\n", mean, min, max);
	
	do {
		prevMin = min;
		prevMax = max;
		fillVoltages(voltages, size, latency, file);
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
		
		for (j = 0; j < 2; j++) {
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
