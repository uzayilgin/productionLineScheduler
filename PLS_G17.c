#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>

#define MAX_ORDERS 100
#define MAX_ORDER_LEN 256
#define MAX_COMMAND_LEN 256

// Orders in struct
typedef struct {
    char orderNumber[20];
    char dueDate[11]; 
    int quantity;
    char productName[20];
} Order;

// Periyot in struct
typedef struct {
    char startDate[11]; 
    char endDate[11];
} Period;

typedef struct {
    char name[10];
    int capacity; // capacity of daily production
    int totalProduced; // Total products produced
    int daysInUse; // Total days in operation
} Plant;

// Define facilities and their capacities
Plant plants[] = {
    {"Plant_X", 300, 0, 0},
    {"Plant_Y", 400, 0 ,0},
    {"Plant_Z", 500, 0 ,0}
};

char lastUsedDates[3][11] = {"2024-06-01", "2024-06-01", "2024-06-01"};
char currentDate[11] = "2024-06-01";
// Period arranged according to the given input. If the added product is not within the period, it should give an error, that's why I defined it.
Period schedulingPeriod;
Order orders[MAX_ORDERS];
int orderCount = 0;

// To compare dates
int compareDates(const char *date1, const char *date2) {
// Returns -1 if date1 < date2, 0 if date1 == date2, and 1 if date1 > date2
    return strcmp(date1, date2);
}

// To add the items given according to input to orders
void addOrder(const char *orderDetails) {
    if (orderCount >= MAX_ORDERS) {
        printf("Order limit reached. Cannot add more orders.\n");
        return;
    }
    
    Order newOrder;
    sscanf(orderDetails, "%s %s %d %s", newOrder.orderNumber, newOrder.dueDate, &newOrder.quantity, newOrder.productName);
    
    // To check whether the order to be added is within the period
    if (compareDates(newOrder.dueDate, schedulingPeriod.startDate) < 0 || compareDates(newOrder.dueDate, schedulingPeriod.endDate) > 0) {
        // If the input is not within the period, it should be written to the invalid orders.txt file (the instruction asks for it like this).
        FILE *invalidFile = fopen("invalid_orders.txt", "a"); 
        if (invalidFile) {
            fprintf(invalidFile, "%s %s %d %s\n", newOrder.orderNumber, newOrder.dueDate, newOrder.quantity, newOrder.productName);
            fclose(invalidFile);
        } else {
            printf("Failed to open invalid orders log file.\n");
        }
        printf("Due date of order for %s is outside the scheduling period and has been marked as invalid.\n", newOrder.productName);
        return;
    }

    //  
    orders[orderCount++] = newOrder;
}

void addBatch(const char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        printf("Failed to open batch file: %s\n", fileName);
        return;
    }
    
    char line[MAX_ORDER_LEN];
    while (fgets(line, sizeof(line), file)) {
        // Remove the newline character at the end of the line
        line[strcspn(line, "\n")] = 0;
        
        // Check if line starts with "addORDER" and skip it
        const char *commandPrefix = "addORDER ";
        if (strncmp(line, commandPrefix, strlen(commandPrefix)) == 0) {
            // Pass the part of the line after "addORDER " to addOrder
            addOrder(line + strlen(commandPrefix));
        } else {
            printf("Invalid command format in batch file: %s\n", line);
        }
    }
    
    fclose(file);
}


// Function to set the period
void setPeriod(const char *startDate, const char *endDate) {
    // Store the start and end dates in the global schedulingPeriod
    strncpy(schedulingPeriod.startDate, startDate, sizeof(schedulingPeriod.startDate) - 1);
    schedulingPeriod.startDate[sizeof(schedulingPeriod.startDate) - 1] = '\0'; // Ensure null-termination

    strncpy(schedulingPeriod.endDate, endDate, sizeof(schedulingPeriod.endDate) - 1);
    schedulingPeriod.endDate[sizeof(schedulingPeriod.endDate) - 1] = '\0'; // Ensure null-termination
}

// Written to check whether it is present in the output or not, and can be deleted it at the end.
void printOrders() {
    if (orderCount == 0) {
        printf("No orders to display.\n");
        return;
    }

    printf("Current Orders:\n");
    printf("Order Number | Due Date   | Quantity | Product Name\n");
    //printf("----------------------------------------------------\n");
    for (int i = 0; i < orderCount; i++) {
        printf("%-13s | %-10s | %-8d | %s\n",
               orders[i].orderNumber, orders[i].dueDate, orders[i].quantity, orders[i].productName);
    }
    //printf("----------------------------------------------------\n");
}

void sortOrdersByDueDate() {
    // Sort orders by dueDate
    for (int i = 0; i < orderCount - 1; i++) {
        for (int j = 0; j < orderCount - i - 1; j++) {
            if (compareDates(orders[j].dueDate, orders[j + 1].dueDate) > 0) {
                // Swap orders
                Order temp = orders[j];
                orders[j] = orders[j + 1];
                orders[j + 1] = temp;
            }
        }
    }
}

void addDaysToDate(char* currentDate, int daysToAdd) {
    struct tm tm = {0};
    int year, month, day;

    // Sets date
    sscanf(currentDate, "%d-%d-%d", &year, &month, &day);
    tm.tm_year = year - 1900; // Set number of year
    tm.tm_mon = month - 1;    // Set number of month (starting from 0)
    tm.tm_mday = day;         // Set number of day

    // Convert time to time_t type, add days and convert back
    time_t t = mktime(&tm);
    t += (time_t)(daysToAdd * 86400); // Number of seconds per day
    struct tm *newTm = localtime(&t);

    // tm is again converted into a string in the format "YYYY-MM-DD"
    strftime(currentDate, 11, "%Y-%m-%d", newTm);
}


int findMinDays(int quantity, int *selectedPlantIndex) {
    int plantCapacities[3] = {300, 400, 500}; // capacity of Plant_X, Plant_Y, Plant_Z
    int minDays = 1000;
    *selectedPlantIndex = -1;
    // Find the facility that requires the least number of days
    for (int i = 0; i < 3; i++) {
        int daysNeeded = (int)ceil((double)quantity / plantCapacities[i]);
        if (daysNeeded < minDays) {
            minDays = daysNeeded;
            *selectedPlantIndex = i;
        }
    }
    
    return minDays; 
}

void fcfsScheduling() {
    
    printf("Date       | Product Name | Order Number | Quantity | Due Date\n");

    for (int i = 0; i < orderCount; i++) {
        int selectedPlantIndex;
        int daysNeeded = findMinDays(orders[i].quantity, &selectedPlantIndex);

        // update the days in use
        plants[selectedPlantIndex].daysInUse += daysNeeded;

        int dailyProduction = plants[selectedPlantIndex].capacity; 
        int remainingQuantity = orders[i].quantity; 
        
        char productionDate[11];
        strcpy(productionDate, lastUsedDates[selectedPlantIndex]); // start date of the production process should continue from last used date

        for (int day = 0; day < daysNeeded; day++) {
            if (remainingQuantity <= 0) {
                break; // Break the loop if tm order is rejected
            }
            // dueDate control
            if (compareDates(productionDate, orders[i].dueDate) > 0) {
                break; // If productionDate exceeded dueDate, break the loop
            }

            int producedToday = (remainingQuantity < dailyProduction) ? remainingQuantity : dailyProduction;
            remainingQuantity -= producedToday;
            plants[selectedPlantIndex].totalProduced += producedToday; // Update total produced


            printf("%s | %s    | %s        | %d      | %s\n",
                   productionDate, orders[i].productName, orders[i].orderNumber, 
                   producedToday, orders[i].dueDate);

            addDaysToDate(productionDate, 1); 
        }


        if (compareDates(productionDate, orders[i].dueDate) > 0) {
            strcpy(currentDate, orders[i].dueDate); 
            addDaysToDate(currentDate, 1);
        } else {
            strcpy(currentDate, productionDate); 
        }

        // Update the last used date to be the last day of production or due date, whichever is earlier
        if (compareDates(productionDate, orders[i].dueDate) > 0) {
            strcpy(lastUsedDates[selectedPlantIndex], orders[i].dueDate);
        } else {
            strcpy(lastUsedDates[selectedPlantIndex], productionDate);
        }
    }

}

// helper function for sjf scheduling
//sorting the orders by their quantity in ascending order 
//to ensure that the shortest jobs are processed first. 
// Function to calculate the job length in days for the given quantity
int calculateJobLength(int quantity) {
    int minDays = INT_MAX;
    for (int i = 0; i < 3; i++) {
        int daysNeeded = (int)ceil((double)quantity / plants[i].capacity);
        if (daysNeeded < minDays) {
            minDays = daysNeeded;
        }
    }
    return minDays;
}

void sortOrdersByJobLength() {
    // Sort orders by the length of the job in days
    for (int i = 0; i < orderCount - 1; i++) {
        for (int j = 0; j < orderCount - i - 1; j++) {
            int jobLength1 = calculateJobLength(orders[j].quantity);
            int jobLength2 = calculateJobLength(orders[j + 1].quantity);
            if (jobLength1 > jobLength2) {
                Order temp = orders[j];
                orders[j] = orders[j + 1];
                orders[j + 1] = temp;
            }
        }
    }
}

void sjfScheduling() {
    sortOrdersByJobLength();
    fcfsScheduling(); // Now the orders are sorted by job length, we can schedule as FCFS
}

/*
void priorityScheduling() {
    // Since orders are already sorted by due date in sortOrdersByDueDate(), 
    // this effectively treats the order with the closest due date as the highest priority.
    fcfsScheduling(); // Call FCFS after sorting, which is essentially priority scheduling in this case.
}
*/

// Helper function to get priority of product category
int getProductPriority(const char* productName) {
    // Assuming product name starts with the category letter
    switch (productName[0]) {
        case 'A': // Category 1 has the highest priority
        case 'B':
        case 'C':
            return 1;
        case 'D': // Category 2
        case 'E':
        case 'F':
            return 2;
        case 'G': // Category 3 has the lowest priority
        case 'H':
        case 'I':
            return 3;
        default:
            return 4; // Unknown category
    }
}

void sortOrdersByPriority() {
    // Sort orders by product category priority
    for (int i = 0; i < orderCount - 1; i++) {
        for (int j = 0; j < orderCount - i - 1; j++) {
            int priority1 = getProductPriority(orders[j].productName);
            int priority2 = getProductPriority(orders[j + 1].productName);
            if (priority1 > priority2) {
                Order temp = orders[j];
                orders[j] = orders[j + 1];
                orders[j + 1] = temp;
            }
        }
    }
}

void priorityScheduling() {
    sortOrdersByPriority();
    fcfsScheduling(); // Now the orders are sorted by priority, we can schedule as FCFS
}

void printReport(const char *fileName, const char *algoName) {
    FILE *reportFile = fopen(fileName, "a"); 
    
    if (!reportFile) {
        printf("Failed to open report file.\n");
        return;
    }
    fprintf(reportFile, "\nAlgorithm used: %s \n", algoName); 
    fprintf(reportFile, "\nDate       | Product Name | Order Number | Quantity | Due Date\n\n");
    
   for (int i = 0; i < orderCount; i++) {
        fprintf(reportFile, "%s | %s    | %s        | %d      | %s\n",
               currentDate, orders[i].productName, orders[i].orderNumber,
               orders[i].quantity, orders[i].dueDate);
    }

    fprintf(reportFile, "\n** PERFORMANCE \n\n");

    int totalPossibleProduction = 0;
    int totalActualProduction = 0;
    for (int i = 0; i < 3; i++) { // Assuming there are 3 plants
        Plant plant = plants[i];
        double utilization = (double)plant.totalProduced / (plant.capacity * plant.daysInUse) * 100.0;
        
        // Accumulate total metrics
        totalPossibleProduction += plant.capacity * plant.daysInUse;
        totalActualProduction += plant.totalProduced;

        fprintf(reportFile, "%s:\n", plant.name);
        fprintf(reportFile, "Number of days in use: %d days\n", plant.daysInUse);
        fprintf(reportFile, "Number of products produced: %d (in total)\n", plant.totalProduced);
        fprintf(reportFile, "Utilization of the plant: %.2f %%\n", utilization);
        fprintf(reportFile, "\n");
    }

     // Calculate and print overall utilization
    double overallUtilization = 0;
    if (totalPossibleProduction > 0) {
        overallUtilization = (double)totalActualProduction / totalPossibleProduction * 100.0;
    }
    fprintf(reportFile, "Overall of utilization: %.2f %%\n", overallUtilization);
    fclose(reportFile);
}

int main() {
    char command[MAX_COMMAND_LEN];
    printf("~~WELCOME TO PLS~~\n");

    int fd[2];
    pid_t pid;

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) { // Child process
        close(fd[1]); // Close the write end
        char buffer[MAX_COMMAND_LEN + 1];

        while (read(fd[0], buffer, MAX_COMMAND_LEN) > 0) {
            buffer[MAX_COMMAND_LEN] = '\0'; // Ensure null termination
            char *command = strtok(buffer, "|");
            char algorithm[50] = {0};

            while (command) {
                if (strncmp(command, "addORDER ", 9) == 0) {
                    addOrder(command + 9);
                } else if (strncmp(command, "addBATCH ", 9) == 0) {
                    char fileName[256];
                    sscanf(command + 9, "%s", fileName); // Extract filename from the command
                    addBatch(fileName);
                } else if (strncmp(command, "addPERIOD ", 10) == 0) {
                    char startDate[11], endDate[11];
                    sscanf(command + 10, "%s %s", startDate, endDate); // Skip "addPERIOD " part
                    strcpy(currentDate, startDate);
                    setPeriod(startDate, endDate);
                } else if (strncmp(command, "exitPLS", 7) == 0) {
                    printf("Bye-bye!\n");
                    exit(0); // Exit child process
                } else if (strncmp(command, "runPLS FCFS", 11) == 0) {
                    fcfsScheduling();
                    strcpy(algorithm, "First-Come-First-Served");
                } else if (strncmp(command, "runPLS PR", 9) == 0) {
                    priorityScheduling();
                    strcpy(algorithm, "Priority Queue");
                } else if (strncmp(command, "runPLS SJF", 10) == 0) {
                    sjfScheduling();
                    strcpy(algorithm, "Shorter-Come-First");
                } else if (strncmp(command, " printREPORT >", 14) == 0) {
                    char fileName[256];
                    sscanf(command + 15, "%s", fileName);
                    printReport(fileName, algorithm);
                } else {
                    printf("Unknown command: %s\n", command);
                }
                command = strtok(NULL, "|");
            }
        }
        close(fd[0]);
    } else { // Parent process
        close(fd[0]); // Close the read end
        while (1) {
            printf("Please enter: \n> ");
            if (!fgets(command, MAX_COMMAND_LEN, stdin)) {
                printf("Error reading input.\n");
                continue;
            }

            if (strncmp(command, "exitPLS", 7) == 0) {
                write(fd[1], command, strlen(command));
                break;
            }

            write(fd[1], command, strlen(command));
        }
        close(fd[1]); // Signal EOF to the child
        wait(NULL); // Wait for the child process to finish
    }

    return 0;
}