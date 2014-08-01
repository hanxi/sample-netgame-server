#ifndef socket_mq_h
#define socket_mq_h

struct socket_message {
    int sz;
    char * data;
};

// 0 for success
int socket_mq_pop(struct socket_message *message);
void socket_mq_push(struct socket_message *message);

// return the length of message queue, for debug
int socket_mq_length();

void socket_mq_init();
void socket_mq_release();

#endif
