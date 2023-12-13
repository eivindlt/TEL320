#This document should read serial data and find peaks in the data

import serial
import numpy as np
import matplotlib.pyplot as plt
import scipy.signal as signal

#This function reads the serial data and returns a list of the data
def read_and_log_serial_data(ser, filename):
    # Read all the serial data from stream
    serial_data = ser.read(ser.in_waiting)
    # Convert the data to a string
    serial_data = serial_data.decode('utf-8')
    # Write the data to a file
    with open(filename, 'a') as f:
        f.write(serial_data)
    # Split the data into a list
    serial_data = serial_data.split(',')
    return serial_data

#This function plots the data
def plot_data(data):
    #print(data)
    # plt.plot(data)
    # plt.show()
    pass

#This function finds the peaks in the data
def find_peaks(data):
    #Find the peaks in the data
    peaks = signal.find_peaks(data, height=0.5, distance=100)
    #Get the peak indices
    peak_indices = peaks[0]
    #Get the peak values
    peak_values = peaks[1]['peak_heights']
    #Return the peak indices and values
    return peak_indices, peak_values

#This function writes the data to a csv file

serial_port = '/dev/ttyACM0'
baud_rate = 115200
ser = serial.Serial(serial_port, baud_rate)

serial_port = '/dev/ttyACM0'
baud_rate = 115200
ser = serial.Serial(serial_port, baud_rate)

filename = 'serial_data.log'

while True:
    # Read and log the serial data
    data = read_and_log_serial_data(ser, filename)
    # Plot the data
   # plot_data(data)
    # Find the peaks in the data
    #peak_indices, peak_values = find_peaks(data)
    # Do something with the peaks
    # ...

