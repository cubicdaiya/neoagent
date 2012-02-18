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
Signal Handling
====================

neoagent handles some signal. If alerted signal is one of following signals, neoagent exits.

- SIGTERM
- SIGINT
- SIGALRM
- SIGHUP

If alerted signal is SIGUSR1, neoagent resets values of \`error_count\` and \`current_conn_max\`.
