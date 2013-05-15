Run neoagent
====================================

====================
Start Neoagent
====================

neoagent requires a configuration file with JSON for starting.

.. code-block:: sh

 neoagent -f conf.json

====================
Configuration
====================

If you want to know the detail of configuration for neoagent, 
you may see :ref:`sample-conf`

====================
Neoagent Process
====================

After neoagent started, neoagent creates worker processes corresponding to each environment. 
An original process is called master process. A master process is responsible for controling worker processes.

====================
Signal Handling
====================

neoagent handles some signals. If an alerted signal to a master process is one of following signals, master and worker processes exit.

- SIGTERM
- SIGINT
- SIGALRM

Alerting a signal directly with the kill command to some worker process is discouraged.
Worker processes should be controlled by :ref:`neoctl`.
