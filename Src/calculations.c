#include "params.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "calculations.h"

/*
    * Calculate the flow rate using the measured water level
    * Utilizes the Manning's equation
    * @param water_level The measured water level in mm
    * @param flow_rate The calculated flow rate in m^3/s
*/
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
        
        W_P = r * (2.0f * acos((r-h)/r));  // Wetted perimeter
        A = pow(r, 2.0f) * (theta - sinf(theta)) / 2.0f; // Area of the segment that is submerged
        R = A / W_P; // Hydraulic radius
    }
    // Mannings roughness coefficient for PVC
    float n = mannings_roughness_coefficient;
    // Calculate the flow rate
    *flow_rate = ((1.0f / n) * A * powf(R, 2.0f/3.0f)) * sqrtf(slope_pipe);
    }

/*
    * Calculate the distance to the water surface
    * @param distance_to_water_surface A pointer to the variable that holds the distance to the water surface
    * @param previous_distance_to_water_surface A pointer to the variable that holds the previous distance to the water surface
    * @param filtered_peaks The array that holds the filtered peaks
    * @param filtered_peaks_count A pointer to the variable that holds the number of filtered peaks
    * @param full_or_empty_indicator A pointer to the variable that indicates if the pipe is full or empty
*/
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
	// //Discard the measurement if the meassurment is VERY different from the previous one
	// if (fabs(*distance_to_water_surface - *previous_distance_to_water_surface) > pipe_diameter * scale_to_mm / 8 * 5)
	// {
	// 	*distance_to_water_surface = *previous_distance_to_water_surface;
	// }
	// *previous_distance_to_water_surface = *distance_to_water_surface;
}
