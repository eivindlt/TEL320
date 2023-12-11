# Authors: Eivind Lid Trøen, Elias, Sivert Lynum, Tage Andresen
import serial
import numpy as np
import matplotlib.pyplot as plt
import time
import math
import pandas as pd

class RadarOperation:
    '''
    This class handles the radar operation, data collection and visualization
    '''
    def __init__(self) -> None:
        '''
        This function initializes the class
        '''
        self.serial_port = '/dev/ttyACM0'
        self.baud_rate = 115200
        self.ser = serial.Serial(self.serial_port, self.baud_rate)
        self.raw_filename = 'serial_data.log'
        self.plotted_data_filename = 'plotted_data.csv'
        self.collected_data = "" # Variable to store the collected data until it is parsed
        self.fig, self.ax = plt.subplots()
        self.collected_data_cleaned = False
        self.slope = 0.1 #Slope of the pipe in m/m
        self.mannings_roughness_coefficient = 0.011 # Mannings roughness coefficient for PVC

        # Factor to multiply the data length with to get the length of the envelope data. 
        # This allows for robust reading when the envelope data is not the same length each time
        self.data_length_factor = 6.125 
        self.start_time = time.time() # Time when the program starts
        self.measurement_df = pd.DataFrame(columns = ['Time', 'Water level', 'Flow rate']) # Dataframe to store the meassurments
        #Initialize the variables to store the data
        self.data_length = 0
        self.start_m = 0.0
        self.length_m = 0.0
        self.pipe_diameter = 0.0
        self.distance = 0.0
        self.water_level = 0.0
        self.average_index = 0
        self.flow_rate = 0.0
        self.metadata_read_flag = False
        self.filtered_peaks = []

        

    def read_serial_data(self):
        '''
        This function reads the serial data and returns a list of the data
        
        Parameters
        ----------
        None
            
        Returns
        -------
        serial_data : str
            The serial data read from the serial port
        '''
        # Read all the serial data from stream
        serial_data = self.ser.read(self.ser.in_waiting)
        # Convert the data to a string
        try:
            serial_data = serial_data.decode('utf-8')
        except:
            serial_data = None
        return serial_data


    def parse_data(self, serial_data):
        '''
        This function parses the serial data and returns the envelope data
        
        Parameters
        ----------
        serial_data : str
            The serial data to parse

        Returns
        -------
        envelope_data : list
            A list of the envelope data
        '''

        if self.metadata_read_flag == False: # If the metadata has not been read, read it
            self.data_length_index = self.collected_data.find('Data length:') + 13 # Find the index of the data length
            if self.data_length_index != 12 and len(self.collected_data) > self.data_length_index + 4: # Check if the data length is found
                self.read_metadata()
            self.collected_data += serial_data # Add the serial data to the collected data
        else:
            #Find the start index of the envelope data
            start_index = self.collected_data.find('Envelope data:\n') + 15
            end_index = start_index + int(self.data_length*self.data_length_factor)
            #Find the index of the distance to water surface data
            distance_index = self.collected_data.find('Distance to water surface: ') + 27
            water_level_index = self.collected_data.find('Water level: ') + 13
            flow_rate_index = self.collected_data.find('Flow rate: ') + 11
            filtered_peaks_index = self.collected_data.find('Filtered peaks: ') + 16
            try:
                if distance_index != 26 and len(self.collected_data) > distance_index + 5:
                    self.distance = float(self.collected_data[distance_index:distance_index + 5])
                if water_level_index != 12 and len(self.collected_data) > water_level_index + 5:
                    self.water_level = float(self.collected_data[water_level_index:water_level_index + 5])
                if flow_rate_index != 11 and len(self.collected_data) > flow_rate_index + 5:
                    self.flow_rate = float(self.collected_data[flow_rate_index:flow_rate_index + 5])
                if filtered_peaks_index != 15 and len(self.collected_data) > filtered_peaks_index + 5:
                    self.filtered_peaks = []
                    filtered_peaks_str = self.collected_data[filtered_peaks_index:filtered_peaks_index + 100]
                    filtered_peaks_str = filtered_peaks_str.split("\n")
                    filtered_peaks_str = filtered_peaks_str[0]
                    filtered_peaks_str = filtered_peaks_str.split(";")
                    for string in filtered_peaks_str:
                        string = string.split(",")
                        if len(string) == 3:
                            string[0] = int(string[2])
                            string[1] = int(string[1])
                            self.filtered_peaks.append(string)
            except:
                pass
            #self.water_level = (self.start_m + self.pipe_diameter) - self.distance
            #if self.water_level < 0: # Only add the water level if it is positive
            #   self.water_level = 0
            self.time_elapsed = time.time() - self.start_time
            #self.calculate_flow_rate()
            #Add meassurment to dataframe
            new_row = pd.DataFrame({'Time': [self.time_elapsed], 'Water level': [self.water_level], 'Flow rate': [self.flow_rate]})
            self.measurement_df = pd.concat([self.measurement_df, new_row], ignore_index=True)

            if start_index != 14 and len(self.collected_data) > end_index and self.data_length != 0:   
                # Get the envelope data
                envelope_data_str = self.collected_data[start_index:end_index]
                # Split the envelope data into a list of integers if possible and return the data
                try:
                    envelope_data = list(map(int, envelope_data_str.split()))
                except:
                    print("Error")
                    print("Start index: ", start_index)
                    print("End index: ", end_index)
                    print("Length of collected data: ", len(self.collected_data))
                    self.collected_data = ""
                    self.collected_data_cleaned = True
                    time.sleep(0.5)
                    return None
                self.collected_data = ""
                self.collected_data_cleaned = True
                #print("Enough data")
                print("Distance to water surface in mm: ", self.distance)
                print("Water level in mm: ", self.water_level)
                print("Flow rate in l/s: ", self.flow_rate)

                return envelope_data
            else:
                self.collected_data_cleaned = False
                self.collected_data += serial_data # Add the serial data to the collected data
                time.sleep(0.02) # Sleep for 20 ms to allow more data to be collected
                return None

    def read_metadata(self):
        '''
        This function reads the metadata from the serial data

        Parameters
        ----------
        None

        Returns
        -------
        None
        '''

        # Find the indexes of the start, length and pipe diameter information in the collected data
        start_m_index = self.collected_data.find('Start: ') + 7
        length_m_index = self.collected_data.find('Length: ') + 8
        pipe_diameter_index = self.collected_data.find('Pipe diameter: ') + 15
        self.data_length = int(self.collected_data[self.data_length_index:self.data_length_index + 4])
        # Get the start, length and pipe diameter information from the collected data
        self.start_m = float(self.collected_data[start_m_index:start_m_index + 3])
        self.length_m = float(self.collected_data[length_m_index:length_m_index + 3])
        self.pipe_diameter = float(self.collected_data[pipe_diameter_index:pipe_diameter_index + 3])
        # Set the metadata read flag to true
        self.metadata_read_flag = True

    

    def plot_data_live(self, data, peaks):
        '''
        This function plots the data live

        Parameters
        ----------
        data : list
            The envelope data to plot
        peaks : list
            A list of the peaks in the data
        
        Returns
        -------
        None
        '''
        
        # Plot peaks as red dots
        plt.ion()  # Turn on interactive mode
        plt.clf()  # Clear the current figure
        
        # Increase spacing between subplots
        plt.subplots_adjust(hspace=1.0)

        #Set the height of the figure to double the width and make it take effect immediately
        self.fig.set_size_inches(8, 10, forward=True)

        # Plot the first plot
        plt.subplot(3, 1, 1)
        #Set plot height to 2/3 of the figure height
        for peak in peaks:
            #Plot the peaks as red dots with double the size of the other points
            plt.plot(peak[0], peak[1], 'ro', markersize=10)

        for peak in self.filtered_peaks:
            plt.plot(peak[0], peak[1], 'bo')
        
        #Calculate the x values and labels for the x axis
        x_values = np.linspace(self.start_m, self.start_m + self.length_m, self.data_length)
        labels = []
        for i in range(5):
            labels.append(str(self.start_m + (i*self.length_m)/4))
        x_labels = np.array(labels)
        x_tick_locations = [0, int(self.data_length/4), int(2*self.data_length/4), int(3*self.data_length/4), int(self.data_length)]
        plt.xticks(x_tick_locations, x_labels)
        #Add axis titles
        plt.xlabel('Distance from sensor to water surface [m]')
        plt.ylabel('Intensity')
        plt.plot(data)
        
        # Plot the second plot
        plt.subplot(3, 1, 2)
        #Add axis titles
        plt.xlabel('Time [s]')
        plt.ylabel('Water level [m]')
        #Add y axis lower limit
        plt.ylim(0, self.pipe_diameter*1.2)
        plt.plot(self.measurement_df['Time'].to_numpy(), self.measurement_df['Water level'].to_numpy())

        #Plot the third plot showing the flow rate from meassurments_df
        plt.subplot(3, 1, 3)
        #Add axis titles
        plt.xlabel('Time [s]')
        plt.ylabel('Flow rate [l/s]')
        plt.plot(self.measurement_df['Time'].to_numpy(), self.measurement_df['Flow rate'].to_numpy())
        
        plt.draw()  # Redraw the current figure

        plt.pause(0.1)  # Pause for a short period


    def log_to_file(self, serial_data):
        '''
        This function writes the data to a log file
        
        Parameters
        ----------
        serial_data : str
            The serial data to write to the file
        
        Returns
        -------
        None
        '''
        # Write the data to a file and overwrite the existing contents
        with open(self.raw_filename, 'a') as f:
            #f.write("NEW PART")
            f.write(serial_data)

            
    def log_plot_data(self):
        '''
        This function writes the plotted data to a csv file
        '''
        # Write the data to a csv file
        self.measurement_df.to_csv(self.plotted_data_filename, index=False)


    def peak_detection(self, serial_data):
        '''
        This function finds the peaks in the data
        
        Parameters
        ----------
        serial_data : list
            A list of the envelope data
        
        Returns
        -------
        peaks : list
            A list of the peaks in the data
        '''
        #Initialize the list of peaks and the previous largest intensity
        peaks= []
        peaks_with_lowest_intensity = []
        previous_largest_intensity = 0

        for i, intensity in enumerate(serial_data):
            if i > 0 and i < len(serial_data) - 1:
                #Adds the peak to the list if the intensity is greater than the previous and next intensity
                if intensity > serial_data[i - 1] and intensity > serial_data[i + 1]:
                    peak = [i, intensity]
                    peaks.append(peak)
                #Adds the peak to the list if the intensity is equal to the previous intensity and greater than the next intensity
                elif intensity == previous_largest_intensity and intensity > serial_data[i + 1]:
                    peak = [i, intensity, lowest_intensity_before]
                    peaks.append(peak)
            elif i == 0:
                lowest_intensity_before = intensity

            #Check if the intensity is greater than the previous intensity, if so, update the previous largest intensity        
            if intensity > previous_largest_intensity and intensity > serial_data[i - 1]:
                previous_largest_intensity = intensity
            #Check if the intensity is less than the previous intensity, if so, reset the previous largest intensity
            elif intensity < serial_data[i - 1]:
                previous_largest_intensity = 0
        return peaks
    
    
    # def calculate_flow_rate(self):
    #     '''
    #     This function calculates the flow rate based on the average distance to the peak
        
    #     Parameters
    #     ----------
    #     None
        
    #     Returns
    #     -------
    #     None
    #     '''
    #     D = self.pipe_diameter/1000 # Diameter of the pipe in meters
    #     r = D / 2   # Radius of the pipe in meters
    #     h = self.water_level/1000 # Height of the water level in meters
    #     if h >= D:    #If the measured water level is greater than the pipe diameter, the height is set to the pipe diameter
    #         h = D
    #     if h == 0:  # If the water level is 0, the flow rate is 0
    #         theta = 0   # Angle from the center to the segment that is submerged
    #         W_P = 0     # Wetted perimeter
    #         R = 0       # Hydraulic radius
    #         A = 0       # Area of the segment that is submerged
    #     else:   
    #         try: 
    #             theta = 2 * math.acos(1 - (h / r)) # Angle from the center to the segment that is submerged
    #         except:
    #             print("Error, could not calculate theta")
    #         W_P = D * (theta / (2 * math.pi))  # Wetted perimeter
    #         A = (W_P * h) / 2 # Area of the segment that is submerged
    #         R = A / W_P # Hydraulic radius
    #     # Mannings roughness coefficient for PVC
    #     n = self.mannings_roughness_coefficient
    #     # Calculate the flow rate
    #     self.flow_rate = (1 / n) * A * R**(2/3) * math.sqrt(self.slope)


def main():
    #Create a new instance of the RadarOperation class
    radar = RadarOperation()
    print("Initiated data collection. Waiting for metadata...")
    #Write a new part to the log file and overwrite the existing contents
    with open(radar.raw_filename, 'w') as f:
        f.write("NEW PART\n")

    #Continuously read and plot the serial data
    while True:
        # Read and log the serial data
        data = radar.read_serial_data()
        if data != None:    
            radar.log_to_file(data)
            # Parse the data
            parsed_data = radar.parse_data(data)

            # If the parsed data is not None, find peaks and plot the data
            if parsed_data != None:
                peaks = radar.peak_detection(parsed_data)
                #print(peaks)
                radar.plot_data_live(parsed_data, peaks)
                # Write the plotted data to a file with the accompanying metadata and envelope data
                radar.log_plot_data()


if __name__ == "__main__":
    main()
