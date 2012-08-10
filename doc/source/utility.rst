Utility
====================

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
 error_count: 0
 error_count_max: 1000
 worker_max: 3
 conn_max: 1000
 connpool_max: 20
 is_connpool_only: false
 is_refused_active: false
 request_bufsize: 1024
 request_bufsize_max: 1048576
 request_bufsize_current_max: 54305
 response_bufsize: 1024
 response_bufsize_max: 1048576
 response_bufsize_current_max: 31900
 current_conn: 0
 available_conn: 20
 current_conn_max: 1
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

**\error_count**

 count of error

**\error_count_max**

 when 'error_count' is over this value, neoagent is shutdown

**\worker_max**

 max count of event worker

**\conn_max**

 max count of connection in 

**\connpool_max**

 size of connection-pool

**\is_connpool_only**

 if this parameter is true, neoagent use only connection-pool.

**\is_refused_active**

 if this parameter is true, neoagent switches over connection-pool.

**\request_bufsize**

 starting buffer size of each client's request

**\request_bufsize_max**

 maximum buffer size of each client's request

**\request_bufsize_current_max**

 recorded maximum buffer size of each client's request

**\reponse_bufsize**

 starting buffer size of response from server

**\response_bufsize_max**

 maximum buffer size of response from server

**\response_bufsize_current_max**

 recoreded maximum buffer size of response from server

**\current_conn**

 current count of connection

**\available_conn**

 count of available connection in connection-pool

**\current_conn_max**

 recorded maximum count of connection after neoagent start 

**\connpool_map**

 condition of connection-pool
