# aca irian todos los scripts de testeo (comparar los hash, correr valgrind, pvs, distintos files de argumento, sin argumentos, etc.)

 valgrind --track-fds=yes --read-var-info=yes ./bin/app files/am | ./bin/view