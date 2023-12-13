#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdint.h>
#include <acc_service.h>
#include <acc_service_envelope.h>
#include <acc_hal_definitions.h>
#include <acc_hal_integration.h>
#include <acc_rss.h>
#include <acc_version.h>

//This file contains the helper functions used in the flow_measurement.c file


void update_configuration(acc_service_configuration_t envelope_configuration);

void print_data(uint16_t *data, uint16_t data_length);

void print_results(float *distance_to_water_surface, float *slope_second_half, float *flatness_second_half,
 int8_t *full_or_empty_indicator, uint16_t *peaks_count, uint16_t *filtered_peaks_count, uint16_t (*filtered_peaks)[3], float *water_level, float *flow_rate);

#endif /* UTILITIES_H */