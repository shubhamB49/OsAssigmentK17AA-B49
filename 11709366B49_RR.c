	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <time.h>
	

	#define TRUE 1
	FILE *log_file;
	

	process processes[MAX_PROCESSES + 1];
	int number_of_processes;
	int incoming_process;
	int context_switches;
	int simulation_time;
	int last_process_end_time = 0;
	int cpu_utilized_time;
	int time_slice = 0;
	process_queue readyQueue;
	process_queue waitingQueue;
	process_queue request_queue;
	

	
	process *CPU[NUMBER_OF_PROCESSORS];
	

	
	process *tmp_ready_process[MAX_PROCESSES + 1];
	process *saved_process[NUMBER_OF_PROCESSORS];
	int tmp_ready_process_size;
	

	void init_(void) {
	    number_of_processes = 0;
	    incoming_process = 0;
	    context_switches = 0;
	    cpu_utilized_time = 0;
	    simulation_time = 0;
	    tmp_ready_process_size = 0;
	}
	

	int compareProcessIds(const void *a, const void *b) {
	    process *one = *((process**) a);
	    process *two = *((process**) b);
	    if (one->pid < two->pid)
	        return -1;
	    if (one->pid > two->pid)
	        return 1;
	

	    return 0;
	}
	




	void running_process_to_waiting() {
	    int i, j;
	    j = 0;
	    for (i = 0; i < NUMBER_OF_PROCESSORS; i++) {
	        if (CPU[i] != NULL) {
	

	            fprintf(log_file, "STEPS:%d\n", CPU[i]->bursts[CPU[i]->currentBurst].step);
	            // at some point step will be equal to length because length gets reduced
	            if (CPU[i]->bursts[CPU[i]->currentBurst].step == CPU[i]->bursts[CPU[i]->currentBurst].length) {
	                fprintf(log_file, "DONE PROCESS ID:%d\n", CPU[i]->pid);
	                CPU[i]->currentBurst++;
	                if (CPU[i]->currentBurst < CPU[i]->numberOfBursts) {
	                    enqueueProcess(&waitingQueue, CPU[i]); // Puts the finish Burst into the waiting queue
	

	                    fprintf(log_file, "Waiting process id:%d\t&& Current Process id:%d\n", waitingQueue.front->data->pid, CPU[i]->pid);
	                } else {
	                    CPU[i]->endTime = simulation_time;
	                    last_process_end_time = CPU[i]->endTime;
	                }
	                CPU[i] = NULL;
	            } else if (CPU[i]->quantumRemaining == 0) { 
	                fprintf(log_file, "** Context Switching is in process  . . . . . . .**\n");
	                int rem_time = CPU[i]->bursts[CPU[i]->currentBurst].length - CPU[i]->bursts[CPU[i]->currentBurst].step;
	                CPU[i]->remainingTime = rem_time;
	                saved_process[j] = CPU[i];
	                fprintf(log_file, "PID\t:%d is Pre-empted\n", saved_process[j]->pid);
	                context_switches++;
	                saved_process[j]->quantumRemaining = time_slice;
	                fprintf(log_file, "Remaining Length of Process\t:%d at Current Burst of\t:%d is\t:%d\n", saved_process[j]->pid, saved_process[j]->currentBurst, saved_process[j]->remainingTime);
	                fprintf(log_file, "*** Pre-emption is Done ***\n");
	                j++;
	                CPU[i] = NULL; 
	            }
	        }
	    }
	    
	    qsort(saved_process, j, sizeof (process*), compareProcessIds);
	    //qsort(saved_process, j, sizeof (process*), compareByArrival);
	    for (i = 0; i < j; i++) {
	        enqueueProcess(&readyQueue, saved_process[i]);
	    }
	}
	




	int runningProcesses() {
	    int runningProcesses = 0;
	    int i;
	    for (i = 0; i < NUMBER_OF_PROCESSORS; i++) {
	        if (CPU[i] != NULL) {
	            runningProcesses++;
	        }
	    }
	    return runningProcesses;
	}
	




	process *get_next_sch_process() {
	    if (readyQueue.size == 0) {
	        return NULL;
	    }
	    process *cpu_ready = readyQueue.front->data;
	    dequeueProcess(&readyQueue);
	    return cpu_ready;
	}
	




	void incoming_process_init() {
	    while (incoming_process < number_of_processes && processes[incoming_process].arrivalTime <= simulation_time) {
	        tmp_ready_process[tmp_ready_process_size] = &processes[incoming_process];
	        tmp_ready_process[tmp_ready_process_size]->quantumRemaining = time_slice; 
	        tmp_ready_process_size++;
	        incoming_process++;
	    }
	}
	




	void most_ready_running_in_cpu() {
	    int i = 0;
	    qsort(tmp_ready_process, tmp_ready_process_size, sizeof (process*), compareProcessIds);
	    for (i = 0; i < tmp_ready_process_size; i++) {
	        enqueueProcess(&readyQueue, tmp_ready_process[i]);
	    }
	    
	    tmp_ready_process_size = 0;
	    for (i = 0; i < NUMBER_OF_PROCESSORS; i++) {
	        if (CPU[i] == NULL) {
	            CPU[i] = get_next_sch_process();
	        }
	    }
	}
	

	void waiting_to_ready() {
	    int i = 0;
	    int waiting_size = waitingQueue.size;
	    for (i = 0; i < waiting_size; i++) {
	        process *ready = waitingQueue.front->data;
	        dequeueProcess(&waitingQueue);
	        //dequeueProcess(&request_queue);
	        if (ready->bursts[ready->currentBurst].step == ready->bursts[ready->currentBurst].length) {
	            ready->currentBurst++;
	            ready->quantumRemaining = time_slice; 
	            tmp_ready_process[tmp_ready_process_size++] = ready;
	        } else {
	            enqueueProcess(&waitingQueue, ready);
	        }
	    }
	}
	

	void increase_waitingTime() {
	    int j;
	    // update CPU BOUND waiting process 
	    for (j = 0; j < readyQueue.size; j++) {
	        process *CPU_BOUND = readyQueue.front->data;
	        dequeueProcess(&readyQueue);
	        CPU_BOUND->waitingTime++; 
	        enqueueProcess(&readyQueue, CPU_BOUND);
	    }
	}
	
	

	void increase_cpu_work() {
	    int j;
	    for (j = 0; j < NUMBER_OF_PROCESSORS; j++) {
	        if (CPU[j] != NULL) {
	            CPU[j]->bursts[CPU[j]->currentBurst].step++; 
	            CPU[j]->quantumRemaining--; 
	        }
	    }
	}
	

	void increase_io_work() {
	    int j;
	    int size = waitingQueue.size;
	
	    for (j = 0; j < size; j++) {
	        process *I_O = waitingQueue.front->data;
	        dequeueProcess(&waitingQueue);
	        I_O->bursts[I_O->currentBurst].step++;
	        enqueueProcess(&waitingQueue, I_O);
	    }
	}
	

	int ex() {
	    return number_of_processes - incoming_process;
	}
	

	int main(int argc, char **argv) {
	    if (argc < 2) {
	        fprintf(stderr, "Please enter a time quantum . . . . \n");
	        exit(1);
	    }
	    time_slice = atoi(argv[1]);
	    if (time_slice <= 0) {
	        fprintf(stderr, "program usage rr <time quantum> < data file . . .\n");
	        exit(1);
	    }
	    init_();
	    clock_t ticks;
	    int i;
	    int status = 0;
	    log_file = fopen("log_file.txt", "w+");
	    if (log_file == NULL) {
	        fprintf(stderr, "LOG FILE CANNOT BE OPEN\n");
	    }
	    
	    for (i = 0; i < NUMBER_OF_PROCESSORS; i++) {
	        CPU[i] = NULL;
	    }
	    initializeProcessQueue(&readyQueue);
	    initializeProcessQueue(&waitingQueue);
	    initializeProcessQueue(&request_queue);
	    
	    while ((status = (readProcess(&processes[number_of_processes])))) {
	        if (status == 1) {
	            number_of_processes++;
	        }
	    }
	    if (number_of_processes > MAX_PROCESSES) {
	        return -2;
	    }
	    if (number_of_processes == 0) {
	        return -1;
	    }
	    int remaining_process = 0;
	    
	    qsort(processes, number_of_processes, sizeof (process), compareByArrival);
	
	    while (TRUE) {
	        ticks = clock();
	        waiting_to_ready();
	        incoming_process_init();
	        running_process_to_waiting();
	        most_ready_running_in_cpu();
	

	        increase_waitingTime();
	        increase_io_work();
	        increase_cpu_work();
	

	        cpu_utilized_time += runningProcesses();
	        remaining_process = ex();
	        
	        if (remaining_process == 0 && runningProcesses() == 0 && waitingQueue.size == 0) {
	            break;
	        }
	        simulation_time++;
	    }
	    int total_waiting_time = 0;
	    int turn_around_time = 0;
	    for (i = 0; i < number_of_processes; i++) {
	        turn_around_time += processes[i].endTime - processes[i].arrivalTime;
	        total_waiting_time += processes[i].waitingTime;
	    }
	    printf(">>>>>>>>>>ROUND ROUBIN WITH A QUANTUM\t:%d<<<<<<<<<\n", time_slice);
	    printf("********************************************************************\n");
	    printf("Average Waiting Time\t\t\t:%.2f\n", total_waiting_time / (double) number_of_processes);
	    printf("Average Turn Around Time\t\t:%.2f\n", turn_around_time / (double) number_of_processes);
	    printf("Time all for all CPU processes\t\t:%d\n", simulation_time);
	    printf("CPU Utilization Time\t\t\t:%.2f%c\n", (double) (cpu_utilized_time * 100.0) / (double) (simulation_time), (int) 37);
	    printf("Total Number of Context Switches\t:%d\n", context_switches);
	    printf("Last Process to finish ");
	    for (i = 0; i < number_of_processes; i++) {
	        if (processes[i].endTime == simulation_time) {
	            printf("PID\t\t:%d\n", processes[i].pid);
	        }
	    }
	    printf("********************************************************************\n");
	    double cpu_time = (double) ticks / CLOCKS_PER_SEC;
	    fprintf(log_file, "CPU Time\t:%.2fmilliseconds\n", cpu_time);
	    fclose(log_file);
	    return 0;
	}


