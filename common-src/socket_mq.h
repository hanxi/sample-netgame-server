#ifndef socket_mq_h
#define socket_mq_h

struct socket_message {
    int sz;
    char * data;
};

struct message_queue;

// 0 for success
int socket_mq_pop(struct message_queue *q, struct socket_message *message);
void socket_mq_push(struct message_queue *q, struct socket_message *message);

// return the length of message queue, for debug
int socket_mq_length(struct message_queue *q);

struct message_queue * socket_mq_new();
void socket_mq_delete(struct message_queue *q);

#endif
