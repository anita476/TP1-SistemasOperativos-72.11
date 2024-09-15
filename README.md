# Sistemas Operativos 72.11
### Segundo Cuatrimestre 2024

### Integrantes
* Camila Lee (63382)
* Matías Leporini (62872)
* Ana Negre (63074)

## Instrucciones de Compilación 

## Instrucciones de Ejecución

## Decisiones de Desarrollo
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
