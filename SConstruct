# -*- coding: utf-8 -*-

import build.util

build.util.info_print()

progs = [
    'neoagent',
]

[ SConscript( prog + "/SConscript")  for prog in progs ]

if 'debian' in COMMAND_LINE_TARGETS:
    SConscript("debian/SConscript")
elif 'doc' in COMMAND_LINE_TARGETS:
    SConscript("doc/SConscript")
