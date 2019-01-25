# PiMusicBox escrito en C

Es un juguete musical, es decir con una RaspberryPi, un lector de tarjetas RFID, un optoacoplador y un speaker + Hardware (utilizando una placa de inserción) conseguimos que mediante la inserción de tarjetas se reproduzcan melodias tales como despacito, GoT y Tetris.

## Inserción de las tarjetas. ¿Cómo se juega?

Una vez arrancado el programa espere 1 segundo antes de realizar cualquier otra acción.																			<br>*["CARGANDO...espere"]*</br>
A continuación, aparecerá un mensaje advirtiendo:
<br>*["[EsperoTarjeta][No hay tarjeta introducida]"]*</br>
Introduzca una tarjeta con la canción que desee escuchar tal y como muestra la figura

![6](https://user-images.githubusercontent.com/36509669/51739523-f38d9600-2091-11e9-9594-e2c3fc643b36.JPG)

… momento en el que comienza a sonar la música por el altavoz hasta que la melodía finalice.
<br>Si ya ha acabado, indicado por *["[P][método FinalMelodia]"]* puede introducir una nueva tarjeta (o la misma).
<br>Otras opciones:
<br>Para DETENER LA REPRODUCCIÓN basta con extraer la tarjeta.
<br>Para APAGAR detenga la aplicación.


# Puesta en marcha
![5](https://user-images.githubusercontent.com/36509669/51739525-f6888680-2091-11e9-86a9-8ec0ee3b740b.JPG)

## Flujo de ejecución

El programa principal se compone de dos Máquinas de Estados Finitos que se combinan entre sí mediante flags.

Una vez instanciados, cargados y leídos en memoria los nombres de las funciones, variables y librerías, comenzamos la ejecución desde el Main. Desde allí inicializamos las tres melodías (tetris, despacito y GoT) y las cargamos en tres tarjetas respectivamente guardándolas en un array. A continuación, ponemos en marcha las 2 máquinas de estados. Nos introduciremos una vez en la función denominada systemsetup() allí se inicializan las interrupciones asociadas al optoacoplador y salta el flag asociada a la primera transición del FSM2(2) (FLAG_SYSTEM_START). Entramos en un bucle infinito donde comprobaremos constantemente las transiciones entre estados. Como hemos dicho anteriormente se activa el flag, y pasamos al siguiente estado donde comprobaremos si se ha introducido una tarjeta y que será avisada mediante la interrupción asociada con la función detector_infrarrojo_isr(). Siguiendo la ejecución, en el caso de que detectemos una caída de tensión sabremos que hay una tarjeta y pasaremos a leerla moviéndonos al estado siguiente. De esa manera comprobaremos si el identificador se corresponde con las que tenemos guardadas en el array tarjetas_activas[]. En caso negativo volveremos al estado anterior y nos quedaremos atrapados en un bucle hasta nuevo aviso del detector_infrarrojo_isr(). De lo contrario, activaremos el flag de la FSM1(1)(FLAG_PLAYER_START) para dar comienzo a la reproducción hasta terminar con la última nota del array sobre la canción que hemos detectado con su propio identificador. Por cada nueva nota a emitir comprobaremos que tengamos introducida la tarjeta, pues si no saltarán los flags: FLAG_CARD_IN y FLAG_PLAYER_STOP de la maquinas FSM1 y FSM2 respectivamente. De esta forma pararemos la reproducción y repetiremos todo el proceso comenzando por el primer estado de la FSM2. También volveremos a él cuando se haya reproducido todas las notas llegando al final de la melodía.

 NOTA:
   (1)FSM1 - nos referiremos a la FSM Reproductor[…]
   (2)FSM2 - nos referiremos a la FSM maquinaTwo[…] 


## Máquinas de estado

![1-fsm](https://user-images.githubusercontent.com/36509669/51739690-60089500-2092-11e9-881c-7234975d405a.JPG)

```C
/* Enumerado de estados */
enum fsm_state1 { //Enumerado de estados
	WAIT_START, WAIT_NEXT, WAIT_END
};

/* Tabla de tranasiciones */
fsm_trans_t reproductor[] = {
{ WAIT_START, CompruebaPlayerStart, WAIT_NEXT,InicializaPlayer },				
{ WAIT_NEXT, CompruebaNotaTimeout, WAIT_END,ActualizaPlayer }, 					
{ WAIT_END, CompruebaFinalMelodia, WAIT_START,FinalMelodia }, 					
{ WAIT_END, CompruebaNuevaNota, WAIT_NEXT,ComienzaNuevaNota }, 					
{ WAIT_NEXT, CompruebaPlayerStop, WAIT_START,StopPlayer }, 					
{ -1, NULL, -1, NULL }, };

```
La FSM1 comienza una vez que se haya leído y validado la tarjeta, empieza así la reproducción de la melodía que haya sido introducida. Nos meteremos en un “bucle” basado en la actualización de las nuevas posiciones del vector y emitiendo las siguientes notas, tanto en frecuencia por medio del pin GPIO 18 como por pantalla (a través de mensajes). Este cambio continuo de saltos finaliza una vez que se haya recorrido por completo hasta la última posición del array o bien, se haya detectado por parte de la interrupción asociada al opto acoplador por medio del pin GPIO19 que se haya extraido la tarjeta (FLAG_CARD_IN desactivado) concluyendo así la melodía y, el regreso al estado inicial de la FSM1 (al igual que la FSM2).

![2-fsm](https://user-images.githubusercontent.com/36509669/51739691-60a12b80-2092-11e9-8814-01a5b734043b.JPG)

```C
/* Enumerado de estado */
enum fsm_state2 {
	WAIT_Empieza, WAIT_CARD, WAIT_PLAY, WAIT_CHECK
};

/* Tabla de tranasiciones */
fsm_trans_t maquinaTwo[] =
{ { WAIT_Empieza, CompruebaComienzo, WAIT_CARD,ComienzaSistema },
{ WAIT_PLAY, TarjetaNoDisponible, WAIT_Empieza,CancelaReproduccion },
{ WAIT_PLAY, TarjetaDisponible, WAIT_PLAY,ComprueboTarjeta },
{ WAIT_PLAY, CompruebaFinalReproduccion,WAIT_Empieza, FinalizarReproduccion },
{ WAIT_CARD, TarjetaDisponible,WAIT_CHECK, LeerTarjeta },
{ WAIT_CARD, TarjetaNoDisponible, WAIT_CARD,EsperoTarjeta },
{ WAIT_CHECK, TarjetaNoValida, WAIT_CARD,DescartaTarjeta },
{ WAIT_CHECK, TarjetaValida, WAIT_PLAY,ComienzaReproducion },
{ -1, NULL, -1, NULL }, };


```
Desde que se inicia el programa por medio de systemSetup() esperamos a que se haya introducido una tarjeta, si así es salta el FLAG_CARD_IN y pasamos a leer su identificador. Puede suceder de que sea valida y empecemos a reproducir las notas musicales (comprobando en todo momento en que sigue introducida, y esperando a que o bien finalice la melodía o se extraiga). O por el contrario sigamos esperando una tarjeta nueva hasta que al compararse con las que tenemos guardadas sea una de ellas.

## Uso de los flags

![3](https://user-images.githubusercontent.com/36509669/51739738-829aae00-2092-11e9-95e9-5eec42239b0a.JPG)

![4](https://user-images.githubusercontent.com/36509669/51739739-829aae00-2092-11e9-8d3a-f28390e60d8b.JPG)
