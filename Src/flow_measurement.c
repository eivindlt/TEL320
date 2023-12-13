//Authors: Eivind Lid Trøen, Elias Evjen Hartmark, Sivert Lynum, Tage Andersen

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "acc_hal_definitions.h"
#include "acc_hal_integration.h"
#include "acc_rss.h"
#include "acc_service.h"
#include "acc_service_envelope.h"
#include "acc_version.h"
#include "data_processing.h" // Peak detection algorithm
#include "utilities.h" // Functions for printing data and updating the envelope configuration
#include "params.h" // Parameters for the sensor and the peak detection algorithm
#include "calculations.h" // Functions for calculating the flow rate and the distance to the water surface

/* This file contains the main function for the flow measurement application.
 *   - Activate Radar System Software (RSS)
 *   - Create an envelope service configuration
 *   - Update the envelope service configuration with suitable parameters retrieved from the params.h file
 *   - Create an envelope service using the previously created configuration
 *   - Destroy the envelope service configuration
 *   - Activate the envelope service
 *   - Get the result
 *   - Use the measurement result for flow measurement
 *   	- Calling necessary functions to calculate the flow rate
 *   - Reads measurement data and prints it the number of iterations set in the params.h file
 *   - Deactivate and destroy the envelope service
 *   - Deactivate Radar System Software (RSS)
 */


int flow_measurement_service_envelope(int argc, char *argv[])
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Initiate nessesary setup for the sensor
	(void)argc;
	(void)argv;
	printf("Acconeer software version %s\n", acc_version_get());
	//Initialize the HAL library and board peripherals
	const acc_hal_t *hal = acc_hal_integration_get_implementation();

	if (!acc_rss_activate(hal))
	{
		printf("acc_rss_activate() failed\n");
		return EXIT_FAILURE;
	}

	//Create the envelope configuration
	acc_service_configuration_t envelope_configuration = acc_service_envelope_configuration_create();

	if (envelope_configuration == NULL)
	{
		printf("acc_service_envelope_configuration_create() failed\n");
		acc_rss_deactivate();
		return EXIT_FAILURE;
	}
	//Update the configuration
	update_configuration(envelope_configuration);

	acc_service_handle_t handle = acc_service_create(envelope_configuration);

	if (handle == NULL)
	{
		printf("acc_service_create() failed\n");
		acc_service_envelope_configuration_destroy(&envelope_configuration);
		acc_rss_deactivate();
		return EXIT_FAILURE;
	}

	acc_service_envelope_configuration_destroy(&envelope_configuration);

	acc_service_envelope_metadata_t envelope_metadata = { 0 };
	acc_service_envelope_get_metadata(handle, &envelope_metadata);

	//Print the metadata, this data is needed for the data visualization in the Python script
	printf("Start: %d mm\n", (int)(envelope_metadata.start_m * scale_to_mm));
	printf("Length: %u mm\n", (unsigned int)(envelope_metadata.length_m * scale_to_mm));
	printf("Pipe diameter: %u mm\n", (unsigned int)(pipe_diameter * scale_to_mm));
	printf("Data length: %u\n", (unsigned int)(envelope_metadata.data_length));
	printf("Step length: %f mm\n", envelope_metadata.step_length_m * scale_to_mm);

	if (!acc_service_activate(handle))
	{
		printf("acc_service_activate() failed\n");
		acc_service_destroy(&handle);
		acc_rss_deactivate();
		return EXIT_FAILURE;
	}

	bool                               success    = true;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//Initialize the array that will hold the envelope data
	uint16_t                           data[envelope_metadata.data_length];
	//Initialize the struct that will hold the result info
	acc_service_envelope_result_info_t result_info;
	//Initialize the variable that will hold the indicator of if the pipe is full or empty
	int8_t							   full_or_empty_indicator = 0;
	float							   previous_distance_to_water_surface = (distance_to_pipe*scale_to_mm)+(pipe_diameter*scale_to_mm/2);


	//Run the program for a set number of iterations before shutting down
	for (int k = 0; k < iterations; k++)
	{
			//Initialize the variable that will hold the slope of the second half of the envelope
			float							   slope_second_half = 0;
			//Initialize the variable that will hold the flatness of the second half of the envelope
			float							   flatness_second_half = 0;
			//Initialize the array that will hold the peaks
			uint16_t 						   peaks[100][3];
			//Initialize the variable that will hold the number of peaks
			uint16_t 						   peaks_count = 0;
			//Initialize the array that will hold the filtered peaks
			uint16_t 						   filtered_peaks[100][3];
			//Initialize the variable that will hold the number of filtered peaks
			uint16_t 						   filtered_peaks_count = 0;
			//Initialize the variable that will hold the distance to the pipe
			float							   distance_to_water_surface = ((pipe_diameter/2)+distance_to_pipe)*scale_to_mm;
			//Initialize the variable that will hold the water level in the pipe
			float							   water_level = 0;
			//Initialize the variable that will hold the flow rate
			float							   flow_rate = 0;

			//Run the envelope service and get the result
			success = acc_service_envelope_get_next(handle, data, envelope_metadata.data_length, &result_info);
			//Check if the envelope service was successful
			if (!success)
			{
				printf("acc_service_envelope_get_next() failed\n");
				continue;
			}
			else if (k < 2) //Throw away the first two iterations to get a more accurate result
			{
				continue;
			}

			//Filter the data using a moving average filter
			moving_average_filter(data, envelope_metadata.data_length, moving_average_window_size);

			//Print the envelope data
			print_data(data, envelope_metadata.data_length);

			//Find the peaks in the envelope data
			peak_detection(data, envelope_metadata.data_length, envelope_metadata.start_m, envelope_metadata.length_m,
			 envelope_metadata.step_length_m, peaks, &peaks_count);
			
			//Calculate the slope of the second half of the envelope
			slope_second_half = calculate_slope_second_half(data, envelope_metadata.data_length, &full_or_empty_indicator);
			
			//Calculate the flatness of the second half of the envelope
			flatness_second_half = calculate_flatness_second_half(data, envelope_metadata.data_length, &full_or_empty_indicator);

			//Filter the peaks
			filter_peaks(filtered_peaks, &filtered_peaks_count, peaks, &peaks_count);
			
			//Calculate the distance to the water surface
			calculate_distance_to_water_surface(&distance_to_water_surface, &previous_distance_to_water_surface, filtered_peaks, &filtered_peaks_count, &full_or_empty_indicator);

			//Calculate the water level
			water_level = ((distance_to_pipe+pipe_diameter)*scale_to_mm)- distance_to_water_surface;

			//Calculate the flow rate
			calculate_flow_rate(&water_level, &flow_rate);

			//Print the results
			print_results(&distance_to_water_surface, &slope_second_half, &flatness_second_half, &full_or_empty_indicator, &peaks_count, &filtered_peaks_count, filtered_peaks, &water_level, &flow_rate);

	}		
	bool deactivated = acc_service_deactivate(handle);

	acc_service_destroy(&handle);

	acc_rss_deactivate();

	if (deactivated && success)
	{
		printf("Application finished OK\n");
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;

}
