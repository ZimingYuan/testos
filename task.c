#include "kernel.h"

extern usize _num_app;

enum TaskStatus {
    UnInit, Ready, Running, Exited
};
struct TaskControlBlock {
    usize task_cx_ptr;
    enum TaskStatus task_status;
} tasks[MAX_APP_NUM];
typedef struct TaskControlBlock TaskControlBlock;
usize current_task;
void __switch(usize *, usize *);

void task_init() {
    for (int i = 0; i < MAX_APP_NUM; i++) {
        tasks[i].task_cx_ptr = 0; tasks[i].task_status = UnInit;
    }
    for (int i = 0; i < _num_app; i++) {
        tasks[i].task_cx_ptr = init_app_cx(i);
        tasks[i].task_status = Ready;
    }
}
void run_next_task() {
    int next_task = -1;
    for (int i = current_task + 1; i < current_task + _num_app + 1; i++)
        if (tasks[i % _num_app].task_status == Ready) {
            next_task = i % _num_app; break;
        }
    if (next_task == -1) exit_all();
    tasks[next_task].task_status = Running;
    usize *current_task_cx_ptr2 = &tasks[current_task].task_cx_ptr;
    usize *next_task_cx_ptr2 = &tasks[next_task].task_cx_ptr;
    current_task = next_task;
    __switch(current_task_cx_ptr2, next_task_cx_ptr2);
}
void suspend_current_and_run_next() {
    tasks[current_task].task_status = Ready;
    run_next_task();
}
void exit_current_and_run_next() {
    tasks[current_task].task_status = Exited;
    run_next_task();
}
void run_first_task() {
    tasks[0].task_status = Running;
    usize t;
    __switch(&t, &tasks[0].task_cx_ptr);
}
