# -*- coding: utf-8 -*-

import build.util

build.util.info_print()

SConscript("neoagent/SConscript")

if 'debian' in COMMAND_LINE_TARGETS:
    SConscript("debian/SConscript")
elif 'doc' in COMMAND_LINE_TARGETS:
    SConscript("doc/SConscript")
