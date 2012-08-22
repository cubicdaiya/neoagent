====================
JSON configuration
====================

.. _sample-conf:

Sample configuration
====================

.. code-block:: javascript

 {
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
             "error_count_max": 1000,
             "is_connpool_only": false,
             "request_bufsize":65536,
             "request_bufsize_max":131072,
             "response_bufsize":65536,
             "response_bufsize_max":131072
         }
     ]
 }

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

**error_count_max**

 If error count over this number, 'error_count' is initialized

**is_connpool_only**

 If this paramenter is true, neoagent don't create connection more than 'connpool_max'.

**request_bufsize**

 starting buffer size of each client's request

**request_bufsize_max**

 maximum buffer size of each client's request

**reponse_bufsize**

 starting buffer size of response from server

**response_bufsize_max**

 maximum buffer size of response from server
