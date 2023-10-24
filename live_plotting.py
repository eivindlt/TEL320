#This document should read serial data and find peaks in the data

import serial
import numpy as np
import matplotlib.pyplot as plt
# import time
# import csv
# import os
# import sys
# import math
import scipy.signal as signal
import re 
import time

#This function reads the serial data and returns a list of the data
class RadarOperation:
    def __init__(self) -> None:
        self.serial_port = '/dev/ttyACM0'
        self.baud_rate = 115200
        self.ser = serial.Serial(self.serial_port, self.baud_rate)
        self.filename = 'serial_data1.log'
        self.collected_data = ""
        self.fig, self.ax = plt.subplots()
        self.data_length = 0
        self.data_length_factor = 6.125
        

    def read_serial_data(self):
        # Read all the serial data from stream
        serial_data = self.ser.read(self.ser.in_waiting)
        # Convert the data to a string
        serial_data = serial_data.decode('utf-8')
        #print(serial_data)
        # Split the data into a list
        #serial_data = serial_data.split(',')
        return serial_data

    #This function plots the data
    def parse_data(self, serial_data):
        #Find the data length
        start_index = self.collected_data.find('Envelope data:\n') + 15
        data_length_index = self.collected_data.find('Data length:') + 13
        if self.data_length == 0 and data_length_index != 12:
            self.data_length = int(self.collected_data[data_length_index:data_length_index + 4])
        if data_length_index != 12 or self.data_length != 0:
            print("Data length: ", self.data_length)
            end_index = start_index + int(self.data_length*self.data_length_factor)
            if start_index != 14 and len(self.collected_data) > end_index and self.data_length != 0:   
                # Get the envelope data
                envelope_data_str = self.collected_data[start_index:end_index]
                #print(envelope_data_str)
                # Split the envelope data into a list of integers
                try:
                    envelope_data = list(map(int, envelope_data_str.split()))
                except:
                    print("Error")
                    print("Start index: ", start_index)
                    print("End index: ", end_index)
                    print("Length of collected data: ", len(self.collected_data))
                    #print("Collected data: ", self.collected_data)
                    self.collected_data = ""
                    time.sleep(0.5)
                    return None
                self.collected_data = ""
                print("Enough data")
                return envelope_data
        
        self.collected_data += serial_data
        print("Not enough data")
        #print(self.collected_data)
        return None


    

            # Find the start and end indices of the all the instances of envelope data
        #start_indices = [m.start() for m in re.finditer('Envelope data:\n', serial_data)]

        #Use list comprehension to modify the start indices to account for the length of the string
        #start_indices = [i + 15 for i in start_indices]


        # enveloped_data_lists = []

        # for start_index in start_indices[1:]:
        #     end_index = start_index + 6327
        #     # Get the envelope data
        #     envelope_data_str = serial_data[start_index:end_index]
        #     # Split the envelope data into a list of integers
        #     envelope_data = list(map(int, envelope_data_str.split()))
        
        #     # Add the envelope data to the list of lists
        #     enveloped_data_lists.append(envelope_data)
        #return enveloped_data_lists

    # def plot_data_live(self, enveloped_data_lists):
    #     data = np.array(enveloped_data_lists[0]).T
    #     # Create a new figure
    #     fig, ax = plt.subplots()
    #     # Create a heatmap plot
    #     im = ax.imshow(data, cmap='viridis')
    #     # Set the x-axis label
    #     ax.set_xlabel('Intensity')
    #     #Set the y-axis start and end value labes from 200 to 500
    #     ax.set_yticks([200, 300, 400, 500])

    #     # Set the y-axis label
    #     ax.set_ylabel('Envelope')
    #     # Add a colorbar
    #     cbar = ax.figure.colorbar(im, ax=ax)
    #     # Show the plot
    #     plt.show()
        
    #     # Fix problem 1: Close the figure to free up memory
    #     plt.close(fig)

        # Fix problem 2: Remove unused variable "cbar"
    
    def plot_data_live(self, data):
        plt.ion() # Turn on interactive mode
        plt.clf() # Clear the current figure
        #Set x axis labels from 50 200 for all the values in the list
        
        plt.plot(data) # Plot the data
        plt.draw() # Redraw the current figure
        plt.pause(0.1) # Pause for a short period

    def log_to_file(self, serial_data):
        # Write the data to a file and overwrite the existing contents
        with open(self.filename, 'a') as f:
            #f.write("NEW PART")
            f.write(serial_data)
    def peak_detection(self, serial_data):
        for i in range(len(serial_data)):
            if serial_data[i] > 200:
                print("Peak detected at index: ", i)
                return i


    #This function writes the data to a csv file

def main():
    radar = RadarOperation()
    with open(radar.filename, 'w') as f:
        f.write("NEW PART")
    while True:
        # Read and log the serial data
        data = radar.read_serial_data()
        radar.log_to_file(data)
        # print(data)
        # # Parse the data
        parsed_data = radar.parse_data(data)
        if parsed_data != None:
            radar.plot_data_live(parsed_data)
        #print(parsed_data)
        #print(parsed_data)
        #Pause for 0.1 seconds
        time.sleep(0.1)
        # # Plot the data
        #plot_data_live(parsed_data)

main()
