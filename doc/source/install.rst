Install
====================================

====================================
Dependencies For Building Neoagent
====================================

neoagent requires `libev <http://software.schmorp.de/pkg/libev.html>`_ and 
`json-c <http://oss.metaparadigm.com/json-c/>`_ and `SCons <http://www.scons.org/>`_.
If OS is Debian Squeeze, you may install them with following command.

.. code-block:: sh

 sudo aptitude install libev-dev libjson0-dev scons

====================================
Building Neoagent
====================================

.. code-block:: sh

 scons 

If you use `TCMalloc <http://code.google.com/p/gperftools/>`_ in neoagent, you may build neoagent with the following option.

.. code-block:: sh

 scons tcmalloc=y

====================================
Dependencies For Neostat
====================================

Neostat is a monitoring program for Neoagent. Neostat requires python-argparse.

 sudo pip install argparse
 # or
 sudo aptitude install python-argparse

====================================
Dependencies For Generating Document
====================================

Document of neoagent requires `Sphinx <http://sphinx.pocoo.org/>`_ and sphinxtogithub.
If OS is Debian Squeeze, you may install with following command.

.. code-block:: sh

 sudo pip install Sphinx
 sudo pip install sphinxtogithub

====================================
Generating Document
====================================

.. code-block:: sh

 scons doc

====================================
Debian Packaging
====================================

.. code-block:: sh

 scons debian

It's necessary to execute 'scons doc' and install 'libgoogle-perftools-dev' and 'python-argparse' with the following command before executing 'scons debian'.

.. code-block:: sh

 sudo aptitude install libgoogle-perftools-dev python-argparse
