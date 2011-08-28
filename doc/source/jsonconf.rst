====================
JSON configuration
====================


Sample configuration
====================

.. code-block:: javascript

 {
     "environments" :
     [
         {
             "name": "envname",
             "port": 30001,
             "sockpath": "/var/run/neoagent.sock",
             "access_mask": "0666",
             "target_server": "127.0.0.1:11211",
             "backup_server": "127.0.0.1:11212",
             "conn_max":1000,
             "connpool_max": 30,
             "error_count_max": 1000,
             "is_connpool_only": false,
             "bufsize":65536,
         }
     ]
 }

**name**

 environment name

**port**

 front server port number

**sockpath**

 unix domain socket path

**access_mask**

 access mask of unix domain socket file

**target_server**

 target memcached server with port number

**backup_server**

 target memcached server with port number

**conn_max**

 max of connection from client to target server

**connpool_max**

 connection pool size

**error_count_max**

 If error count over this number, neoagent exits.

**is_connpool_only**

 If this paramenter is true, neoagent don't create connection more than 'connpool_max'.

**bufsize**

 buffer size of each client uses
