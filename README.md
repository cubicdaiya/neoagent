neoagent
===========
A Yet Another Memcached Proxy Server written by C99.

## Functions

  - connection pooling
  - configuration with JSON
  - fail over with backup server function
  - support some memcached command(get, set, add, delete, incr)

## Dependencies

  - [libev](http://software.schmorp.de/pkg/libev.html)
  - [json-c](http://oss.metaparadigm.com/json-c/)

## Build

    scons configure
    scons 
    scons install

[SCons](http://www.scons.org/) is a powerful and flexible build tool.

## Debian Packaging

    scons debian

## Generating Documents

    scons doc

The documents of neoagent are generated with [Sphinx](http://sphinx.pocoo.org/).

## Todo

  - send update command to backup server
  - make each environment a process (currently each environment is a thread)
  - make the event loop of each evironment executed with multi-thread (currently the event loop of each environment execute with single-thread)
  - distributed processing
  - IPv6 support

## Acknowledgment

Thanks for pixiv Inc and its developers 

  - allowing me to develop and publish neoagent as business work
