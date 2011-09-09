# -*- coding: utf-8 -*-

cflags = [
    '-std=c99',
    '-Wall',
    '-g',
    '-O2',
    '-fno-strict-aliasing',
    '-D_GNU_SOURCE',
    ]

libs = [
    'pthread',
    'ev',
    'json',
    ]

includes = [
    'ext',
    ]

headers = [
    'stdint.h',
    'stdbool.h',
    'unistd.h',
    'sys/stat.h',
    'sys/types.h',
    'sys/socket.h',
    'sys/un.h',
    'sys/ioctl.h',
    'arpa/inet.h',
    'netinet/in.h',
    'netdb.h',
    'signal.h',
    'errno.h',
    'pthread.h',
    ]

funcs = [
    'sigaction',
    'sigignore',
    ]
