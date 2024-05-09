#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
    if (q == NULL)
        return 1;
    
    return (q->size <= 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
    /* TODO: put a new process to queue [q] */

    /* Check for invalid queue or process */
    if (q == NULL || proc == NULL)
        return;

    /* Check if the queue is already full */
    if (q->size >= MAX_QUEUE_SIZE)
        return;

    /* Add process to tail of the queue */
    q->proc[q->size++] = proc;
}

struct pcb_t *dequeue(struct queue_t *q)
{
    /* TODO: return a pcb whose prioprity is the highest
     * in the queue [q] and remember to remove it from q
     * */

    /* Check if the queue is empty */
    if (empty(q))
        return NULL;

    /* Get the head process of the queue */
    struct pcb_t *ret_proc = q->proc[0];
    q->proc[0] = NULL;
    --q->size;

    /* Shift elements towards head of the queue */
    for (int idx = 0; idx < q->size; ++idx)
        q->proc[idx] = q->proc[idx + 1];

    return ret_proc;
}