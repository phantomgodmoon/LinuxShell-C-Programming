// COMP3511 Spring 2023
// PA3: Page Replacement Algorithms
//
// Your name: Leung Pak Hei
// Your ITSC email:phleungaf@connect.ust.hk
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

#define UNFILLED_FRAME -1
#define MAX_QUEUE_SIZE 10
#define MAX_FRAMES_AVAILABLE 10
#define MAX_REFERENCE_STRING 30

#define ALGORITHM_FIFO "FIFO"
#define ALGORITHM_OPT "OPT"
#define ALGORITHM_LRU "LRU"
#define ALGORITHM_CLOCK "CLOCK"

// Keywords (to be used when parsing the input)
#define KEYWORD_ALGORITHM "algorithm"
#define KEYWORD_FRAMES_AVAILABLE "frames_available"
#define KEYWORD_REFERENCE_STRING_LENGTH "reference_string_length"
#define KEYWORD_REFERENCE_STRING "reference_string"

#define SPACE_CHARS " \t"

char algorithm[10];
int reference_string[MAX_REFERENCE_STRING];
int reference_string_length;
int frames_available;
int frames[MAX_FRAMES_AVAILABLE];

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

void parse_tokens(char **argv, char *line, int *numTokens, char *delimiter)
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
    size_t nread;
    size_t len = 0;

    char *two_tokens[2];
    char *reference_string_tokens[MAX_REFERENCE_STRING];
    int numTokens = 0, n = 0, i = 0;
    char equal_plus_spaces_delimiters[5] = "";

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters, SPACE_CHARS);

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        if (is_skip(line) == 0)
        {
            line = strtok(line, "\n");
            if (strstr(line, KEYWORD_ALGORITHM))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    strcpy(algorithm, two_tokens[1]);
                }
            }
            else if (strstr(line, KEYWORD_FRAMES_AVAILABLE))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &frames_available);
                }
            }
            else if (strstr(line, KEYWORD_REFERENCE_STRING_LENGTH))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &reference_string_length);
                }
            }
            else if (strstr(line, KEYWORD_REFERENCE_STRING))
            {
                parse_tokens(two_tokens, line, &numTokens, "=");
                if (numTokens == 2)
                {
                    parse_tokens(reference_string_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(reference_string_tokens[i], "%d", &reference_string[i]);
                    }
                }
            }
        }
    }
}

void print_parsed_values()
{
    int i;
    printf("%s = %s\n", KEYWORD_ALGORITHM, algorithm);
    printf("%s = %d\n", KEYWORD_FRAMES_AVAILABLE, frames_available);
    printf("%s = %d\n", KEYWORD_REFERENCE_STRING_LENGTH, reference_string_length);
    printf("%s = ", KEYWORD_REFERENCE_STRING);
    for (i = 0; i < reference_string_length; i++)
        printf("%d ", reference_string[i]);
    printf("\n");
}

const char template_total_page_fault[] = "Total Page Fault: %d\n";
const char template_no_page_fault[] = "%d: No Page Fault\n";

void display_fault_frame(int current_frame)
{
    int j;
    printf("%d: ", current_frame);
    for (j = 0; j < frames_available; j++)
    {
        if (frames[j] != UNFILLED_FRAME)
            printf("%d ", frames[j]);
        else
            printf("  ");
    }
    printf("\n");
}

void frames_init()
{
    int i;
    for (i = 0; i < frames_available; i++)
        frames[i] = UNFILLED_FRAME;
}

// Define a Queue with value front, rear, and count.
struct Queue
{
    int values[MAX_QUEUE_SIZE];
    int front, rear, count;
};

// Check if the queue is peek
int queue_peek(struct Queue *q)
{
    return q->values[q->front];
}

// Initialize the value of count frint rear to 0, 0, and 1
void queue_init(struct Queue *q)
{
    q->rear = -1;
    q->front = 0;
    q->count = 0;
}

// Check if the queue is full
int queue_is_full(struct Queue *q)
{
    return q->count == MAX_QUEUE_SIZE;
}

// Check if the queue is empty
int queue_is_empty(struct Queue *q)
{
    return q->count == 0;
}

// Dequeue function
void queue_dequeue(struct Queue *q)
{
    q->front++;
    if (q->front == MAX_QUEUE_SIZE)
        q->front = 0;
    q->count--;
}

// Enqueue funciton
void queue_enqueue(struct Queue *q, int new_value)
{
    if (queue_is_full(q) != 1)
    {
        if (q->rear == MAX_QUEUE_SIZE - 1)
            q->rear = -1;
        q->count++;
        q->values[++q->rear] = new_value;
    }
}

void FIFO_replacement()
{
    int size = 0;
    int num_fault = 0;
    struct Queue q;
    queue_init(&q);

    for (int i = 0; i < reference_string_length; i++)
    {
        // Denote exists = 0 as condution
        int exists = 0;
        for (int j = 0; j < frames_available; j++)
        {
            if (frames[j] == reference_string[i])
            {
                exists = 1; // non-zero to denote it as exists
                break;
            }
        }

        if (!exists)
        {
            // record the old_page
            int old_page = queue_peek(&q);

            if (size == frames_available)
            {
                for (int j = 0; j < frames_available; j++)
                {
                    if (frames[j] == old_page)
                    {
                        frames[j] = reference_string[i];
                        break;
                    }
                }
                // dequeue
                queue_dequeue(&q);
                size--;
            }
            else
            {
                int temp = 0;
                while (frames[temp] != UNFILLED_FRAME)
                    temp++;

                frames[temp] = reference_string[i];
            }

            // enqueue
            queue_enqueue(&q, reference_string[i]);
            num_fault++;
            size++;
            display_fault_frame(reference_string[i]);
        }
        else
        {
            printf("%d: No Page Fault\n", reference_string[i]);
        }
    }

    printf("Total Page Fault: %d\n", num_fault);
}

void LRU_replacement()
{
    int num_fault = 0;
    int last_used[frames_available];

    for (int i = 0; i < frames_available; i++)
    {
        last_used[i] = -1;
    }

    for (int i = 0; i < reference_string_length; i++)
    {
        int exists = 0;

        for (int j = 0; j < frames_available; j++)
        {
            if (frames[j] == reference_string[i])
            {
                exists = 1;
                last_used[j] = i;
                break;
            }
        }

        if (!exists)
        {
            int minimum_index = 0;
            int minimum_used = i;

            for (int j = 0; j < frames_available; j++)
            {
                if (last_used[j] < minimum_used)
                {
                    minimum_index = j;
                    minimum_used = last_used[j];
                }
            }

            num_fault++;                  // number of fault found +1
            last_used[minimum_index] = i; // Set the last_used array of min_index to i
            frames[minimum_index] = reference_string[i];
            display_fault_frame(reference_string[i]);
        }
        else
        {
            printf("%d: No Page Fault\n", reference_string[i]);
        }
    }

    printf("Total Page Fault: %d\n", num_fault);
}

void OPT_replacement()
{
    int num_fault = 0;

    for (int i = 0; i < reference_string_length; i++)
    {
        int exists = 0;

        for (int j = 0; j < frames_available; j++)
        {
            if (frames[j] == reference_string[i])
            {
                exists = 1;
                break;
            }
        }

        if (!exists)
        {
            // Definte maximum_index, minimum_reference and maximum_length
            int maximum_index = 0;
            int minimum_reference = 0;
            int maximum_length = 0;

            for (int j = 0; j < frames_available; j++)
            {
                int reference = frames[j];
                int length = 1;

                for (int z = i + 1; z < reference_string_length; z++)
                {
                    if (reference_string[z] == reference)
                        break;
                    length++;
                }

                if ((length > maximum_length) || (reference < minimum_reference && length == maximum_length))
                {
                    minimum_reference = reference;
                    maximum_index = j;
                    maximum_length = length;
                }
            }

            num_fault++;
            frames[maximum_index] = reference_string[i];
            display_fault_frame(reference_string[i]);
        }
        else
        {
            printf("%d: No Page Fault\n", reference_string[i]);
        }
    }

    printf("Total Page Fault: %d\n", num_fault);
}

void CLOCK_replacement()
{
    // Create an array name second_chance
    int second_chance[frames_available];
    // initialize all to zero according to the instruction
    for (int i = 0; i < frames_available; ++i)
    {
        second_chance[i] = 0;
    }

    // Create three variables:
    // referenced is set to 1 if the page referenced by the current process is already present in the frames.
    // empty_avaliable is set to 1 if there is an empty frame available for the current process to be loaded into.
    // is used to determine whether a page replacement is required when all the frames are occupied.
    // If it is 0, then a page replacement is required. If it is 1, then the current process can be loaded into the current frame without any replacement.
    int found = -1;
    int num_fault = 0;
    int pointer = 0;
    int referenced = -1;
    int empty_avaliable = -1;
    int placement_required = -1;

    for (int i = 0; i < reference_string_length; ++i)
    {
        found = 0;
        referenced = 0;
        empty_avaliable = 0;
        for (int j = 0; j < frames_available; ++j)
        {
            if (frames[j] == reference_string[i])
            {
                found = 1;
                referenced = 1;
                empty_avaliable = 1;
                second_chance[j] = 1;
                break;
            }
        }
        if (referenced < 1)
        {
            for (int j = 0; j < frames_available; ++j)
            {
                if (frames[j] == -1)
                {
                    empty_avaliable = 1;
                    frames[j] = reference_string[i];
                    num_fault = num_fault + 1;
                    break;
                }
            }
        }
        if (empty_avaliable < 1)
        {
            placement_required = 0;
            for (int j = 0; j < frames_available; ++j)
            {
                if (second_chance[pointer] == 1)
                {
                    second_chance[pointer] = 0;
                    pointer = pointer + 1;
                    pointer = pointer % frames_available;
                }
                else if (second_chance[pointer] == 0)
                {
                    placement_required = 1;
                    frames[pointer] = reference_string[i];
                    pointer += 1;
                    pointer = pointer % frames_available;
                    num_fault = num_fault + 1;
                    break;
                }
            }
            if (placement_required == 0)
            {
                frames[pointer] = reference_string[i];
                pointer += 1;
                pointer = pointer % frames_available;
                num_fault = num_fault + 1;
            }
        }
        if (found == 1)
        {
            printf(template_no_page_fault, reference_string[i]);
        }
        else
        {
            display_fault_frame(reference_string[i]);
        }
    }
    printf(template_total_page_fault, num_fault);
}

int main()
{
    parse_input();
    print_parsed_values();
    frames_init();

    if (strcmp(algorithm, ALGORITHM_FIFO) == 0)
    {
        FIFO_replacement();
    }
    else if (strcmp(algorithm, ALGORITHM_OPT) == 0)
    {
        OPT_replacement();
    }
    else if (strcmp(algorithm, ALGORITHM_LRU) == 0)
    {
        LRU_replacement();
    }
    else if (strcmp(algorithm, ALGORITHM_CLOCK) == 0)
    {
        CLOCK_replacement();
    }

    return 0;
}