# CFLAGS+=-std=c99
CFLAGS+=-Wall
CFLAGS+=-Werror
CFLAGS+=-g

INCLUDES+=-I/opt/vc/include
INCLUDES+=-I/opt/vc/include/interface/vcos/pthreads
INCLUDES+=-I/opt/vc/include/interface/vmcs_host/linux

LIBS+=-L/opt/vc/lib
LIBS+=-lGLESv2
LIBS+=-lEGL
LIBS+=-lm
LIBS+=-lrt
LIBS+=-lbcm_host
LIBS+=-lvcos
LIBS+=-lvchiq_arm
LIBS+=-lpthread


iqvis: iqvis.c
	gcc $(CFLAGS) $(INCLUDES) $(LIBS) -o iqvis iqvis.c
