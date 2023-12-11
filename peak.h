#ifndef PEAK_H
#define PEAK_H

void update_configuration(acc_service_configuration_t envelope_configuration);

void print_data(uint16_t *data, uint16_t data_length);

int acc_example_service_envelope(int argc, char *argv[]);

void peak_detection(uint16_t *serial_data, uint16_t data_length, float start_m, float length_m,
                    float step_length_m,  uint16_t (*peaks)[3], uint16_t *peaks_count);

float calculate_slope_second_half(uint16_t *serial_data, uint16_t data_length, int8_t *full_or_empty_indicator);

void filter_peaks(uint16_t (*filtered_peaks)[3], uint16_t *filtered_peaks_count, uint16_t (*peaks)[3], uint16_t *peaks_count);

void moving_average_filter(uint16_t *data, uint16_t data_length, uint16_t window_size);

float calculate_flatness_second_half(uint16_t *serial_data, uint16_t data_length, int8_t *full_or_empty_indicator);

void calculate_distance_to_water_surface(float *distance_to_water_surface, float *previous_distance_to_water_surface, uint16_t (*filtered_peaks)[3], uint16_t *filtered_peaks_count, int8_t *full_or_empty_indicator);

void calculate_flow_rate(float *water_level, float *flow_rate);

void print_results(float *distance_to_water_surface, float *slope_second_half, float *flatness_second_half, int8_t *full_or_empty_indicator, uint16_t *peaks_count, uint16_t *filtered_peaks_count, uint16_t (*filtered_peaks)[3], float *water_level, float *flow_rate);

#endif // PEAK_H
