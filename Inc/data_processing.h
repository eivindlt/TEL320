#ifndef PEAK_H
#define PEAK_H

void peak_detection(uint16_t *serial_data, uint16_t data_length, float start_m, float length_m,
                    float step_length_m,  uint16_t (*peaks)[3], uint16_t *peaks_count);

float calculate_slope_second_half(uint16_t *serial_data, uint16_t data_length, int8_t *full_or_empty_indicator);

void filter_peaks(uint16_t (*filtered_peaks)[3], uint16_t *filtered_peaks_count, uint16_t (*peaks)[3], uint16_t *peaks_count);

void moving_average_filter(uint16_t *data, uint16_t data_length, uint16_t window_size);

float calculate_flatness_second_half(uint16_t *serial_data, uint16_t data_length, int8_t *full_or_empty_indicator);


#endif // PEAK_H
