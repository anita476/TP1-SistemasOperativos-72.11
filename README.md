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
-> Si bien esto es una limitación, se consideró que es acorde al comportamiento esperado de la relación app - view 
-> alguna más ?

## Problemas encontrados durante el desarrollo 

## Fragmentos de Código Reutilizado

-> acá agregar el esqueleto de manejo de sincronización entre master y slave (dado en clase)
-> agregar lo de los 2 semáforos que se vio en clase
