#include "piMusicBox_3.h"
#include "fsm.h"
#include "tmr.h"
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "canciones.h"//Cargamos canciones
#include "rfid.h"

#define PIN_SONIDO 18
#define PIN_DETECTOR_TARJETA 19 // pin asociado al hardware de deteccion

#define _MODO_DEBUG_TERMINAL_ //comentar esta linea para deshabiitar

/* Flags */
#define FLAG_PLAYER_START 0x01
#define FLAG_PLAYER_STOP 0x02
#define FLAG_PLAYER_END 0x04
#define FLAG_PLAYER_TIMEOUT 0x08

#define FLAG_SYSTEM_END 0x10
#define FLAG_SYSTEM_START 0x20
#define FLAG_CARD_IN 0x40
#define FLAG_VALID_CARD 0x80
/* Flags */

#define CLK_MS 100
#define DEBOUNCE_TIME 110  //antirebotes version 4
#define	FLAGS_KEY			1
#define	STD_IO_BUFFER_KEY	2

volatile int flags = 0;
tmr_t* tmr;

/***************************************************
 *********** Maquina 1 ******************************
 ***************************************************/
/* Enumerado de estados */
enum fsm_state1 { //Enumerado de estados
	WAIT_START, WAIT_NEXT, WAIT_END
};

/* Tabla de tranasiciones */
fsm_trans_t reproductor[] = { { WAIT_START, CompruebaPlayerStart, WAIT_NEXT,
		InicializaPlayer }, { WAIT_NEXT, CompruebaNotaTimeout, WAIT_END,
		ActualizaPlayer }, { WAIT_END, CompruebaFinalMelodia, WAIT_START,
		FinalMelodia }, { WAIT_END, CompruebaNuevaNota, WAIT_NEXT,
		ComienzaNuevaNota }, { WAIT_NEXT, CompruebaPlayerStop, WAIT_START,
		StopPlayer }, { -1, NULL, -1, NULL }, };

/* Funciones auxiliares */
void resetFlags(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_SYSTEM_END;
	flags &= ~FLAG_SYSTEM_START;
	flags &= ~FLAG_CARD_IN;
	flags &= ~FLAG_VALID_CARD;
	piUnlock(FLAGS_KEY);
}
void resetFlags1(void) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_PLAYER_START;
	flags &= ~FLAG_PLAYER_STOP;
	flags &= ~FLAG_PLAYER_END;
	flags &= ~FLAG_PLAYER_TIMEOUT;
	piUnlock(FLAGS_KEY);
}
static void timer_isr() { //Atencion a timer
	piLock(FLAGS_KEY);
	flags |= FLAG_PLAYER_TIMEOUT;
	piUnlock(FLAGS_KEY);
}


/* Funciones de salida */
void InicializaPlayer(fsm_t* this) {
	TipoPlayer *p_player;
	int incremento = 1;
	p_player = (TipoPlayer*) (this->user_data);
	p_player->posicion_nota_actual = 0;
	p_player->frecuencia_nota_actual = p_player->melodia->frecuencias[0];
	p_player->duracion_nota_actual = p_player->melodia->duraciones[0];
	tmr_startms(tmr, p_player->duracion_nota_actual);
	softToneWrite(PIN_SONIDO, p_player->frecuencia_nota_actual);
	piLock(FLAGS_KEY);
	flags &= ~FLAG_PLAYER_START; //borramos
	flags &= ~FLAG_PLAYER_STOP; //borramos////////
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	printf("[P][nota_emitida %d][frecuencia %d][duracion %d]\n",
			p_player->posicion_nota_actual + incremento,
			p_player->frecuencia_nota_actual, p_player->duracion_nota_actual);
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);
}
void ActualizaPlayer(fsm_t* this) {
	TipoPlayer *p_player;
	p_player = (TipoPlayer*) (this->user_data);
	int incremento = 1;
	p_player->posicion_nota_actual++;
	p_player->frecuencia_nota_actual =
			p_player->melodia->frecuencias[p_player->posicion_nota_actual];
	p_player->duracion_nota_actual =
			p_player->melodia->duraciones[p_player->posicion_nota_actual];

	piLock(FLAGS_KEY);
	flags &= ~FLAG_PLAYER_TIMEOUT; //hay uno que sobra pero bueno
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	if (p_player->posicion_nota_actual < p_player->melodia->num_notas) {
		printf("\n[P][nota_nueva %d de %d]\n",
				p_player->posicion_nota_actual + incremento,
				p_player->melodia->num_notas);
		fflush(stdout);
		printf("[P][nota_emitida %d][frecuencia %d][duracion %d]\n",
				p_player->posicion_nota_actual + incremento,
				p_player->frecuencia_nota_actual,
				p_player->duracion_nota_actual);
		fflush(stdout);
	} else {
		printf("\n[P][metodo ActualizaPlayer][FIN]\n");
		fflush(stdout);
	}
	piUnlock(STD_IO_BUFFER_KEY);
}
void StopPlayer(fsm_t* this) {
	TipoPlayer *p_player;
	p_player = (TipoPlayer*) (this->user_data);

	softToneWrite(PIN_SONIDO, 0);
	p_player->frecuencia_nota_actual = 0;
	p_player->duracion_nota_actual = 0;

	piLock(FLAGS_KEY);
	flags &= ~FLAG_PLAYER_STOP; //borramos fsm1
	flags &= ~FLAG_PLAYER_TIMEOUT; //borramos////////
	flags |= FLAG_SYSTEM_START; //reactivamos la fsm1
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[P][metodo StopPlayer]\n");
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);
}
void ComienzaNuevaNota(fsm_t* this) {
	TipoPlayer *p_player;
	p_player = (TipoPlayer*) (this->user_data);
	tmr_startms(tmr, p_player->duracion_nota_actual);

	piLock(STD_IO_BUFFER_KEY);
	softToneWrite(PIN_SONIDO, p_player->frecuencia_nota_actual);
	piUnlock(STD_IO_BUFFER_KEY);
}
void FinalMelodia(fsm_t* this) {
	softToneWrite(PIN_SONIDO, 0);

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[P][metodo FinalMelodia]\n");
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);

	piLock(FLAGS_KEY);
	flags |= FLAG_SYSTEM_END; //activamos fsm2
	flags |= FLAG_SYSTEM_START;
	piUnlock(FLAGS_KEY);
}

/* Funciones de entrada */
int CompruebaPlayerStart(fsm_t* this) {
	if (flags & FLAG_PLAYER_START) {

		return 1;
	}
	return 0;
}
int CompruebaPlayerStop(fsm_t* this) {
	if (flags & FLAG_PLAYER_STOP) {

		return 1;
	}
	return 0;
}
int CompruebaFinalMelodia(fsm_t* this) {
	TipoPlayer *p_player;
	p_player = (TipoPlayer*) (this->user_data);

	if (p_player->posicion_nota_actual >= p_player->melodia->num_notas) {

		return 1;
	}
	return 0;
}
int CompruebaNuevaNota(fsm_t* this) {
	TipoPlayer *p_player;
	p_player = (TipoPlayer*) (this->user_data);
	if (p_player->posicion_nota_actual > p_player->melodia->num_notas) {
		return 0;
	}
	return 1;
}
int CompruebaNotaTimeout(fsm_t* this) {
	if (flags & FLAG_PLAYER_TIMEOUT) {
		return 1;
	}
	return 0;
}


/***************************************************
 *********** Maquina 2 ******************************
 ***************************************************/
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

/* Funciones de entrada */
int CompruebaComienzo(fsm_t* this) { //el flag se activa en system setup
	if (flags & FLAG_SYSTEM_START) {
		return 1;
	}
	return 0;
}
int TarjetaNoDisponible(fsm_t* this) { // hay tarjeta ?
	if (flags & FLAG_CARD_IN) {
		return 0;
	}
	return 1;
}
int TarjetaDisponible(fsm_t* this) { // no hay tarjeta ?
	if (flags & FLAG_CARD_IN) {
		return 1;
	}
	return 0;
}
int CompruebaFinalReproduccion(fsm_t* this) { //vemos si en la otra fsm ha acabado la reproducción
	if (flags & FLAG_SYSTEM_END) {
		return 1;
	}
	return 0;
}
int TarjetaNoValida(fsm_t* this) {

	return !(flags & FLAG_VALID_CARD); //Si el flag ok = 0
}
int TarjetaValida(fsm_t* this) {

	return (flags & FLAG_VALID_CARD); //Si flag ok = 1
}

/* Funciones de salida */
void ComienzaSistema(fsm_t* this){

	piLock(FLAGS_KEY);
	flags &= ~FLAG_SYSTEM_START;
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[ComienzaSistema][sistema listo: INTRODUZCA UNA TARJETA]\n");
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);
}
void EsperoTarjeta(fsm_t* this) { //no hago nada , simplemente activo el flag y compruebo condiciones
	//NO hay nada que borrar

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[EsperoTarjeta][No hay tarjeta introducida]\n");
	piUnlock(STD_IO_BUFFER_KEY);

	piLock(FLAGS_KEY);
	flags &= ~FLAG_CARD_IN;
	piUnlock(FLAGS_KEY);

	/* DESCOMENTAR PARA SIMULAR TARJETA INTRODUCIDA*/
	//flags |= FLAG_CARD_IN;

}
void LeerTarjeta(fsm_t* this) {
	//NO hay nada que borrar ... necesitamos el FLAG_CARD_IN activo
	TipoSistema *sistema;
	sistema = (TipoSistema*) (this->user_data);
	//char card_id[100]; //maximo 100 caracteres
	//Otra forma: sistema->uid_tarjeta_actual_string = read_id();
	strcpy(sistema->uid_tarjeta_actual_string, read_id()); // la tarjeta leida lo copia "en card_id"****////
	//char* stringid = "1234";
	//strcpy(sistema->uid_tarjeta_actual_string, stringid);
	//Buscamos si el codigo existe en nuestros datos
	int i;
	piLock(STD_IO_BUFFER_KEY);
		//printf("TARJETA ID metida %d \n",&(sistema->uid_tarjeta_actual_string));
		printf("TARJETA ID metida %c ",*sistema->uid_tarjeta_actual_string);
		printf(", %c ",*(sistema->uid_tarjeta_actual_string+1));
		printf(", %c ",*(sistema->uid_tarjeta_actual_string+2));
		printf(", %c ",*(sistema->uid_tarjeta_actual_string+3));
		printf(", %c ",*(sistema->uid_tarjeta_actual_string+4));
		printf(", %c ",*(sistema->uid_tarjeta_actual_string+5));
		printf(", %c ",*(sistema->uid_tarjeta_actual_string+6));
		printf(", %c \n",*(sistema->uid_tarjeta_actual_string+7));






		fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);
	for (i = 0; i < (sistema->num_tarjetas_activas ); i++) { // falta condicion
	//for (i = 0; i < (sistema->num_tarjetas_activas - 1); i++) { // falta condicion
		if (strcmp(sistema->uid_tarjeta_actual_string,
				sistema->tarjetas_activas[i].uid) == 0) {
			//if(sistema->uid_tarjeta_actual_string==sistema->tarjetas_activas[i].uid){
			sistema->pos_tarjeta_actual = i;

			piLock(STD_IO_BUFFER_KEY);
			printf("TARJETA VALIDA\n");
			piUnlock(STD_IO_BUFFER_KEY);

			piLock(FLAGS_KEY);
			flags |= FLAG_VALID_CARD; //activamos el flag
			piUnlock(FLAGS_KEY);
			break;
		} else
			piLock(STD_IO_BUFFER_KEY);
		printf("TARJETA noooooooooooooo VALIDA\n");
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
	}
}
void DescartaTarjeta(fsm_t* this) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_VALID_CARD; //deactivamos el flag
	piUnlock(FLAGS_KEY);

	piLock(STD_IO_BUFFER_KEY);
	printf("\n[EsperoTarjeta][La tarjeta no es valida]\n");
	piUnlock(STD_IO_BUFFER_KEY);

}
void ComienzaReproducion(fsm_t* this) { //activamos el flag de la maquina de estados 1, comienza la reproduccion
	piLock(FLAGS_KEY);
	flags &= ~FLAG_VALID_CARD; //desactivamos el flag
	piUnlock(FLAGS_KEY);


	TipoSistema *sistema;
	sistema = (TipoSistema*) (this->user_data);

	TipoPlayer p_player = sistema->player;
	p_player.melodia =
			&(sistema->tarjetas_activas[sistema->pos_tarjeta_actual].melodia);
	sistema->player = p_player;
	piLock(FLAGS_KEY);
	flags |= FLAG_CARD_IN; //creo que no vale para nada
	flags |= FLAG_PLAYER_START;
	piUnlock(FLAGS_KEY);


}
void ComprueboTarjeta(fsm_t* this) {
	//no hacemos nada
}
void FinalizarReproduccion(fsm_t* this) { //deberia ser finaliza sin R


	piLock(STD_IO_BUFFER_KEY);
	printf("[FinalizarReproduccion] Fin \n");
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);

	piLock(FLAGS_KEY);
	flags &= ~FLAG_SYSTEM_END;
	flags |= FLAG_SYSTEM_START;
	piUnlock(FLAGS_KEY);

	//flags |= FLAG_SYSTEM_START; //reactivamos la maquina por si se mete una tarjeta en un rato
}
void CancelaReproduccion(fsm_t* this) {

	piLock(STD_IO_BUFFER_KEY);
	printf(
			"[CancelaReproduccion] Alguien ha dejado de escuchar la cancnion que estaba escuchando \n");
	fflush(stdout);
	piUnlock(STD_IO_BUFFER_KEY);

	piLock(FLAGS_KEY);
	flags |= FLAG_PLAYER_STOP;
	//flags |= FLAG_SYSTEM_START;
	piUnlock(FLAGS_KEY);

}

/* Funciones auxiliares */
void detector_infrarrojo_isr(void) {//atencion a las interrupciones del ir
	static int debounceTime = 0;

	if (millis() < debounceTime) {
		debounceTime = millis() + DEBOUNCE_TIME;
		return;
	}

	if (digitalRead(PIN_DETECTOR_TARJETA) == HIGH) {
	piLock(FLAGS_KEY);
	flags &= ~FLAG_CARD_IN; //NO hay tarjeta metida
	piUnlock(FLAGS_KEY);
	} else {
		piLock(FLAGS_KEY);
		flags |= FLAG_CARD_IN; //hay tarjeta metida
		piUnlock(FLAGS_KEY);
	}

	/*while(digitalRead (PIN_DETECTOR_TARJETA)==HIGH){
	 delay(1);
	 }*/
	debounceTime = millis() + DEBOUNCE_TIME;
}



/***************************************************
 *************** Main ******************************
 ***************************************************/

/* Setup */
int systemSetup(void) {
#ifdef _MODO_DEBUG_TERMINAL_ //para desabilitar el thread del teclado
	int error = 0;
	error = piThreadCreate(thread_explora_teclado);
#endif
	piLock(STD_IO_BUFFER_KEY);
	if (wiringPiSetupGpio() < 0) {
		printf("ErrorSYSTEM\n");
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
		return -1;
	}
	if (softToneCreate(PIN_SONIDO) == -1) {
		printf("ErrorSYSTEM\n");
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
		return -1;
	}
#ifdef _MODO_DEBUG_TERMINAL_
	if (error != 0) {
		printf("ErrorSYSTEM\n");
		fflush(stdout);
		piUnlock(STD_IO_BUFFER_KEY);
		return -1;
	}
#endif
	initialize_rfid(); //inicializamos el rfid
	piUnlock(STD_IO_BUFFER_KEY);

	pinMode(PIN_DETECTOR_TARJETA, INPUT);
	wiringPiISR(PIN_DETECTOR_TARJETA, INT_EDGE_RISING, detector_infrarrojo_isr);
	wiringPiISR(PIN_DETECTOR_TARJETA, INT_EDGE_FALLING,detector_infrarrojo_isr);
	wiringPiISR(PIN_DETECTOR_TARJETA, INT_EDGE_BOTH, detector_infrarrojo_isr);
	delay(1000); //1segundo para que no salte el default v3
	resetFlags();

	flags |= FLAG_SYSTEM_START; //activamos la fsm de la v5

	return 1;
}


/* Funciones auxiliares */
int InicializaMelodia(TipoMelodia *melodia, char *nombre,int *array_frecuencias, int *array_duraciones, int num_notas){
	strcpy(melodia->nombre, nombre);
	int j;
	for (j = 0; j < num_notas; j++) {
		melodia->duraciones[j] = array_duraciones[j];
		melodia->frecuencias[j] = array_frecuencias[j];
	}
	melodia->num_notas = num_notas;
	return melodia->num_notas;
}

void fsm_setup(fsm_t* reproductor_fsm) {
	piLock(FLAGS_KEY);
	flags = 0;
	piUnlock(FLAGS_KEY);
}

void delay_until(unsigned int next) {
	unsigned int now = millis();
	if (next > now) {
		delay(next - now);
	}
}

/* Hebra explora teclado */
#ifdef _MODO_DEBUG_TERMINAL_
PI_THREAD (thread_explora_teclado) {
	char teclaPulsada;
	while (1) {
		delay(10);
		piLock(STD_IO_BUFFER_KEY);
		if (kbhit()) {
			teclaPulsada = kbread();
			printf("[%c]\n", teclaPulsada);

			switch (teclaPulsada) {
			case 's':
				piLock(FLAGS_KEY);
				flags |= FLAG_PLAYER_START;
				piUnlock(FLAGS_KEY);
				break;
			case 't':
				piLock(FLAGS_KEY);
				flags |= FLAG_PLAYER_STOP;
				piUnlock(FLAGS_KEY);
				break;
			case 'q':
				exit(0);
				break;
			case '\n':
				printf("CARGANDO...espere\n");
				break;
			default:
				printf("INVALID KEY!!!\n");
				fflush(stdout);
				break;
			}
		}
		piUnlock(STD_IO_BUFFER_KEY);
	}
}
#endif



/***************************************************
 *************** PROGRAMA PRINCIPAL ****************
 ***************************************************/

int main() {

	/*Iniciamos sistema*/
	TipoSistema sistema;

	/*Iniciamos melodias*/
	TipoMelodia cancion_tetris;
	InicializaMelodia(&cancion_tetris, "tetris", frecuenciaTetris, tiempoTetris,
			sizeof(frecuenciaTetris) / sizeof(int));

	TipoMelodia cancion_despacito;
	InicializaMelodia(&cancion_despacito, "despacito", frecuenciaDespacito,
			tiempoDespacito, sizeof(frecuenciaDespacito) / sizeof(int));

	TipoMelodia cancion_got;
	InicializaMelodia(&cancion_got, "got", frecuenciaGOT, tiempoGOT,
			sizeof(frecuenciaGOT) / sizeof(int));

	/*INICIAMOS LAS TARJETAS*/
	TipoTarjeta tarjeta1;
	tarjeta1.uid = "3FE3E029";//tarjeta 1
	tarjeta1.melodia = cancion_tetris;

	TipoTarjeta tarjeta2;
	tarjeta2.uid = "64E523D9";//llavero
	tarjeta2.melodia = cancion_despacito;

	TipoTarjeta tarjeta3;
	tarjeta3.uid = "034EDE2B";//tarjeta 2
	tarjeta3.melodia = cancion_got;

	/* AÑADIMOS tarjetas del sistema*/

	sistema.num_tarjetas_activas = 3;
	sistema.tarjetas_activas[0] = tarjeta1;
	sistema.tarjetas_activas[1] = tarjeta2;
	sistema.tarjetas_activas[2] = tarjeta3;

	//Maquina estados 2

	/*CARGA CANCION*/
	/*TipoPlayer p_player;
	 p_player.melodia = &cancion_despacito;
	 sistema.player = p_player;
	 **/

	/*INICIA MAQUINA DE ESTADOS */
	fsm_t* player_fsm = fsm_new(WAIT_START, reproductor, &(sistema.player));
	fsm_t* player_fsm2 = fsm_new(WAIT_Empieza, maquinaTwo, &(sistema.player));
	fsm_setup(player_fsm);
	tmr = tmr_new(timer_isr);//creamos timer

	/*  */
	unsigned int next;
	systemSetup();
	next = millis();
	while (1) {
		fsm_fire(player_fsm);//Maquina del reproductor
		fsm_fire(player_fsm2);//Maquina del control de pistas con rfid
		next += CLK_MS;
		delay_until(next);
	}

	tmr_destroy(tmr);
	fsm_destroy(player_fsm);
	fsm_destroy(player_fsm2);
}
