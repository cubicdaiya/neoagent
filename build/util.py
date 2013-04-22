# -*- coding: utf-8 -*-

import sys
import SCons

def info_print():
    print "Python " + ".".join([ str(i) for i in sys.version_info ])
    print "SCons  " + SCons.__version__

def check_pkg(conf, lib):
    result = conf.TryAction('pkg-config --exists %s' % lib)[0]
    if result == 0:
        return False
    return True

def configure(conf, libs, headers, funcs):
    if not conf.CheckCC():
        print "c compiler is not installed!"
        return False

    for lib in libs:
        if not conf.CheckLib(lib):
            print "library " + lib + " not installed!"
            return False

    for header in headers:
        if not conf.CheckHeader(header):
            print header + " does not exist!"
            return False

    for func in funcs:
        if not conf.CheckFunc(func):
            print func + " does not exist!"
            return False
    
    conf.Finish()
    return True

def install(env, prog, prefix, doc_dir, misc_dir):
    if not prefix[-1] == '/':
        prefix += '/'
    bin_dir = prefix + 'bin/'
    man_dir = prefix + 'share/man/man1/'
    env.Alias(target="install", source=env.Install(bin_dir, prog))
    env.Install(bin_dir, prog)
    env.Alias(target="install", source=env.Install(bin_dir, misc_dir + 'neostat/neostat'))
    env.Install(bin_dir, misc_dir + 'neostat/neostat')
    env.Alias(target="install", source=env.Install(bin_dir, misc_dir + 'neoctl/neoctl'))
    env.Install(bin_dir, misc_dir + 'neoctl/neoctl')
    env.Alias(target="install", source=env.Install(man_dir, doc_dir + 'build/man/neoagent.1'))
    env.Install(man_dir, doc_dir + 'build/man/neoagent.1')
    env.Alias(target="install", source=env.Install(man_dir, doc_dir + 'build/man/neostat.1'))
    env.Install(man_dir, doc_dir + 'build/man/neostat.1')
    env.Alias(target="install", source=env.Install(man_dir, doc_dir + 'build/man/neoctl.1'))
    env.Install(man_dir, doc_dir + 'build/man/neoctl.1')
