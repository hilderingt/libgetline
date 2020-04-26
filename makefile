CC=gcc
CFLAGS=-Wall
INCLUDE=-I./include
TARGET=libgetline.so

all: ${TARGET}

${TARGET}: src/libgetline.c include/libgetline.h
	${CC} -o ${TARGET} ${CFLAGS} -shared -fpic ${INCLUDE} src/libgetline.c
