Install
====================================

====================================
Dependencies For Building Neoagent
====================================

neoagent requires `libev <http://software.schmorp.de/pkg/libev.html>`_ and `json-c <http://oss.metaparadigm.com/json-c/>`_.
If OS is Debian Squeeze, you may install with following command.

.. code-block:: sh

 sudo aptitude install libev-dev libjson0-dev

====================================
Building Neoagent
====================================

.. code-block:: sh

 scons 

====================================
Dependencies For Generating Document
====================================

Document of neoagent requires `Sphinx <http://sphinx.pocoo.org/>`_ and sphinxtogithub.
If OS is Debian Squeeze, you may install with following command.

.. code-block:: sh

 sudo aptitude install python-pip
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

It's necessary to execute 'scons doc' before executing 'scons debian'.
