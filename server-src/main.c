#include "socket_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static void *
_poll(void * ud) {
    struct socket_server *ss = ud;
    struct socket_message result;
    for (;;) {
        int type = socket_server_poll(ss, &result, NULL);
        switch (type) {
        case SOCKET_EXIT:
            return NULL;
        case SOCKET_DATA:
            printf("message(%u) [id=%d] size=%d\n",result.opaque,result.id, result.ud);
            free(result.data);
            break;
        case SOCKET_CLOSE:
            printf("close(%u) [id=%d]\n",result.opaque,result.id);
            break;
        case SOCKET_OPEN:
            printf("open(%u) [id=%d] %s\n",result.opaque,result.id,result.data);
            break;
        case SOCKET_ERROR:
            printf("error(%u) [id=%d]\n",result.opaque,result.id);
            break;
        case SOCKET_ACCEPT:
            printf("accept(%u) [id=%d %s] from [%d]\n",result.opaque, result.ud, result.data, result.id);
            socket_server_start(ss,300,result.ud);
            break;
        default:
            printf("unknow type:%d\n",type);
        }
    }
}

int
main() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);

    struct socket_server * ss = socket_server_create();
    int l = socket_server_listen(ss,200,"",8888,32);
    printf("listening %d\n",l);
    socket_server_start(ss,201,l);
    _poll(ss);
    socket_server_release(ss);

    return 0;
}
