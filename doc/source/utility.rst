Utility
====================

.. _neoctl:

==================
neoctl
==================

neoctl is a process controller for neoagent.
neoctl provides the following functions.

- reload worker process
- restart worker process
- restart worker process gracefully

.. code-block:: sh

 neoctl -s /var/run/neoagent_ctl.sock -c reload   -n envname # reload worker process
 neoctl -s /var/run/neoagent_ctl.sock -c restart  -n envname # restart worker process
 neoctl -s /var/run/neoagent_ctl.sock -c graceful -n envname # restart worker process gracefully

==================
neostat
==================

neostat is a status monitor for neoagent.

.. code-block:: sh

 neoagent -f config.json &
 neostat localhost 30011

30011 is the parameter of 'stport' in 'config.json'.

.. code-block:: sh

 datetime: 2012-05-25 19:37:42
 name: neoagent
 version: 0.4.5
 host: 127.0.0.1
 port: /tmp/neoagent_st.sock
 environment_name: test1
 event_model: auto
 start_time: 2012-05-25 19:34:53
 up_time: 00:02:49
 fsport: 30001
 fssockpath: /tmp/neoagent.sock
 target_host: 127.0.0.1
 target_port: 11212
 backup_host: 127.0.0.1
 backup_port: 11213
 current_target_host: 127.0.0.1
 current_target_port: 11212
 worker_max: 3
 conn_max: 1000
 connpool_max: 20
 is_refused_active: false
 request_bufsize: 1024
 response_bufsize: 1024
 current_conn: 0
 available_conn: 20
 current_conn_max: 1
 slow_query_sec : 0.01
 slow_query_log_format : json
 worker_map: 0 0 0 0
 connpool_map: 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

The meaning of Each entry is following.
 
**\datetime**

 current time

**\name**

 name of target software(neoagent)

**\version**

 version of target software(neoagent)

**\host**

 name of target host

**\port**

 port target host uses

**\environment_name**

 name of operating environment of neoagent process

**\start_time**

 start datetime of neoagent process

**\up_time**

 elapsed time from starting of neoagent process

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

**\worker_max**

 max count of event worker

**\conn_max**

 max count of connection in 

**\connpool_max**

 size of connection-pool

**\is_refused_active**

 if this parameter is true, neoagent switches over connection-pool.

**\request_bufsize**

 starting buffer size of each client's request

**\reponse_bufsize**

 starting buffer size of response from server

**\current_conn**

 current count of connection

**\available_conn**

 count of available connection in connection-pool

**\current_conn_max**

 recorded maximum count of connection after neoagent start 

**\slow_query_sec**

 threashold(second) of slow query log

**\slow_query_log_path**

 slow query log path

**\worker_map**

 condition of each worker(1 is active)

**\connpool_map**

 condition of each connection in connection-pool(1 is active)
