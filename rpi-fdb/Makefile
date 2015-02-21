#CFLAGS+=-std=c99
CFLAGS+=-Wall
#CFLAGS+=-Werror
CFLAGS+=-g
CFLAGS+=-O3

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
LIBS+=-lrtlsdr


iqvis: iqvis.c
	gcc $(CFLAGS) $(INCLUDES) $(LIBS) -o iqvis iqvis.c
iqvis_lines: iqvis_lines.c
	gcc $(CFLAGS) $(INCLUDES) $(LIBS) -o iqvis_lines iqvis_lines.c

