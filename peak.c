#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "acc_hal_definitions.h"
#include "acc_hal_integration.h"
#include "acc_rss.h"
#include "acc_service.h"
#include "acc_service_envelope.h"
#include "acc_version.h"
#include "peak.h"
#include "params.h"

void calculate_distance_to_water_surface(float *distance_to_water_surface, float *previous_distance_to_water_surface, uint16_t (*filtered_peaks)[3], uint16_t *filtered_peaks_count, int8_t *full_or_empty_indicator)
{
	// Check how many of the filtered peaks there are and if the pipe is full or empty
	if (*filtered_peaks_count == 0)
	{
		printf("No peaks found.\n");
	}
	else if (*filtered_peaks_count == 1)
	{
		printf("One peak found.\n");
		if (*full_or_empty_indicator < 0)
		{
			// Pipe is empty
			*distance_to_water_surface = (distance_to_pipe + pipe_diameter) * scale_to_mm;
		}
		else if (*full_or_empty_indicator >= 0)
		{
			// Pipe is full, or almost full
			*distance_to_water_surface = filtered_peaks[0][0];
		}
	}
	else
	{
		printf("Multiple peaks found.\n");
		*distance_to_water_surface = filtered_peaks[1][0];
	}
	//Discard the measurement if the meassurment is VERY different from the previous one
	if (fabs(*distance_to_water_surface - *previous_distance_to_water_surface) > pipe_diameter * scale_to_mm / 8 * 5)
	{
		*distance_to_water_surface = *previous_distance_to_water_surface;
	}
	*previous_distance_to_water_surface = *distance_to_water_surface;
}

// Filter the data using a moving average filter to remove noise
void moving_average_filter(uint16_t *data, uint16_t data_length, uint16_t window_size)
{
    uint16_t *filtered_data = malloc(data_length * sizeof(uint16_t));
    for (int i = 0; i < data_length; i++)
    {
        uint16_t sum = 0;
        uint16_t count = 0;
        for (int j = i - window_size / 2; j <= i + window_size / 2; j++)
        {
            if (j >= 0 && j < data_length)
            {
                sum += data[j];
                count++;
            }
        }
        filtered_data[i] = sum / count;
    }
    memcpy(data, filtered_data, data_length * sizeof(uint16_t));
    free(filtered_data);
}

/* Calculate the ratio of the average of the third quarter of the array to the average of the 
last quarter of the array to determine if the pipe is full or empty*/
float calculate_slope_second_half(uint16_t *serial_data, uint16_t data_length, int8_t *full_or_empty_indicator)
{
    float slope = 0.0;
	int quarter_length = data_length / 4; // Length of a quarter of the array

    // Calculate the average of the third quarter of the array
    float sum_third_quarter = 0.0;
    for (int i = 2 * quarter_length; i < 3 * quarter_length; i++)
    {
        sum_third_quarter += serial_data[i];
    }
    float avg_third_quarter = sum_third_quarter / quarter_length;

    // Calculate the average of the last quarter of the array
    float sum_last_quarter = 0.0;
    for (int i = 3 * quarter_length; i < data_length; i++)
    {
        sum_last_quarter += serial_data[i];
    }
    float avg_last_quarter = sum_last_quarter / quarter_length;
	//Calculate the ratio of the two averages
    slope = avg_third_quarter / avg_last_quarter;

	//Check if the slope of the second half of the envelope is above or below 1.0 to determine if the pipe is full or empty
	if (slope < slope_threshold && *full_or_empty_indicator > -full_or_empty_threshold)	// If the pipe is empty, decrease the indicator
	{
		(*full_or_empty_indicator)--;
	}
	else if (slope >= slope_threshold && *full_or_empty_indicator < full_or_empty_threshold)	//If the pipe is full, increase the indicator
	{
		(*full_or_empty_indicator)++;
	}
    // Return the ratio of the two averages
	return slope;
}

float calculate_flatness_second_half(uint16_t *serial_data, uint16_t data_length, int8_t *full_or_empty_indicator)
{
	float average_second_half = 0;
	float sum_second_half = 0;
	float flatness_second_half = 0;

	for (int i = data_length / 2; i < data_length; i++)
	{
		sum_second_half += serial_data[i];
	}
	average_second_half = sum_second_half / (data_length / 2);
	for (int i = data_length / 2; i < data_length; i++)
	{
		flatness_second_half += abs(serial_data[i] - average_second_half);
	}
	flatness_second_half = (flatness_second_half / (data_length / 2))/average_second_half;
	
	if (flatness_second_half < flatness_threshold && *full_or_empty_indicator < full_or_empty_threshold)
	{
		(*full_or_empty_indicator)++;
	}
	else if (flatness_second_half >= flatness_threshold && *full_or_empty_indicator > -full_or_empty_threshold)
	{
		(*full_or_empty_indicator)--;
	}
	return flatness_second_half;
}

//Filter the peaks to remove peaks that are not from the pipe
void filter_peaks(uint16_t (*filtered_peaks)[3], uint16_t *filtered_peaks_count, uint16_t (*peaks)[3], uint16_t *peaks_count)
{
	for (int i = 0; i < *peaks_count; i++)
	{//If the peak is above the threshold and is within the range of the pipe plus error margin
		if (peaks[i][1] > peak_threshold && peaks[i][0] < (distance_to_pipe + pipe_diameter)*1000)
		{
			filtered_peaks[*filtered_peaks_count][0] = peaks[i][0];
			filtered_peaks[*filtered_peaks_count][1] = peaks[i][1];
            filtered_peaks[*filtered_peaks_count][2] = peaks[i][2];
			*filtered_peaks_count += 1;
		}
		printf("Number of peaks: %d\n", *peaks_count);
	}
}

//Find the peaks in the envelope data
void peak_detection(uint16_t *serial_data, uint16_t data_length, float start_m, 
float length_m, float step_length_m, uint16_t (*peaks)[3], uint16_t *peaks_count)
{
    uint16_t previous_largest_intensity = 0;
    uint16_t intensity;
    uint16_t i;
    uint16_t peak[3];
	float temp = 0;

    for (i = 0; i < data_length; i++)
    {
        intensity = serial_data[i];

        if (i > 0 && i < data_length - 1)
        {
			//Calculate the distance to the peak and add the offset
			temp = ((start_m + ((float)i * step_length_m))*1000)-meassurement_offset;
			peak[0] = (uint16_t)temp;
			peak[1] = intensity;
            peak[2] = i;
            if (intensity > serial_data[i - 1] && intensity > serial_data[i + 1])
            {
                peaks[*peaks_count][0] = peak[0];
                peaks[*peaks_count][1] = peak[1];
                peaks[*peaks_count][2] = peak[2];
                (*peaks_count)++;
            }
            else if (intensity == previous_largest_intensity && intensity > serial_data[i + 1])
            {
				peaks[*peaks_count][0] = peak[0];
                peaks[*peaks_count][1] = peak[1];
                peaks[*peaks_count][2] = peak[2];
                (*peaks_count)++;
            }
        }
		//If the intensity is larger than the previous largest intensity, set the previous largest intensity to the current intensity
        if (intensity > previous_largest_intensity && intensity > serial_data[i - 1])
        {
            previous_largest_intensity = intensity;
        }
		//If the intensity is smaller than the previous intensity, set the previous largest intensity to 0
        else if (intensity < serial_data[i - 1]) 
        {
            previous_largest_intensity = 0;
        }
    }
}

void update_configuration(acc_service_configuration_t envelope_configuration)
{
	float start_m  = distance_to_pipe;
	float length_m = pipe_diameter+distance_beyond_pipe;

	// Set the start and length of the envelope in meters
	acc_service_requested_start_set(envelope_configuration, start_m);
	acc_service_requested_length_set(envelope_configuration, length_m);
	// Set the profile to 1
    acc_service_profile_set(envelope_configuration, envelope_profile);
	// Set how many samples that should be used to calculate the average value
	acc_service_hw_accelerated_average_samples_set(envelope_configuration, accelerated_average_samples);
	// Set how much the current measurement should be weighed against the previous one
	acc_service_envelope_running_average_factor_set(envelope_configuration, running_average_factor);
	//Set normalization to true or false
	acc_service_envelope_noise_level_normalization_set(envelope_configuration, normalization);
	// Set the gain
	acc_service_receiver_gain_set(envelope_configuration, gain);
	// Set the signal mode to maximum signal attenuation
	acc_service_maximize_signal_attenuation_set(envelope_configuration, max_signal);
}

//Prints the envelope data
void print_data(uint16_t *data, uint16_t data_length)
{
	printf("Envelope data:\n");
	for (uint16_t i = 0; i < data_length; i++)
	{
		if ((i > 0) && ((i % 8) == 0))
		{
			printf("\n");
		}

		printf("%6u", (unsigned int)(data[i]));
	}

	printf("\n");
}

void print_results(float *distance_to_water_surface, float *slope_second_half, float *flatness_second_half, int8_t *full_or_empty_indicator, uint16_t *peaks_count, uint16_t *filtered_peaks_count, uint16_t (*filtered_peaks)[3], float *water_level, float *flow_rate)
{
	printf("Distance to water surface: %f\n", *distance_to_water_surface);
    printf("Water level: %f\n", *water_level);
    printf("Flow rate: %f\n", *flow_rate*1000);
	printf("Slope of the second half of the envelope: %f\n", *slope_second_half);		//Print the slope of the second half of the envelope
	printf("Full or empty indicator: %d\n", *full_or_empty_indicator);					//Print the indicator of if the pipe is full or empty
	printf("Number of peaks: %d\n", *peaks_count);
	printf("Flatness of the second half of the envelope: %f\n", *flatness_second_half);
	printf("Number of filtered peaks: %d\n", *filtered_peaks_count);
    printf("Filtered peaks: ");
	for (int i = 0; i < *filtered_peaks_count; i++)
	{
		printf("%d,%d,%d", filtered_peaks[i][0], filtered_peaks[i][1], filtered_peaks[i][2]);
        if (i < *filtered_peaks_count - 1)
        {
            printf(";");
        }
	}
    printf("\n");
}

void calculate_flow_rate(float *water_level, float *flow_rate) {
    float D = pipe_diameter; // Diameter of the pipe in meters
    float r = D / 2.0f;   // Radius of the pipe in meters
    float h = *water_level / 1000.0f; // Height of the water level in meters
    if (h >= D) {    // If the measured water level is greater than the pipe diameter, the height is set to the pipe diameter
        h = D;
    }
    float theta, W_P, R, A;
    if (h == 0.0f) {  // If the water level is 0, the flow rate is 0
        theta = 0.0f;   // Angle from the center to the segment that is submerged
        W_P = 0.0f;     // Wetted perimeter
        R = 0.0f;       // Hydraulic radius
        A = 0.0f;       // Area of the segment that is submerged
    } else {   
        theta = 2.0f * acosf(1.0f - (h / r)); // Angle from the center to the segment that is submerged
        if (isnan(theta)) {
            printf("Error, could not calculate theta\n");
            return;
        }
        W_P = D * (theta / (2.0f * M_PI));  // Wetted perimeter
        A = (W_P * h) / 2.0f; // Area of the segment that is submerged
        R = A / W_P; // Hydraulic radius
    }
    // Mannings roughness coefficient for PVC
    float n = mannings_roughness_coefficient;
    // Calculate the flow rate
    *flow_rate = ((1.0f / n) * A * powf(R, 2.0f/3.0f)) * sqrtf(slope_pipe);
    }
