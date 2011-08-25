# -*- coding: utf-8 -*-

import util.build_util as build_util

build_util.info_print()

SConscript("neoagent/SConscript")

if 'debian' in COMMAND_LINE_TARGETS:
    SConscript("debian/SConscript")
elif 'doc' in COMMAND_LINE_TARGETS:
    SConscript("doc/SConscript")
