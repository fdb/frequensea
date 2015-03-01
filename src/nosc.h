// NDBX OSC Implementation
// Can be used to receive messages from OSCulator.

#ifndef NOSC_H
#define NOSC_H

#include <pthread.h>

#define NOSC_MAX_PATH_LENGTH 200
#define NOSC_MAX_TYPES_LENGTH 10

typedef union nosc_arg {
    char *s;
    int32_t i;
    float f;
} nosc_arg;

typedef struct {
    char path[NOSC_MAX_PATH_LENGTH];
    char types[NOSC_MAX_TYPES_LENGTH];
    int arg_count;
    nosc_arg *args;
} nosc_message;

const char *nosc_message_get_string(const nosc_message *msg, int index);
int32_t nosc_message_get_int(const nosc_message *msg, int index);
float nosc_message_get_float(const nosc_message *msg, int index);


typedef struct nosc_server nosc_server;

typedef void (*nosc_server_handle_message_fn)(nosc_server *server, nosc_message *message, void *ctx);

struct nosc_server {
    int port;

    nosc_server_handle_message_fn handle_message_fn;
    void *handle_message_ctx;

    pthread_t server_thread;

    nosc_message *current_message;
    pthread_mutex_t message_mutex;
};

nosc_server *nosc_server_new(int port, nosc_server_handle_message_fn fn, void *ctx);
void nosc_server_update(nosc_server *server);
void nosc_server_free(nosc_server *server);

#endif // NOSC_H
