# -*- coding: utf-8 -*-

import os
import shutil

def write_file(fpath, content):
    f = open(fpath, 'w')
    f.write(content)
    f.close()

def copy_file(target=None, source=None, env=None):
    shutil.copy2(str(source[0]), str(target[0]))

def make_package(env, DEBNAME, DEBVERSION, SUBVERSION, DEBMAINT, DEBARCH, DEBDEPENDS, DEBDESC, DEBCONTROLFILE, DEBCOPYRIGHTFILE, DEBCHANGELOGFILE, DEBFILES):
    
    def make_control(target=None, source=None, env=None):
        CONTROL_TEMPLATE = """
Package: %s
Priority: extra
Section: utils
Installed-Size: %s
Maintainer: %s
Architecture: %s
Version: %s
Depends: %s
Description: %s

"""
        installed_size = 0
        for i in DEBFILES:
            installed_size += os.stat(str(env.File(i[1])))[6]
        control_info = CONTROL_TEMPLATE % (
            DEBNAME, installed_size, DEBMAINT, DEBARCH, DEBVERSION,
            DEBDEPENDS, DEBDESC)
        write_file(str(target[0]), control_info)
        write_file(os.path.dirname(os.path.dirname(os.path.dirname(str(target[0])))) + '/' + os.path.basename(str(target[0])), control_info)
    
    debpkg = '#%s_%s-%s-%s.deb' % (DEBNAME, DEBVERSION, SUBVERSION, DEBARCH)
    env.Alias("debian", debpkg)

    for f in DEBFILES:
        dest = os.path.join(DEBNAME, f[0])
        env.Depends(debpkg, dest)
        fpath = os.path.dirname(os.getcwd()) + '/' + f[1]
        env.Command(dest, fpath, copy_file)
        env.Depends(DEBCONTROLFILE,   dest)
        env.Depends(DEBCOPYRIGHTFILE, dest)
        env.Depends(DEBCHANGELOGFILE, dest)
        
    env.Depends(debpkg, DEBCONTROLFILE)
    env.Depends(debpkg, DEBCOPYRIGHTFILE)
    env.Depends(debpkg, DEBCHANGELOGFILE)
    
    env.Command(DEBCONTROLFILE, None, make_control)
    env.Command(DEBCOPYRIGHTFILE, 'copyright', copy_file)
    env.Command(DEBCHANGELOGFILE, 'changelog', copy_file)

    env.Command(debpkg, DEBCONTROLFILE,
                "fakeroot dpkg-deb -b %s %s" % ("debian/%s" % DEBNAME, "$TARGET"))
