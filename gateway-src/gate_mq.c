#include "gate_mq.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define DEFAULT_QUEUE_SIZE 64

struct message_queue {
	int cap;
	int head;
	int tail;
	int lock;
	struct gate_message *queue;
	struct message_queue *next;
};

static struct message_queue *Q = NULL;

#define LOCK(q) while (__sync_lock_test_and_set(&(q)->lock,1)) {}
#define UNLOCK(q) __sync_lock_release(&(q)->lock);

int
gate_mq_length() {
    struct message_queue *q = Q;
	int head, tail,cap;

	LOCK(q)
	head = q->head;
	tail = q->tail;
	cap = q->cap;
	UNLOCK(q)
	
	if (head <= tail) {
		return tail - head;
	}
	return tail + cap - head;
}

int
gate_mq_pop(struct gate_message *message) {
    struct message_queue *q = Q;
	int ret = 1;
	LOCK(q)

	if (q->head != q->tail) {
		*message = q->queue[q->head];
		ret = 0;
		if ( ++ q->head >= q->cap) {
			q->head = 0;
		}
	}
	
	UNLOCK(q)

	return ret;
}

static void
expand_queue(struct message_queue *q) {
	struct gate_message *new_queue = MALLOC(sizeof(struct gate_message) * q->cap * 2);
	int i;
	for (i=0;i<q->cap;i++) {
		new_queue[i] = q->queue[(q->head + i) % q->cap];
	}
	q->head = 0;
	q->tail = q->cap;
	q->cap *= 2;
	
	FREE(q->queue);
	q->queue = new_queue;
}

void 
gate_mq_push(struct gate_message *message) {
    struct message_queue *q = Q;
	assert(message);
	LOCK(q)

	q->queue[q->tail] = *message;
	if (++ q->tail >= q->cap) {
		q->tail = 0;
	}

	if (q->head == q->tail) {
		expand_queue(q);
	}
	
	UNLOCK(q)
}

void 
gate_mq_init() {
	struct message_queue *q = MALLOC(sizeof(*q));
	q->cap = DEFAULT_QUEUE_SIZE;
	q->head = 0;
	q->tail = 0;
	q->lock = 0;
	// When the queue is create (always between service create and service init) ,
	q->queue = MALLOC(sizeof(struct gate_message) * q->cap);
	q->next = NULL;
	Q=q;
}

void
gate_mq_release() {
	struct message_queue *q = Q;
	struct gate_message msg;
	while(!gate_mq_pop(&msg)) {
        FREE(msg.buffer);
	}
	assert(q->next == NULL);
	//FREE(q->queue);
	FREE(q);
}


