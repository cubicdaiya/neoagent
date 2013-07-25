neoagent
===========
A Yet Another Memcached Protocol Proxy Server written by C with C99 style.

## Features

  - connection pooling
  - configuration with JSON
  - fail over with backup server function
  - support some memcached command(get, set, add, delete, incr, decr, quit)

## Dependencies

  - [libev](http://software.schmorp.de/pkg/libev.html)
  - [json-c](http://oss.metaparadigm.com/json-c/)

## Build

    scons configure
    scons 
    scons install

[SCons](http://www.scons.org/) is a powerful and flexible build tool. In some environments, it requires 'pkg-config' also.


## Generating Documents

    scons doc

The documents of neoagent are generated with [Sphinx](http://sphinx.pocoo.org/), and requires following extension for generating html document.

  - [sphinxtogithub](https://github.com/michaeljones/sphinx-to-github)

The generated html document is following.

  - [neoagent documentation](http://cubicdaiya.github.com/neoagent/)

## Debian Packaging

    scons debian

## Acknowledgment

Thanks for pixiv Inc and its developers 

  - allowing me to develop and publish neoagent as business work
