// NDBX OSC Implementation
// Can be used to receive messages from OSCulator.

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "nosc.h"

#define MILLIS_TO_NANOS 1000000

static void die(const char * format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, ".\n");
    exit(1);
}

static void warn(const char * format, ...) {
    va_list vargs;
    va_start(vargs, format);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, ".\n");
}

#if defined( __BIG_ENDIAN__ )
    static inline void swap32(void *v) { }
#elif defined( __LITTLE_ENDIAN__ )
    static inline void swap(char *a, char *b){
        char t = *a;
        *a = *b;
        *b = t;
    }

    static inline void swap32(void *v) {
        char *b = (char *) v;
        swap(b  , b+3);
        swap(b+1, b+2);
    }
#else
    #error Either __BIG_ENDIAN__ or __LITTLE_ENDIAN__ must be defined.
#endif

static void check_arg(int cond, const char *format, ...) {
    if (!cond) {
        va_list vargs;
        va_start(vargs, format);
        fprintf(stderr, "ERROR checking arg: ");
        vfprintf(stderr, format, vargs);
        fprintf(stderr, ".\n");
        exit(1);
    }
}

const char *nosc_message_get_string(const nosc_message *msg, int index) {
    char arg_type = msg->types[index];
    check_arg(arg_type == 's', "OSC argument %d is not a string.", index);
    return msg->args[index].s;
}

int32_t nosc_message_get_int(const nosc_message *msg, int index) {
    char arg_type = msg->types[index];
    check_arg(arg_type == 'i', "OSC argument %d is not an int32.", index);
    return msg->args[index].i;
}

float nosc_message_get_float(const nosc_message *msg, int index) {
    char arg_type = msg->types[index];
    check_arg(arg_type == 'f', "OSC argument %d is not a float.", index);
    return msg->args[index].f;
}

typedef struct {
    char *pos;
    int remaining;
} parser;

char *parse_string(parser *p) {
    assert(p != NULL);
    assert(p->pos != NULL);
    assert(p->remaining >= 4);

    // The string starts where the parser is.
    char *start = p->pos;

    // Now let's find the end
    char *end = p->pos;
    end += 3;
    while (*end) {
        end += 4;
    }
    end++;

    p->pos = end;
    p->remaining -= end - start;

    return start;
}

int32_t parse_int32(parser *p) {
    assert(p != NULL);
    assert(p->pos != NULL);
    assert(p->remaining >= 4);

    swap32(p->pos);
    uint32_t v = *(int32_t *)p->pos;

    p->pos += 4;
    p->remaining -= 4;

    return v;
}

float parse_float(parser *p) {
    assert(p != NULL);
    assert(p->pos != NULL);
    assert(p->remaining >= 4);

    swap32(p->pos);
    float v = *(float *)p->pos;

    p->pos += 4;
    p->remaining -= 4;

    return v;
}

static nosc_message *_nosc_server_parse_message(char *data, size_t size) {
    nosc_message *msg = calloc(1, sizeof(nosc_message));

    parser p;
    p.pos = data;
    p.remaining = size;

    // Parse the path
    const char *path = parse_string(&p);
    strncpy(msg->path, path, NOSC_MAX_PATH_LENGTH);

    // Parse the types
    const char *types = parse_string(&p);
    check_arg(*types == ',', "OSC message does not contain type tag string.");
    types++;
    strncpy(msg->types, types, NOSC_MAX_TYPES_LENGTH);
    int types_count = strlen(types);
    msg->arg_count = types_count;

    // Allocate the data structures.
    msg->args = calloc(types_count, sizeof(nosc_arg));

    // Parse the actual arguments.
    //char *args_ptr = types_ptr + len;
    for (int i = 0; i < types_count; i ++) {
        char arg_type = types[i];
        if (arg_type == 's') {
            const char *str = parse_string(&p);
            //len = get_string(args_ptr, remaining);
            //remaining -= len;
            int len = strlen(str);
            msg->args[i].s = calloc(len, 1);
            strncpy(msg->args[i].s, str, len);
        } else if (arg_type == 'i') {
            int v = parse_int32(&p);
            msg->args[i].i = v;
        } else if (arg_type == 'f') {
            float v = parse_float(&p);
            msg->args[i].f = v;
        } else {
            fprintf(stderr, "_nosc_server_parse_message: Unknown argument type %c\n", arg_type);
            exit(1);
        }
    }

    return msg;
}

static void _nosc_server_push_message(nosc_server *server, nosc_message *message) {
    pthread_mutex_lock(&server->message_mutex);
    nosc_message_item *item = calloc(1, sizeof(nosc_message_item));
    item->message = message;
    if (server->rear == NULL) {
        server->rear = item;
        server->front = item;
    } else {
        server->rear->next = item;
        server->rear = item;
    }
    pthread_mutex_unlock(&server->message_mutex);
}

static int _nosc_server_has_messages(nosc_server *server) {
    return server->rear ? 1 : 0;
}

static nosc_message *_nosc_server_pop_message(nosc_server *server) {
    nosc_message *msg = NULL;
    pthread_mutex_lock(&server->message_mutex);
    if (server->front != NULL) {
        nosc_message_item *item = server->front;
        if (server->front == server->rear) {
            // Queue is empty;
            server->front = NULL;
            server->rear = NULL;
        } else {
            server->front = server->front->next;
        }
        msg = item->message;
        free(item);
    }
    pthread_mutex_unlock(&server->message_mutex);
    return msg;
}

static void _nosc_server_start(nosc_server *server) {
    const char *hostname = 0;
    const char *port = "2222";
    struct addrinfo hints;
    memset(&hints, 0 ,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    struct addrinfo *res = NULL;
    int err = getaddrinfo(hostname, port, &hints, &res);
    if (err != 0) {
            die("failed to resolve local socket address (err=%d)",err);
    }

    // Create the socket
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) {
            die("%s", strerror(errno));
    }

    // Bind the local address to the socket
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
            die("%s", strerror(errno));
    }

    // Free addrinfo
    freeaddrinfo(res);

    char buffer[548];
    struct sockaddr_storage src_addr;

    struct iovec iov[1];
    iov[0].iov_base = buffer;
    iov[0].iov_len = sizeof(buffer);

    struct msghdr message;
    message.msg_name = &src_addr;
    message.msg_namelen = sizeof(src_addr);
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = 0;
    message.msg_controllen = 0;

    while(server->running) {
        ssize_t count = recvmsg(fd, &message, MSG_DONTWAIT);
        if (count == -1) {
            if (errno == EAGAIN) {
                nanosleep((struct timespec[]){{0, 1 * MILLIS_TO_NANOS}}, NULL);
            } else {
                die("%s", strerror(errno));
            }
        } else if (message.msg_flags & MSG_TRUNC) {
            warn("datagram too large for buffer: truncated");
        } else {
            nosc_message *msg = _nosc_server_parse_message(buffer, count);
            _nosc_server_push_message(server, msg);
        }
    }

    close(fd);
}

nosc_server *nosc_server_new(int port, nosc_server_handle_message_fn fn, void *ctx) {
    nosc_server *server = calloc(1, sizeof(nosc_server));
    server->port = port;
    server->running = 1;
    server->handle_message_fn = fn;
    server->handle_message_ctx = ctx;

    pthread_create(&server->server_thread, NULL, (void *(*)(void *))&_nosc_server_start, server);

    return server;
}

void nosc_server_update(nosc_server *server) {
    while (_nosc_server_has_messages(server)) {
        nosc_message *msg = _nosc_server_pop_message(server);
        // The check is here to avoid a race condition between has_messages and pop_message
        if (msg) {
            server->handle_message_fn(server, msg, server->handle_message_ctx);
            free(msg);
        }
    }
}

void nosc_server_free(nosc_server *server) {
    server->running = 0;
    pthread_join(server->server_thread, NULL);
    free(server);
}
