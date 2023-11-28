#This document should read serial data and find peaks in the data

import serial
import numpy as np
import matplotlib.pyplot as plt
import time

#This function reads the serial data and returns a list of the data
class RadarOperation:
    def __init__(self) -> None:
        self.serial_port = '/dev/ttyACM0'
        self.baud_rate = 115200
        self.ser = serial.Serial(self.serial_port, self.baud_rate)
        self.raw_filename = 'serial_data1.log'
        self.distance_to_peak_filename = 'distance_to_peak.log'
        self.collected_data = ""
        self.fig, self.ax = plt.subplots()
        self.data_length = 0
        self.data_length_factor = 6.125
        self.start_m = 0.0
        self.length_m = 0.0
        self.average_distance = 0.0
        self.average_distance_updated = False
        self.average_index = 0
        self.collected_data_cleaned = False
        

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

        #Find the start index of the envelope data
        start_index = self.collected_data.find('Envelope data:\n') + 15
        #Find the data length
        data_length_index = self.collected_data.find('Data length:') + 13
        # Find start_m and length_m
        start_m_index = self.collected_data.find('Start: ') + 7
        length_m_index = self.collected_data.find('Length: ') + 8
        self.average_index = self.collected_data.find('Average distance to filtered peaks for all iterations: ') + 55
        if self.average_index != 54:
            try:
                self.average_distance = float(self.collected_data[self.average_index:self.average_index + 4])
                self.calculate_flow_rate()
                self.average_distance_updated = True
            except:
                print("Error, could not convert average distance to float")
        # Check if start_m_index is out of range
        if self.start_m == 0.0 and start_m_index != 6:
            try:
                self.start_m = float(self.collected_data[start_m_index:start_m_index + 3])
            except:
                print("Error, could not convert start_m to float")
        # Check if length_m_index is out of range
        if self.length_m == 0.0 and length_m_index != 7:
            try:
                self.length_m = float(self.collected_data[length_m_index:length_m_index + 3])
            except:
                print("Error, could not convert length_m to float")
        #print("Start: ", self.start_m)
        #print("Length: ", self.length_m)   
        if self.data_length == 0 and data_length_index != 12:
            self.data_length = int(self.collected_data[data_length_index:data_length_index + 4])
        if data_length_index != 12 or self.data_length != 0:
            end_index = start_index + int(self.data_length*self.data_length_factor)
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
                    #print("Collected data: ", self.collected_data)
                    self.collected_data = ""
                    self.collected_data_cleaned = True
                    time.sleep(0.5)
                    return None
                self.collected_data = ""
                self.collected_data_cleaned = True
                print("Enough data")
                print("Average distance: ", self.average_distance)
                return envelope_data
            else:
                self.collected_data_cleaned = False
        
        self.collected_data += serial_data # Add the serial data to the collected data
        time.sleep(0.02) # Sleep for 20 ms to allow more data to be collected
        return None
    
    # def calculate_flow_rate(self): #Based on hydraulic diameter and an assumption that the pipe is at an 5 deg inclination
    #     hydraulic_diameter = 0.0254
    #     inclination = 5
    #     area = hydraulic_diameter**2*np.pi/4
    #     #Velocity is based on hydrraulic diameter and 
    #     velocity = self.average_distance/1000/(1/1000)
    #     flow_rate = area*velocity
    #     print("Flow rate: ", flow_rate)

    
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

        #Plot peaks as red dots'
        plt.ion() # Turn on interactive mode
        plt.clf() # Clear the current figure
        for peak in peaks:
            plt.plot(peak[0], peak[1], 'ro')
            #print("Peak: ", peak)
        #Create a list of the x values from start_m to length_m
        x_values = np.linspace(self.start_m, self.start_m + self.length_m, self.data_length)
        #Create a 2D numpy array of the data and the x values
    
        labels = []
        for i in range(5):
            labels.append(str(self.start_m + i*self.length_m/4))
 
        x_labels = np.array(labels)
        x_tick_locations = [0, int(self.data_length/4), int(2*self.data_length/4), int(3*self.data_length/4), int(self.data_length)]
        plt.xticks(x_tick_locations, x_labels)
        #Plot the data
        plt.plot(data)
        plt.draw() # Redraw the current figure
        plt.pause(0.1) # Pause for a short period

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

    def log_distance_to_peak(self):
            if self.average_distance_updated == True and self.collected_data_cleaned == True:
                with open(self.distance_to_peak_filename, 'a') as f:
                    f.write(str(self.average_distance) + "\n")
                self.average_index = 0
                self.average_distance_updated = False


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

def main():
    #Create a new instance of the RadarOperation class
    radar = RadarOperation()
    #Write a new part to the log file and overwrite the existing contents
    with open(radar.raw_filename, 'w') as f:
        f.write("NEW PART\n")
    with open(radar.distance_to_peak_filename, 'w') as f:
        f.write("Average distances in mm\n")

    #Continuously read and plot the serial data
    while True:
        # Read and log the serial data
        data = radar.read_serial_data()
        if data != None:    
            radar.log_to_file(data)
            # Parse the data
            parsed_data = radar.parse_data(data)
            #Log the distance to peak
            radar.log_distance_to_peak()
            # If the parsed data is not None, find peaks and plot the data
            if parsed_data != None:
                peaks = radar.peak_detection(parsed_data)
                #print(peaks)
                radar.plot_data_live(parsed_data, peaks)
            #Pause for 0.1 seconds

main()
