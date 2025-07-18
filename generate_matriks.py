import random
import csv

def generate_csv(filename, size=3000):
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        for _ in range(size):
            row = [random.randint(1, 10) for _ in range(size)]
            writer.writerow(row)

generate_csv("file_1.csv")
generate_csv("file_2.csv")
