#ifndef GATE_MESSAGE_QUEUE_H
#define GATE_MESSAGE_QUEUE_H

#include <stdlib.h>
#include <stdint.h>

struct gate_message {
	int type;
	int id;
	int ud;
	char * buffer;
};

// 0 for success
int gate_mq_pop(struct gate_message *message);
void gate_mq_push(struct gate_message *message);

// return the length of message queue, for debug
int gate_mq_length();

void gate_mq_init();
void gate_mq_release();

#endif
