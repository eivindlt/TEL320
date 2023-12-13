#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include <stdint.h>

void calculate_distance_to_water_surface(float *distance_to_water_surface, float *previous_distance_to_water_surface, uint16_t (*filtered_peaks)[3], uint16_t *filtered_peaks_count, int8_t *full_or_empty_indicator);

void calculate_flow_rate(float *water_level, float *flow_rate);

#endif /* CALCULATIONS_H */