#include "utilities.h"
#include "params.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "acc_service.h"
#include "acc_service_envelope.h"
#include "acc_hal_definitions.h"
#include "acc_hal_integration.h"
#include "acc_rss.h"
#include "acc_version.h"




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