#ifndef MAIN_TASK_H
#define MAIN_TASK_H

extern TX_EVENT_FLAGS_GROUP g_main_event_flags;

void     Main_thread_create(void* first_unused_memory);
uint32_t Main_thread_set_event(ULONG flags);
void     Init_synchronization_objects(void);

#endif  // MAIN_TASK_H
