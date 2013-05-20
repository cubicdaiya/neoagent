# -*- coding: utf-8 -*-

import sys

cflags = [
    '-std=c99',
    '-Wall',
    '-g',
    '-O2',
#    '-fno-strict-aliasing',
    '-D_GNU_SOURCE',
    '-Wimplicit-function-declaration',
    '-Wunused-variable',
]

libs = [
    'pthread',
    'ev',
    'json',
]

if sys.platform != 'darwin':
    libs.append('rt')

includes = [
    #'ext',
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
    'ev.h',
]

funcs = [
    'sigaction',
    'sigignore',
]
