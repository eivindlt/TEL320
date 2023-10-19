import re

import matplotlib.pyplot as plt
import numpy as np

def plot_heatmap(enveloped_data_lists):
    # Find the maximum length of the lists
    max_length = max(len(lst) for lst in enveloped_data_lists)
    # Pad the shorter lists with zeros
    padded_data = [lst + [0]*(max_length - len(lst)) for lst in enveloped_data_lists]
    # Convert the list of lists to a 2D numpy array and transpose it
    data = np.array(padded_data).T
    # Create a new figure
    fig, ax = plt.subplots()
    # Create a heatmap plot
    im = ax.imshow(data, cmap='viridis')
    # Set the x-axis label
    ax.set_xlabel('Intensity')
    #Set the y-axis start and end value labes from 200 to 500
    ax.set_yticks([200, 300, 400, 500])

    # Set the y-axis label
    ax.set_ylabel('Envelope')
    # Add a colorbar
    cbar = ax.figure.colorbar(im, ax=ax)
    # Show the plot
    plt.show()


filename = 'serial_data.log'

# Read the contents of the file
with open(filename, 'r') as f:
    file_contents = f.read()

# Find the start and end indices of the all the instances of envelope data
start_indices = [m.start() for m in re.finditer('Envelope data:\n', file_contents)]

#Use list comprehension to modify the start indices to account for the length of the string
start_indices = [i + 15 for i in start_indices]
print(len(start_indices))


enveloped_data_lists = []

for start_index in start_indices[1:]:
    end_index = start_index + 6327
    # Get the envelope data
    envelope_data_str = file_contents[start_index:end_index]
    # Split the envelope data into a list of integers
    envelope_data = list(map(int, envelope_data_str.split()))
    # Add the envelope data to the list of lists
    enveloped_data_lists.append(envelope_data)

# Print the resulting list
#print(enveloped_data_lists)
print(len(enveloped_data_lists))

# Plot the intensity data
plot_heatmap(enveloped_data_lists)