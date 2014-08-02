#include "socket_mq.h"
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
	struct socket_message *queue;
	struct message_queue *next;
};

#define LOCK(q) while (__sync_lock_test_and_set(&(q)->lock,1)) {}
#define UNLOCK(q) __sync_lock_release(&(q)->lock);

int
socket_mq_length(struct message_queue *q) {
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
socket_mq_pop(struct message_queue *q, struct socket_message *message) {
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
	struct socket_message *new_queue = MALLOC(sizeof(struct socket_message) * q->cap * 2);
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
socket_mq_push(struct message_queue *q, struct socket_message *message) {
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

struct message_queue *
socket_mq_new() {
	struct message_queue *q = MALLOC(sizeof(*q));
	q->cap = DEFAULT_QUEUE_SIZE;
	q->head = 0;
	q->tail = 0;
	q->lock = 0;
	// When the queue is create (always between service create and service init) ,
	q->queue = MALLOC(sizeof(struct socket_message) * q->cap);
	q->next = NULL;
    return q;
}

void
socket_mq_delete(struct message_queue *q) {
	struct socket_message msg;
	while(!socket_mq_pop(q,&msg)) {
        FREE(msg.data);
	}
	assert(q->next == NULL);
	//FREE(q->queue);
	FREE(q);
}


