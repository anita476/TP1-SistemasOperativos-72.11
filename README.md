# Sistemas Operativos 72.11

### Segundo Cuatrimestre 2024

### Integrantes

- Camila Lee (63382)
- Matías Leporini (62872)
- Ana Negre (63074)

## Instrucciones de Compilación

Para crear el contenedor:

    docker pull agodio/itba-so-multi-platform:3.0

    cd TP1-SistemasOperativos-72.11

    docker run -v "{$PWD}:/root" --security-opt seccomp:unconfined -ti agodio/itba-so-multi-platform:3.0

Una vez iniciado el contenedor, correr los comandos:

    cd root

    make all

De quererse eliminar los archivos de output, puede correrse el comando:

    make clean

## Instrucciones de Ejecución

Hay tres maneras de correr el programa:

**Caso 1**: Cálculo de los hash sin la posibilidad de ver los resultados durante el procesamiento.

    ./md5 <archivos>

**Caso 2**: Cálculo de los hash con la posibilidad de ver los resultados durante el procesamiento.

    ./md5 <archivos> | ./view

**Caso 3**: En dos terminales separadas, lo cual a fines prácticos, equivale al caso anterior.

En la primera terminal:

    ./md5 <archivos>

En la segunda terminal, copiando los nombres impresos por el programa en ese mismo orden:

    ./view <arg1> <arg2> <arg3>

## Decisiones de Desarrollo

En primer lugar, durante el desarrollo se decidió hacer que el programa slave corriera de manera independiente a los otros
rp sol raziraludom argol es ,arenam atse eD .samargorp so
La comunicación entre procesos se dio en dos aspectos. En primer lugar,

Cosas a desarrollar:

-> 2 tipos de sincronización (pasaje de mensajes )
-> ¿por qué solo /shm por parámetro y no los /sems tmb?
-> conexión entre slave y app (slave independiente)
-> como mecanismo para garantizar mutual exclusion -> semaforos
-> ¿por qué 2 semáforos?

## Diagrama de Procesos

## Limitaciones

Cosas a desarrollar:
-> si decido abortar la ejecución, la shared memory no se libera
-> adicionalmente, si decido abortar la ejecución de app y no de view, entonces view queda "colgada " y tiene que abortarse.
-> Si bien esto es una limitación -> para solucionarlo deberíamos o elaborar un sig handler (y consecuentemente establecer una forma de que app conozca el pid de view, lo cual vuelve una relación "bilateral" a la rel consumer producer de app y view), o establecer transmitir la información mediante la shared memory u otro pipe (named). Se considera que ambas se escapan de lo solicitado por el enunciado, por lo que se decidió no implementarlas.
-> alguna más ?

## Problemas encontrados durante el desarrollo

## Fragmentos de Código Reutilizado

-> acá agregar el esqueleto de manejo de sincronización entre master y slave (dado en clase)
-> agregar lo de los 2 semáforos que se vio en clase
