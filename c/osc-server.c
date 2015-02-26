// A simple OSC listening server.
// Can be used to receive messages from OSCulator.

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

void check_arg(int cond, const char *format, ...) {
    if (!cond) {
        va_list vargs;
        va_start(vargs, format);
        fprintf(stderr, "ERROR checking arg: ");
        vfprintf(stderr, format, vargs);
        fprintf(stderr, ".\n");
        exit(1);
    }
}

ssize_t get_string(void *data, ssize_t size) {
    ssize_t i = 0;
    ssize_t len = 0;
    char *c = data;
    if (size < 0) {
        return -1;
    }
    for (i = 0; i < size; ++i) {
        if (c[i] == '\0') {
            len = 4 * (i / 4 + 1);
            break;
        }
    }
    if (0 == len) {
        return -1;
    }
    if (len > size) {
        return -1;
    }
    for (; i < len; ++i) {
        if (c[i] != '\0') {
            return -1;
        }
    }
    return len;
}

#define MAX_PATH_LENGTH 200
#define MAX_TYPES_LENGTH 10

typedef union osc_arg {
    char *s;
    int *i;
    float *f;
} osc_arg;

typedef struct {
    char path[MAX_PATH_LENGTH];
    char types[MAX_TYPES_LENGTH];
    osc_arg *args;
} osc_message;

const char *osc_message_get_string_arg(const osc_message *msg, int index) {
    char arg_type = msg->types[index];
    check_arg(arg_type == 's', "OSC argument %d is not a string.", index);
    return msg->args[index].s;
}


void handle_datagram(char *data, size_t size) {
    printf("Size: %ld\n", size);
    printf("Buffer: %s\n", data);

    for (int i = 0; i < size; i++) {
        printf("%4d: %d\n", i, data[i]);
    }

    osc_message *msg = calloc(1, sizeof(osc_message));

    //int pos = 0;
    int len = 0;
    int remaining = size;


    // Parse the path
    len = get_string(data, remaining);
    remaining -= len;
    strncpy(msg->path, data, MAX_PATH_LENGTH);

    // Parse the types
    char *types_ptr = data + len;
    check_arg(*types_ptr == ',', "OSC message does not contain type tag string.");
    types_ptr++;
    len = get_string(types_ptr, remaining);
    int types_count = strlen(types_ptr);
    remaining -= len;
    strncpy(msg->types, types_ptr, MAX_TYPES_LENGTH);

    // Allocate the data structures.
    msg->args = calloc(types_count, sizeof(osc_arg));

    // Parse the actual arguments.
    char *args_ptr = types_ptr + len;
    for (int i = 0; i < types_count; i ++) {
        char arg_type = types_ptr[i];
        if (arg_type == 's') {
            len = get_string(args_ptr, remaining);
            remaining -= len;
            msg->args[i].s = calloc(len, sizeof(char));
            printf("Len %d\n", len);
            strncpy(msg->args[i].s, args_ptr, len);
            args_ptr += len;
        }

        printf("Arg %c\n", arg_type);
    }

    printf("Message PATH %s TYPES %s\n", msg->path, msg->types);

    const char *arg0 = osc_message_get_string_arg(msg, 0);
    printf("Arg 0 %s\n", arg0);

    free(msg);


}



int main() {
    const char *hostname = 0;
    const char *portname = "2222";
    struct addrinfo hints;
    memset(&hints, 0 ,sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_PASSIVE|AI_ADDRCONFIG;
    struct addrinfo *res = NULL;
    int err=getaddrinfo(hostname,portname,&hints,&res);
    if (err!=0) {
            die("failed to resolve local socket address (err=%d)",err);
    }
    // Create the socket
    int fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if (fd==-1) {
            die("%s",strerror(errno));
    }
    // Bind the local address to the socket
    if (bind(fd,res->ai_addr,res->ai_addrlen)==-1) {
            die("%s",strerror(errno));
    }
    // Free addrinfo
    freeaddrinfo(res);

    char buffer[548];
    struct sockaddr_storage src_addr;

    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof(buffer);

    struct msghdr message;
    message.msg_name=&src_addr;
    message.msg_namelen=sizeof(src_addr);
    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_control=0;
    message.msg_controllen=0;

    while (1) {
        ssize_t count=recvmsg(fd,&message,0);
        if (count==-1) {
                die("%s",strerror(errno));
        } else if (message.msg_flags&MSG_TRUNC) {
                warn("datagram too large for buffer: truncated");
        } else {
                handle_datagram(buffer,count);
        }

    }



}
