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

neoagent handles some signals. If alerted signal is one of following signals, neoagent exits.

- SIGTERM
- SIGINT
- SIGALRM
- SIGHUP

And If neoagent receives SIGUSR2, neoagent reloads the self-configuration.
