import matplotlib.pyplot as plt


def read_data(file_name):
    y_values = []

    # Open the file and read each line
    with open(file_name, 'r') as file:
        for line in file:
            # Each line contains a single y value
            try:
                y_values.append(float(line.strip()))
            except ValueError:
                continue  # Skip lines that don't contain valid numbers

    # Generate x values (indices of the y values)
    x_values = list(range(len(y_values)))

    return x_values, y_values


def plot_data(x_values, y_values):
    plt.plot(x_values, y_values, label="Data")
    plt.xlabel("Index")
    plt.ylabel("Y Value")
    plt.title("Plot of Y Values")
    plt.legend()
    plt.show()


# File name containing only y values
file_name = "output.txt"

# Read the data
x_values, y_values = read_data(file_name)

# Plot the data
plot_data(x_values, y_values)
