// COMP3511 Spring 2023
// PA2: Completely Fair Scheduler
//
// Your name:
// Your ITSC email: phleungaf@connect.ust.hk
//
// Declaration:
//
// I declare that I am not involved in plagiarism
// I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DISPLAY_VRUNTIME 0
#define MAX_PROCESS 10
#define SPACE_CHARS " \t"

#define KEYWORD_NUM_PROCESS "num_process"
#define KEYWORD_SCHED_LATENCY "sched_latency"
#define KEYWORD_MIN_GRANULARITY "min_granularity"
#define KEYWORD_BURST_TIME "burst_time"
#define KEYWORD_NICE_VALUE "nice_value"



const char template_key_value_pair[] = "%s = %d\n";
const char template_parsed_values[] = "=== CFS input values ===\n";
const char template_cfs_algorithm[] = "=== CFS algorithm ===\n";
const char template_step_i[] = "=== Step %d ===\n";
const char template_gantt_chart[] = "=== Gantt chart ===\n";

static const int DEFAULT_WEIGHT = 1024;
static const int NICE_TO_WEIGHT[40] = {
    88761, 71755, 56483, 46273, 36291, 
    29154, 23254, 18705, 14949, 11916, 
    9548, 7620, 6100, 4904, 3906,      
    3121, 2501, 1991, 1586, 1277,      
    1024, 820, 655, 526, 423,          
    335, 272, 215, 172, 137,           
    110, 87, 70, 56, 45,               
    36, 29, 23, 18, 15,                
};


struct CFSProcess
{
    int weight;      
    double vruntime;
    int remain_time; 
    int time_slice;  
};


#define MAX_GANTT_CHART 300
struct GanttChartItem
{
    int pid;
    int duration;
};

int num_process = 0;
int sched_latency = 0;
int min_granularity = 0;
int burst_time[MAX_PROCESS] = {0}; 
int nice_value[MAX_PROCESS] = {0}; 
struct CFSProcess process[MAX_PROCESS];
struct GanttChartItem chart[MAX_GANTT_CHART];
int num_chart_item = 0;
int finish_process_count = 0; 


void parse_input();         // Given, parse the input and store the parsed values to global variables
void print_parsed_values(); // Given, display the parsed values
void print_cfs_process();   // Given, print the CFS processes
void init_cfs_process();    // TODO, initialize the CFS process table
void run_cfs_scheduling();  // TODO, run the CFS scheduler
void gantt_chart_print();   // Given, display the final Gantt chart



int main()
{
    parse_input();
    print_parsed_values();
    init_cfs_process();
    run_cfs_scheduling();
    gantt_chart_print();
    return 0;
}

void init_cfs_process()
{
    int i;
    double min_weight = NICE_TO_WEIGHT[19];
    for (i = 0; i < num_process; i++) {
        double w = (double) DEFAULT_WEIGHT / NICE_TO_WEIGHT[nice_value[i] + 20];
        process[i].weight = (int) (w * min_weight);
        process[i].vruntime = 0;
        process[i].remain_time = burst_time[i];
        process[i].time_slice = 0;
    }
}

void run_cfs_scheduling()
{
    int i;
    double min_vruntime;
    int min_vruntime_idx;
    int slice_duration;
    
    while (finish_process_count < num_process)
    {
        min_vruntime = __DBL_MAX__;
        min_vruntime_idx = -1;
        for (i = 0; i < num_process; i++)
        {
            if (process[i].remain_time > 0 && process[i].vruntime < min_vruntime)
            {
                min_vruntime = process[i].vruntime;
                min_vruntime_idx = i;
            }
        }
        
        if (min_vruntime_idx == -1) break;
        
        slice_duration = (int)(process[min_vruntime_idx].weight / ((double)process[min_vruntime_idx].weight + 1024.0) * sched_latency);
        slice_duration = slice_duration < min_granularity ? min_granularity : slice_duration;
        slice_duration = slice_duration > process[min_vruntime_idx].remain_time ? process[min_vruntime_idx].remain_time : slice_duration;
        
        process[min_vruntime_idx].vruntime += ((double)slice_duration * process[min_vruntime_idx].weight) / 1024.0;
        process[min_vruntime_idx].remain_time -= slice_duration;
        
        chart[num_chart_item].pid = min_vruntime_idx + 1;
        chart[num_chart_item].duration = slice_duration;
        num_chart_item++;
        
        if (process[min_vruntime_idx].remain_time == 0)
        {
            finish_process_count++;
        }
    }
    
    gantt_chart_print();
}



void gantt_chart_print()
{
    int t = 0, i = 0, id = 0;
    printf(template_gantt_chart);
    if (num_chart_item > 0)
    {
        printf("%d ", t);
        for (i = 0; i < num_chart_item; i++)
        {
            t = t + chart[i].duration;
            printf("P%d %d ", chart[i].pid, t);
        }
        printf("\n");
    }
    else
    {
        printf("The gantt chart is empty\n");
    }
}

int is_blank(char *line)
{
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch))
            return 0;
        ch++;
    }
    return 1;
}

int is_skip(char *line)
{
    if (is_blank(line))
        return 1;
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}


void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void parse_input()
{
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2];               // buffer for 2 tokens
    char *process_tokens[MAX_PROCESS]; // buffer for n tokens
    int numTokens = 0, n = 0, i = 0;
    char equal_plus_spaces_delimiters[5] = "";

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters, SPACE_CHARS);

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        if (is_skip(line) == 0)
        {
            line = strtok(line, "\n");
            if (strstr(line, KEYWORD_NUM_PROCESS))
            {
                // parse num_process
                read_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &num_process);
                }
            }
            else if (strstr(line, KEYWORD_SCHED_LATENCY))
            {
                // parse sched_latency
                read_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &sched_latency);
                }
            }
            else if (strstr(line, KEYWORD_MIN_GRANULARITY))
            {
                // parse min_granularity
                read_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &min_granularity);
                }
            }
            else if (strstr(line, KEYWORD_BURST_TIME))
            {

                // parse the burst_time
                // note: we parse the equal delimiter first
                read_tokens(two_tokens, line, &numTokens, "=");
                if (numTokens == 2)
                {
                    // parse the second part using SPACE_CHARS
                    read_tokens(process_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(process_tokens[i], "%d", &burst_time[i]);
                    }
                }
            }
            else if (strstr(line, KEYWORD_NICE_VALUE))
            {
                // parse the nice_value
                // note: we parse the equal delimiter first
                read_tokens(two_tokens, line, &numTokens, "=");
                if (numTokens == 2)
                {
                    // parse the second part using SPACE_CHARS
                    read_tokens(process_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(process_tokens[i], "%d", &nice_value[i]);
                    }
                }
            }
        }
    }
}

// Helper function: print an array of integer
// This function is used in print_parsed_values
void print_vec(char *name, int vec[MAX_PROCESS], int n)
{
    int i;
    printf("%s = [", name);
    for (i = 0; i < n; i++)
    {
        printf("%d", vec[i]);
        if (i < n - 1)
            printf(",");
    }
    printf("]\n");
}

void print_parsed_values()
{
    printf(template_parsed_values);
    printf(template_key_value_pair, KEYWORD_NUM_PROCESS, num_process);
    printf(template_key_value_pair, KEYWORD_SCHED_LATENCY, sched_latency);
    printf(template_key_value_pair, KEYWORD_MIN_GRANULARITY, min_granularity);
    print_vec(KEYWORD_BURST_TIME, burst_time, num_process);
    print_vec(KEYWORD_NICE_VALUE, nice_value, num_process);
}

void print_cfs_process()
{
    int i;
    if (DISPLAY_VRUNTIME)
    {
        // Print out an extra vruntime for debugging
        printf("Process\tWeight\tRemain\tSlice\tvruntime\n");
        for (i = 0; i < num_process; i++)
        {
            printf("P%d\t%d\t%d\t%d\t%.2f\n",
                   i,
                   process[i].weight,
                   process[i].remain_time,
                   process[i].time_slice,
                   process[i].vruntime);
        }
    }
    else
    {
        printf("Process\tWeight\tRemain\tSlice\n");
        for (i = 0; i < num_process; i++)
        {
            printf("P%d\t%d\t%d\t%d\n",
                   i,
                   process[i].weight,
                   process[i].remain_time,
                   process[i].time_slice);
        }
    }
}
