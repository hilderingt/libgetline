CC=gcc
CFLAGS=-g -Wall
INCLUDE=-I.
TARGET=libgetline.so

all: ${TARGET}

${TARGET}: libgetline.c libgetline.h
	${CC} -o ${TARGET} ${CFLAGS} -shared -fpic ${INCLUDE} libgetline.c
