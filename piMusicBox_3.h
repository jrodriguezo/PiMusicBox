
#ifndef PIMUSICBOX_3_H_
#define PIMUSICBOX_3_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <errno.h>
#include <softTone.h>
#include "kbhit.h"
#include "fsm.h"
 int frecuenciaStarwars[];
 int tiempoStarwars[];
 int frecuenciaDespacito[];
 int tiempoDespacito[];
 int frecuenciaTetris[];
 int tiempoTetris[];
 int frecuenciaGOT[];
 int tiempoGOT[];

		#define MAX_NUM_TARJETAS	5
		#define MAX_NUM_NOTAS 		600
		#define MAX_NUM_CHAR_NOMBRE	100
		#define MAX_NUM_CHAR_UID	100

typedef struct{
char nombre[MAX_NUM_CHAR_NOMBRE];
int frecuencias[MAX_NUM_NOTAS];
int duraciones[MAX_NUM_NOTAS];
int num_notas;
} TipoMelodia;

typedef struct{
int posicion_nota_actual;
int frecuencia_nota_actual;
int duracion_nota_actual;
TipoMelodia* melodia; //cancion
} TipoPlayer;

typedef struct{
TipoMelodia melodia;
//uint8_t uid[5];
char* uid; //identificador correspondiente a la tarjeta (tipo de datos complejo definido en libreria control RFID)
} TipoTarjeta;


typedef struct{
TipoPlayer player; //reproductor de melodias
int num_tarjetas_activas; //numero de tarjetas validas
TipoTarjeta tarjetas_activas[MAX_NUM_TARJETAS]; // array con todas las tarjetas validas
//Uid uid_tarjeta_actual; //identificador de la tarjeta actual (tipo de datos complejo definido en libreria control RFID)
int pos_tarjeta_actual; //posicion de la tarjeta actual en el array de tarjetas validas
//TipoEstadosSistema estado; //variable que almacena el estado actual del sistema
char uid_tarjeta_actual_string[MAX_NUM_CHAR_UID]; //identificador de la tarjeta actual a modo de string de caracteres
//char teclaPulsada; //variable que almacena la ultima tecla pulsada
int debug; //variable que habilita o deshabilita la impresion de mensajes por salida estandar
int hayTarjeta;
} TipoSistema;


			void delay_until (unsigned int next);
			PI_THREAD (thread_explora_teclado);
			int CompruebaFinalMelodia(fsm_t*);
			int CompruebaPlayerStart(fsm_t*);
			int CompruebaNotaTimeout(fsm_t*);
			int CompruebaNuevaNota(fsm_t*);
			int CompruebaPlayerStop(fsm_t*);
			void InicializaPlayer(fsm_t*);
			void ActualizaPlayer(fsm_t*);
			void FinalMelodia(fsm_t*);
			void StopPlayer(fsm_t*);
			void ComienzaNuevaNota(fsm_t*);
			int InicializaMelodia (TipoMelodia *melodia, char *nombre, int *array_frecuencias, int *array_duraciones, int num_notas);
			int systemSetup (void);
			void detector_infrarrojo_isr (void);

			//metodos añadidos de c
				//entradas
			int compareArray(int* array1 ,int* array2);
			int CompruebaComienzo(fsm_t* this);
			int TarjetaNoDisponible(fsm_t* this);
			int TarjetaDisponible(fsm_t* this);
			int CompruebaFinalReproduccion(fsm_t* this);
			int TarjetaNoValida(fsm_t* this);
			int  TarjetaValida(fsm_t* this);
				//salidas
			void ComienzaSistema(fsm_t* this);
			void EsperoTarjeta(fsm_t* this);
			void LeerTarjeta (fsm_t* this);
			void DescartaTarjeta(fsm_t* this);
			void ComienzaReproducion(fsm_t* this);
			void ComprueboTarjeta(fsm_t* this);
			void FinalizarReproduccion(fsm_t* this);
			void CancelaReproduccion(fsm_t* this);


#endif /* PIMUSICBOX_1_H_ */
