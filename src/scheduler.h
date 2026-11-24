
#ifndef SCHEDULER_H
#define SCHEDULER_H

/*
 * LPC2148 Mini Embedded Operating System
 * Module: Task Scheduler
 * Architecture: ARM7TDMI-S
 */

#define MAX_TASKS 5

/* Task States */
typedef enum
{
    READY,
    RUNNING,
    WAITING,
    SUSPENDED
} TaskState;

/* Task Function Pointer */
typedef void (*TaskFunction)(void);

/* Task Control Block */
typedef struct
{
    unsigned char task_id;
    TaskFunction task_function;

    unsigned long period_ms;
    unsigned long last_run;

    unsigned char priority;
    TaskState state;

} TCB;

/* Scheduler Function Prototypes */
void Scheduler_Init(void);

int Scheduler_AddTask(
    TaskFunction function,
    unsigned long period,
    unsigned char priority
);

void Scheduler_Run(void);

void Scheduler_Tick(void);

void Scheduler_SuspendTask(unsigned char task_id);

void Scheduler_ResumeTask(unsigned char task_id);

#endif
