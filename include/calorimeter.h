/* @file calorimeter.h
 * @brief Calorimeter routines
 * @date August 18th, 2015
 */

#ifndef CALORIMETER_H
#define CALORIMETER_H

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

extern FILE *file;

/**
 * @brief Initialize devices
 */
void initDevices();

/**
 * @brief Read voltage
 * @return Voltage (volts)
 */
double readVoltage();

/**
 * @brief Measure voltage and time to read
 * @param voltage Pointer to voltage (volts)
 * @return Time spent to read data (milliseconds)
 */
int measureVoltage(double *voltage);

/**
 * @brief Set intensity
 * @param amps Intensity (amps)
 */
void setIntensity(double amps);

/**
 * @brief Shutdown source
 */
void shutdownSource();

/**
 * @brief Ask number of inyections from stdin
 * @post Inyections positive
 * @return Input value
 */
int askInyections();

/**
 * @brief Ask latency from stdin
 * @post Latency positive
 * @return Input value
 */
int askLatency();

/**
 * @brief Ask time to establish baseline from stdin
 * @post Time multiply of latency
 * @return Input value
 */
int askTimeBase(int latency);

/**
 * @brief Ask maximum deviation from stdin
 * @post Deviation positive
 * @return Input value
 */
double askMaxDeviation();

/**
 * @brief Ask energy (millijoules)
 * @return Energy (joules)
 */
double askEnergy();

/**
 * @brief Ask gain parameter
 * @return Gain parameter
 */
double askGainParam();

/**
 * @brief Fill an array with reads
 * @param voltages Array of doubles as output parameter
 * @param size Length of the array
 * @param latency Time of waiting between readings (seconds)
 * @param file Output file
 */
void fillVoltages(double *voltages, int size, int latency, FILE *file);

/**
 * @brief Get mean from a set of data
 * @param data Array of data
 * @param size Count of data
 * @return Mean of the data
 */
double getMean(const double data[], int size);

/**
 * @brief Get deviation from a set of data
 * @param data Array of data
 * @param size Count of data
 * @return Standard deviation of the data
 */
double getDeviation(const double data[], int size);

/**
 * @brief Get deviation from a set of data
 * @param data Array of data
 * @param size Count of data
 * @return Standard deviation of the data
 */
double getMax(const double data[], int size);

/**
 * @brief Get minimum value from an array
 * @param data Array of values
 * @param size Length of the array
 * @return Minimum value found
 */
double getMin(const double data[], int size);

#endif