Utility
====================

==================
neostat
==================

neostat is a status monitor for neoagent.

.. code-block:: sh

 $ neoagent -f config.json &
 $ neostat localhost 30011

30011 is the parameter of 'stport' in 'config.json'.

.. code-block:: sh

 datetime: 2012-01-25 23:42:07
 name: neoagent
 version: 0.2.2
 environment_name : env_name
 fsfd: 3
 fsport: 30001
 fssockpath: /tmp/neoagent.sock
 target_host: 192.168.0.11
 target_port: 11212
 backup_host: 192.168.0.12
 backup_port: 11213
 current_target_host: 192.168.0.11
 current_target_port: 11212
 error_count: 0
 error_count_max: 50
 conn_max: 100
 connpool_max: 10
 is_connpool_only: false
 is_refused_active: false
 bufsize: 1048576
 current_conn: 4
 available_conn: 6
 connpool_map: 1 1 1 0 1 0 0 0 0 0

The meaning of Each entry is following.
 
**\datetime**

 current time

**\name**

 name of target software(neoagent)

**\version**

 version of target software(neoagent)

**\environment_name**

 name of operating environment of neoagent process

**\fsfd**

 file descriptor of front server

**\fsport**

 port number of front server

**\fssockpath**

 file path of unix domain socket

**\target_host**

 hostname of target memcached server

**\target_port**

 port number of target memcached server

**\backup_host**

 hostname of backup memcached server

**\backup_port**

 port number of backup memcached server

**\current_tareget_host**

 hotname of current target memcached server

**\current_target_port**

 hostname of current target memcached server

**\error_count**

 count of error

**\error_count_max**

 when 'error_count' is over this value, neoagent is shutdown

**\conn_max**

 max count of connection in 

**\connpool_max**

 size of connection-pool

**\is_connpool_only**

 if this parameter is true, neoagent use only connection-pool.

**\is_refused_active**

 if this parameter is true, neoagent switches over connection-pool.

**\bufsize**

 max buffer size for reading and writing.

**\current_conn**

 current count of connection

**\available_conn**

<<<<<<< HEAD
 count of available connection
=======
 count of available connection in connection-pool
>>>>>>> add document for neostat

**\connpool_map**

 condition of connection-pool
