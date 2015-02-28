// A simple OSC listening server.
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
    int32_t i;
    float f;
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

int32_t osc_message_get_int32_arg(const osc_message *msg, int index) {
    char arg_type = msg->types[index];
    check_arg(arg_type == 'i', "OSC argument %d is not an int32.", index);
    return msg->args[index].i;
}

typedef struct {
    char *pos;
    int remaining;
} parser;

char *parse_string(parser *p) {
    assert(p != NULL);
    assert(p->pos != NULL);
    assert(p->remaining > 0);

    // The string starts where the parser is.
    char *start = p->pos;

    // Now let's find the end
    char *end = p->pos;
    end += 3;
    while (*end) {
        end += 4;
    }
    end++;

    printf("parse_string %s len %ld\n", start, end - start);
    p->pos = end;
    p->remaining = end - start;

    return start;
}

int32_t parse_int32(parser *p) {
    assert(p != NULL);
    assert(p->pos != NULL);
    assert(p->remaining >= 4);

    char *pos = p->pos;
    swap32(pos);
    uint32_t v = *(int32_t *)pos;

    printf("parse_int32 %d\n", v);
    p->pos += 4;
    p->remaining -= 4;

    return v;
}


void handle_datagram(char *data, size_t size) {
    printf("Size: %ld\n", size);
    printf("Buffer: %s\n", data);

    for (int i = 0; i < size; i++) {
        printf("%4d: %d\n", i, data[i]);
    }

    osc_message *msg = calloc(1, sizeof(osc_message));

    parser p;
    p.pos = data;
    p.remaining = size;



    //int pos = 0;
    //int len = 0;
    //int remaining = size;

    //char *p = data;

    // Parse the path
    const char *path = parse_string(&p);
    strncpy(msg->path, path, MAX_PATH_LENGTH);

    // Parse the types
    printf("Types: %s\n", p.pos);
    const char *types = parse_string(&p);
    check_arg(*types == ',', "OSC message does not contain type tag string.");
    types++;
    strncpy(msg->types, types, MAX_TYPES_LENGTH);
    int types_count = strlen(types);
    printf("Types in msg: %s\n", msg->types);

    // char *types_ptr = data + len;
    // types_ptr++;
    // len = get_string(types_ptr, remaining);
    // remaining -= len;
    // strncpy(msg->types, types_ptr, MAX_TYPES_LENGTH);

    // Allocate the data structures.
    msg->args = calloc(types_count, sizeof(osc_arg));

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
            printf("Len %d\n", len);
            strncpy(msg->args[i].s, str, len);
        } else if (arg_type == 'i') {
            int v = parse_int32(&p);
            msg->args[i].i = v;
        }

        printf("Arg %c\n", arg_type);
    }

    printf("Message PATH %s TYPES %s\n", msg->path, msg->types);

    const char *arg0 = osc_message_get_string_arg(msg, 0);
    printf("Arg 0 %s\n", arg0);

    int arg1 = osc_message_get_int32_arg(msg, 1);
    printf("Arg 1 %d\n", arg1);

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
