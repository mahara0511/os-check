#include "queue.h"
#include "sched.h"
#include "timer.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>

static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
#endif

int queue_empty(void)
{
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if (!empty(&mlq_ready_queue[prio]))
			return 0;
	return 1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void)
{
#ifdef MLQ_SCHED
	int i;

	for (i = 0; i < MAX_PRIO; ++i)
	{
		mlq_ready_queue[i].size = 0;
		mlq_ready_queue[i].slot = 0;
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/*
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t *get_mlq_proc(int cpu_id)
{
	struct pcb_t *proc = NULL;
	/*TODO: get a process from PRIORITY [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */

	pthread_mutex_lock(&queue_lock);

	/* Prevent infinite loop around with empty bit */
	static int mlq_empty = 0;
	mlq_empty = 0;

	for (int prio = cur_prio[cpu_id]; prio < MAX_PRIO; ++prio)
	{
		cur_prio[cpu_id] = prio;
		if (!empty(&mlq_ready_queue[prio]))
		{ /* Found the non-empty ready queue */
			mlq_ready_queue[prio].slot += time_slot;
			proc = dequeue(&mlq_ready_queue[prio]);

			if (mlq_ready_queue[prio].slot >= MAX_PRIO - prio || empty(&mlq_ready_queue[prio]))
			{ /* This ready queue becomes empty or exceed slot limit then, */
				mlq_ready_queue[prio].slot = 0; 	  // Reset its slot
				++cur_prio[cpu_id];					  // Bring cur_prio to the next ready qeue
				if (cur_prio[cpu_id] >= MAX_PRIO - 1) // Loop cur_prio around
					cur_prio[cpu_id] = 0;
			}
			break;
		}

		/* In case we are at the end ready queue, perform the loop around */
		if (cur_prio[cpu_id] >= MAX_PRIO - 1)
		{
			cur_prio[cpu_id] = 0;
			if (mlq_empty)
				break;
			prio = -1;
			mlq_empty = 1;
		}
	}

	pthread_mutex_unlock(&queue_lock);
	return proc;
}

void put_mlq_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

struct pcb_t *get_proc(int id)
{
	return get_mlq_proc(id);
}

void put_proc(struct pcb_t *proc)
{
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t *proc)
{
	return add_mlq_proc(proc);
}
#else
struct pcb_t *get_proc(void)
{
	struct pcb_t *proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	pthread_mutex_lock(&queue_lock);

	// Implementation for fetching from ready_queue
	if (!empty(&ready_queue))
	{
		proc = dequeue(&ready_queue);
		enqueue(&ready_queue, proc); // Re-enqueue the process
	}

	pthread_mutex_unlock(&queue_lock);
	return proc;
}

void put_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}
#endif
