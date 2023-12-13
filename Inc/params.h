#ifndef PARAMS_H
#define PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

#define distance_to_pipe 0.120f // Must be minimum ~0.08m from the sensor to the pipe
#define pipe_diameter 0.090f
#define distance_beyond_pipe 0.011f
#define meassurement_offset 6
#define running_average_factor 0.8f // Must be between 0 and 1.0.
#define moving_average_window_size 5
#define accelerated_average_samples 10
#define peak_threshold 250
#define scale_to_mm 1000.0f
#define error_margin 0.00f
#define iterations 40
#define iterations_for_average 10
#define envelope_profile 1
#define gain 0.1f
#define normalization false
#define max_signal false
#define slope_threshold 0.6f
#define flatness_threshold 0.4f
#define full_or_empty_threshold 5
#define mannings_roughness_coefficient 0.011f
#define slope_pipe 0.1f

#ifdef __cplusplus
}
#endif

#endif /* PARAMS_H */
