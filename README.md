# productionLineScheduler
Scheduling application designed to optimize production scheduling for a steel manufacturing plant.

First for executable file run: gcc -o out PLS_G17.c
Then for execute: ./out
Enter input lines such as:

addPERIOD 2024-06-01 2024-06-30

addORDER P0001 2024-06-10 2000 Product_A

addORDER P0202 2024-06-23 800 Product_H

addORDER P0002 2024-06-13 3000 Product_D

runPLS FCFS | printREPORT > report_01_FCFS.txt

for finish: exitPLS