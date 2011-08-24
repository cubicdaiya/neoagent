# -*- coding: utf-8 -*-

cflags = [
    '-std=c99',
    '-Wall',
    '-g0',
    '-O3',
    #'-fno-strict-aliasing',
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
