# -*- coding: utf-8 -*-

import util.build_util as build_util
import util.build_config as build_config

build_util.info_print()

SConscript("neoagent/SConscript")

if 'configure' in COMMAND_LINE_TARGETS:
    if build_util.configure(Configure(env), build_config.libs) is False:
        Exit(1)
    else:
        Exit(0)
elif 'debian' in COMMAND_LINE_TARGETS:
    SConscript("debian/SConscript")
elif 'doc' in COMMAND_LINE_TARGETS:
    SConscript("doc/SConscript")
