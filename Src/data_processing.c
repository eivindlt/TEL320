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
#include "data_processing.h" // Peak detection algorithm
#include "params.h"


/*
Filters the peaks by taking the average of the peaks that are close to each other.
How big the window is, is determined by the window_size parameter.
@param peaks The array that holds the peaks
@param peaks_count A pointer to the variable that holds the number of peaks
@param window_size The size of the window
*/
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

/*
    * Calculate the ratio of the average of the third quarter of the array to the average of the 
        last quarter of the array to determine if the pipe is full or empty
    * @param data The envelope data
    * @param data_length The length of the envelope data
    * @param full_or_empty_indicator A pointer to the variable that indicates if the pipe is full or empty
    * @return The slope of the second half of the envelope
*/
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

/*
    * Calculate the flatness of the second half of the envelope
    * @param data The envelope data
    * @param data_length The length of the envelope data
    * @param full_or_empty_indicator A pointer to the variable that indicates if the pipe is full or empty
    * @return The flatness of the second half of the envelope
*/
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

/*
Filter the peaks to remove peaks that are below the threshold or outside the range of the pipe
@param filtered_peaks The array that will hold the filtered peaks
@param filtered_peaks_count A pointer to the variable that holds the number of filtered peaks
@param peaks The array that holds the peaks
@param peaks_count A pointer to the variable that holds the number of peaks
*/
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

/*
Alogrithm to detect the peaks in the envelope data. It detects points where the intensity is larger than the previous and next point, 
as well as points where the intensity is the same as the previous point and larger than the next point as long as it is not heading downwards.
@param serial_data The envelope data
@param data_length The length of the envelope data
@param start_m The start distance of the envelope
@param length_m The length of the envelope
@param step_length_m The step length of the envelope
@param peaks The array that will hold the peaks
@param peaks_count A pointer to the variable that holds the number of peaks
*/
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


