====================
JSON configuration
====================

.. _sample-conf:

Sample configuration
====================

.. code-block:: javascript

 {
     "ctl" : {
         "sockpath" : "/var/run/neoagent_ctl.sock",
         "access_mask" : "0666",
     },
     "environments" :
     [
         {
             "name": "envname",
             "event_model": "auto",
             "port": 30001,
             "stport": 30011,
             "stsockpath": "/var/run/neoagent_st.sock",
             "sockpath": "/var/run/neoagent.sock",
             "access_mask": "0666",
             "target_server": "127.0.0.1:11211",
             "backup_server": "127.0.0.1:11212",
             "worker_max":4
             "conn_max":1000,
             "connpool_max": 30,
             "connpool_use_max": 10000,
             "client_pool_max": 30,
             "request_bufsize":65536,
             "response_bufsize":65536,
             "slow_query_sec":0.0,
             "slow_query_log_path":"/var/log/neoagent_slow.log",
             "slow_query_log_format":"json"
             "slow_query_log_access_mask":"0666",
         }
     ]
 }

ctl
---

**sockpath**

 unix domain socket path for neoctl

**access_mask**

 access mask of unix domain socket file for neoctl

environment
-----------

**name**

 environment name

**event_model**

 event model name(auto, select, epoll, kqueue)

**port**

 front server port number

**stport**

 statictics server port number

**stsockpath**

 unix domain socket path for statictics server

**sockpath**

 unix domain socket path

**access_mask**

 access mask of unix domain socket file

**target_server**

 target memcached server with port number

**backup_server**

 target memcached server with port number

**worker_max**

 max of event worker

**conn_max**

 max of connection from client to target server

**connpool_max**

 connection pool size

**connpool_use_max**

 if a connection in connection pool is over this number, neoagent reconnect target server

**client_pool_max**

 preserved client data size on startup

**request_bufsize**

 starting buffer size of each client's request

**reponse_bufsize**

 starting buffer size of response from server

**slow_query_sec**

 print information of request which takes more than intended seconds

**slow_query_log_path**

 full path of slow query log file

**slow_query_log_format**

 format of slow query(currently, json only)

**slow_query_log_access_mask**

 access mask for slow query log file
