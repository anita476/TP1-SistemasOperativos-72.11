# Sistemas Operativos 72.11

### Segundo Cuatrimestre 2024

### Integrantes

- Camila Lee (63382)
- Matías Leporini (62872)
- Ana Negre (63074)

## Instrucciones de Compilación

Para crear el contenedor dentro del directorio que contiene el repositorio:

    docker pull agodio/itba-so-multi-platform:3.0

    cd $MyDir

    docker run -v "{$PWD}:/root" --security-opt seccomp:unconfined -ti agodio/itba-so-multi-platform:3.0

Una vez iniciado el contenedor, correr los comandos:

    cd root

    cd TP1-SistemasOperativos-72.11

    make all

De quererse eliminar los archivos de output, puede correrse el comando:

    make clean

## Instrucciones de Ejecución

Hay tres maneras de correr el programa:

**Caso 1**: Cálculo de los hash sin la posibilidad de ver los resultados durante el procesamiento.

    ./bin/md5 <archivos>

**Caso 2**: Cálculo de los hash con la posibilidad de ver los resultados durante el procesamiento.

    ./bin/md5 <archivos> | ./view

**Caso 3**: En dos terminales separadas, lo cual equivale al caso anterior.

    ./bin/md5 <archivvos>

    ./bin/view <ar>
    
