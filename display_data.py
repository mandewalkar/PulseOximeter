import csv
import pandas as pd
import time
import matplotlib.pyplot as plt

def read_csv_and_plot(file_path):
    #create a list to populate with each column's data
    num_columns = 3
    column_data = [[] for _ in range(num_columns)]

    plt.ion() #interactive plot

    orig_df = pd.read_csv(file_path)
    for idx, row in orig_df.iterrows(): #add each row in original csv to the column lists
        if row[0] == '32767.////': #garbage sensor value
            break
        print(row[0])
        column_data[0].append(float(row[0]))
        column_data[1].append(float(row[1]))
        column_data[2].append((float(row[2]) * 50) + 50)
    while True:
            df = pd.read_csv(file_path) # constantly read in the csv for updates
            if df.shape[0] != orig_df.shape[0]: #if there are any new values, update the lists
                for i in range (orig_df.shape[0], df.shape[0]):
                    if df.iloc[i,0] == '32767.////':
                        break
                    column_data[0].append(float(df.iloc[i,0]))
                    column_data[1].append(float(df.iloc[i,1]))
                    column_data[2].append((float(df.iloc[i,2]) * 50) + 50)
                    print(float(df.iloc[i,1]))
                plt.clf() #clear plot to rewrite
                names = ['Hemoglobin Ratio', 'SpO2', 'Presenting Data Status']
                for i in range(1, num_columns):
                    print(len(column_data[i]))
                    plt.plot(column_data[i], label=names[i]) #plot spo2 and status 
                    spo2 = str(column_data[1][-1]) #get spo2 value to be part of the title
                    status = column_data[2][-1]
                    if (status == 100): #only display status during the valid parts
                        plt.title('Real-time SpO2 Tracking, Current SpO2: '+spo2)
                    else:
                        plt.title('Real-time SpO2 Tracking, Calculating...')
                    plt.show()
                plt.legend()
                plt.xlabel('Time')
                plt.ylabel('Value')
                plt.draw()
                plt.pause(.1)  #pause to allow the plot to update
                orig_df = df
                time.sleep(.1)  #wait a bit before reading the csv again


# Example usage
if __name__ == "__main__":
    file_path = "oximeter.csv"
    read_csv_and_plot(file_path)
