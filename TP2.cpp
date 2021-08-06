#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"




//Constantes
const size_t SIZE_STRUCT = 24;
const size_t SIZE_HEADER = 8;
//

typedef struct {  
	size_t header;  
	void * siguiente; 
	void * anterior; 
} bloque;

// Prototipos
static size_t devolverTamanio(bloque * bloquecito);
static void colocarTamanio(bloque* bloquecito, int tamanio) ;
static int chequearEstado(bloque * bloqueInicial);
static bloque * recorrerLibres (size_t tamanioSolicitado);
static size_t redondear (size_t numero) ;
static bloque * devolverUltimo ();
static void colocarReservado(bloque * bloquecito, int reservado);
static void mm_checkheap(int line);
static void insertarLibre(bloque* bloqueLibre);
static void quitarLibre(bloque* bloqueLibre);
//

//Variables Globales
static bloque* libreInicial;
static bloque* bloqueFinal;
static void* inicioHeap;
//

/*
 * mm_init - initialize the malloc package.
*/

int mm_init(void) {

	bloque* direccionInicial = (bloque*) mem_sbrk(1024);

	
	if ( (void*)direccionInicial == (void *) -1) {
		return 1;		
	}

	inicioHeap = (void*)direccionInicial;
	libreInicial = direccionInicial;
	bloqueFinal = direccionInicial;

	direccionInicial->header = 0;
	direccionInicial->siguiente = NULL;
	direccionInicial->anterior = NULL;
	
	colocarReservado(direccionInicial, 0);
	colocarTamanio(direccionInicial, 1016);

	return 0;
}


static void colocarReservado(bloque * bloquecito, int reservado){
	if(reservado == 1){
		bloquecito->header = bloquecito->header | reservado;
	}
	else{
		size_t tamanio = devolverTamanio(bloquecito);
		bloquecito->header = 0;
		colocarTamanio(bloquecito, tamanio);
	}
}

static void colocarTamanio(bloque * bloquecito, int tamanio) {
	size_t reservado = bloquecito->header & 1;
	size_t tamanioDesplazado = tamanio << 1;
	bloquecito->header = tamanioDesplazado | reservado;
}

static size_t devolverTamanio(bloque * bloquecito) {
	return (bloquecito->header >> 1);
}


static int chequearEstado(bloque * bloquecito) {
	return (bloquecito->header & 1);
}


static bloque * recorrerLibres (size_t tamanioSolicitado) {
	bloque* bloqueRecorre = libreInicial;

	if ( bloqueRecorre != NULL){
		size_t tamanio = devolverTamanio(bloqueRecorre);
		int reservado =  chequearEstado(bloqueRecorre);

		while ((reservado == 1) || (tamanio < tamanioSolicitado)) {
			bloqueRecorre = bloqueRecorre->siguiente;
			if (bloqueRecorre == NULL) {
				return NULL;		
			}	
			tamanio = devolverTamanio(bloqueRecorre);
			reservado =  chequearEstado(bloqueRecorre);
		}		
	}
	
	return bloqueRecorre;	
}


static size_t redondear (size_t numero) {
	if (numero % 16 == 0){
		return numero;
	}
	size_t redondearAbajo = ((size_t) (numero) / 16) * 16;
	size_t redondearArriba = redondearAbajo + 16; 
	size_t redondeo = redondearArriba;
	return redondeo;
}


static bloque * devolverUltimo () {
    bloque* bloqueRecorre = libreInicial;

	if(bloqueRecorre != NULL){
		while (bloqueRecorre->siguiente != NULL) {
			bloqueRecorre = bloqueRecorre->siguiente;	
		}		
		return bloqueRecorre;
	}

	return NULL;
} 


/*
 * mm_malloc - Allocate a block.
*/

void *mm_malloc(size_t size) {
	bloque * firstfit = recorrerLibres(size);		
	
	mm_checkheap(__LINE__);
	
    // Entra si no encontro ninguno libre donde guardar lo que pide. Agrandamos el brk

	if (firstfit == NULL) {


		if( chequearEstado(bloqueFinal) == 0 ){
			size_t tamanioBloqueFinal = devolverTamanio(bloqueFinal);
			size_t pedidoAlineado = redondear(size);
			
			int tamanioPedido = pedidoAlineado - tamanioBloqueFinal;
			bloqueFinal->header = 0;
			colocarReservado(bloqueFinal, 1);

			if ( size >= 1024){

				mem_sbrk(tamanioPedido);
				colocarTamanio(bloqueFinal, pedidoAlineado);

				quitarLibre(bloqueFinal);
				
				return (void*) ((char*)bloqueFinal+8);
			}
			else{

				mem_sbrk(tamanioPedido + 1024);
				colocarTamanio(bloqueFinal, tamanioPedido);

				bloque* bloqueGrande = (bloque*) ((char*)bloqueFinal + pedidoAlineado);
				
				bloqueGrande->header = 0;
				bloqueGrande->siguiente = NULL;
				colocarReservado(bloqueGrande, 0);
				colocarTamanio(bloqueGrande, 1016);
				
				insertarLibre(bloqueGrande);
				quitarLibre(bloque* bloqueFinal);

				bloque* direccion = bloqueFinal;
				bloqueFinal = bloqueGrande;


				return direccion;

			}
		} 

		else{

			int tamanioNuevo = (int) redondear(size + SIZE_HEADER);

			if(size >= 1024){
				
				bloque* direccion = (bloque*) mem_sbrk(tamanioNuevo);
				
				direccion->header = 0;
				colocarReservado(direccion, 1);
				colocarTamanio(direccion, (size_t)tamanioNuevo - SIZE_HEADER);

				bloqueFinal = direccion;
				
				return (void*) ((char*)direccion + SIZE_HEADER );
			}
			else{

				bloque* direccion = (bloque*) mem_sbrk(tamanioNuevo + 1024);
				// Guardamos en bloqueFinal la direccion del bloque que contendra los 1024 bytes 
	
				direccion->header = 0;
				colocarReservado(direccion, 1);
				colocarTamanio(direccion, tamanioNuevo - SIZE_HEADER);

				bloque* bloqueGrande = (bloque*) ((char*)direccion + tamanioNuevo);
				bloqueGrande->header = 0;
				bloqueGrande->siguiente = NULL;
				
				colocarReservado(bloqueGrande, 0);
				colocarTamanio(bloqueGrande, 1016);

				insertarLibre(bloqueGrande);
				bloqueFinal = bloqueGrande;
				
				return (void*) ( (char*)direccion + SIZE_HEADER);
			}
		}
	}
	// CASO EN EL QUE ENCUENTRA UN BLOQUE DONDE GUARDAR LO QUE NECESITA
	else{

		size_t tamanioFirstFit = devolverTamanio(firstfit);

		// tamanio (total) con el cual quedara el bloque q pide el usuario
		size_t tamanioUserAlineado = redondear(size + SIZE_HEADER);

		// tamanio con el cual quedara el bloque que se divide
		size_t particionadoTamanio = tamanioFirstFit - tamanioUserAlineado;

		// se fija si al dividir los bloques va a quedar espacio para almacenar el struct 

		if ( particionadoTamanio > SIZE_STRUCT){

			colocarTamanio(firstfit, particionadoTamanio);
			bloque* nuevoBloque = (bloque*) ((char*)firstfit + SIZE_HEADER + particionadoTamanio);
			
			nuevoBloquer->header = 0;
			colocarReservado(nuevoBloque, 1);
			colocarTamanio(nuevoBloque, tamanioUserAlineado - SIZE_HEADER);
			
			if ( bloqueFinal == firstfit){
				bloqueFinal = nuevoBloque;
			}

			void* direccionRetorno = (void*) ((char*)nuevoBloque + SIZE_HEADER);
			bloqueFinal = nuevoBloque;
			
			return direccionRetorno;
     	}
		 else{
			colocarReservado(firstfit, 1);
			quitarLibre(firstfit);
			
			void* direccionRetorno = (void*) ((char*)firstfit+SIZE_HEADER);
			return direccionRetorno;
		}
	}
	return NULL;
}

static void quitarLibre(bloque* bloqueLibre){
	if(bloqueLibre == libreInicial){
		libreInicial = NULL;
	}
	else{
		bloque* anterior = bloqueLibre-> anterior;
		anterior->siguiente = bloqueLibre->siguiente;
	}	
	
	bloqueLibre->anterior = NULL;
	bloqueLibre->siguiente = NULL;
}

/*
 * mm_free - Freeing a block does nothing.
*/
void mm_free(void *ptr) {

	
 	if (ptr){
    	bloque* bloqueLiberar = (bloque*) ((char*)ptr - SIZE_HEADER);
		insertarLibre(bloqueLiberar);
	}
	else{
		exit(1);
	}
}


static void mm_checkheap(int line){

	if( (void*)((char*)inicioHeap) != mem_heap_lo()){
		printf("Inicio de heap incorrecto.\n");
		exit(1);
	}

	if(bloqueFinal != NULL){
		size_t tamanio = devolverTamanio(bloqueFinal);
		if( (void*)((char*)bloqueFinal+7+tamanio) != mem_heap_hi() ){
			printf("Final de heap incorrecto. \n");
			exit(1);
		}	
		size_t tamanioHeader = (size_t) (((char*)bloqueFinal+8+tamanio) - (char*)inicioHeap);
		if(tamanioHeader != mem_heapsize() ){
			printf("Tamanio de heap incorrecto. \n");
			exit(1);
		}
	}

	bloque* primerLibre = libreInicial;
	while ( primerLibre != NULL){
		int reservado = chequearEstado(libreInicial);
		if (reservado == 1){
			printf("Error en la lista de libres. \n");
			exit(1);
		}
		primerLibre = primerLibre->siguiente;
	}

/*
	intptr_t dirInicio = (intptr_t) inicioHeap;


	bool esMultiplo = true;
	while(esMultiplo == true){
		
		if(dirInicio % 16 != 0){
			esMultiplo = false;
			break;
		}


		size_t tamanio = devolverTamanio((bloque*)dirInicio);
		printf("%ld \n", tamanio+8);

		bloque* bloqueHeader = (bloque*) dirInicio;

		if( bloqueHeader == bloqueFinal){
			break;
		}

		dirInicio = dirInicio + tamanio + 8;

	}
	if(esMultiplo == false){
		printf("Hay un puntero que no esta alineado a 16 \n");
		exit(1);
	}
*/

}
static void insertarLibre(bloque* bloqueLibre){
	if ( libreInicial == NULL){
		libreInicial = bloqueLibre;
		bloqueLibre->anterior = NULL;
	}else{
		bloque* ultimo = devolverUltimo();
		ultimo->siguiente = bloqueLibre;
		bloqueLibre->anterior = ultimo;

	}

	colocarReservado(bloqueLibre, 0);
	bloqueLibre->siguiente = NULL;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/
void *mm_realloc(void *ptr, size_t size) {

	if(ptr == NULL){
		return mm_malloc(size);
	}
	
	size_t tamanioViejo = devolverTamanio((bloque*) ((char*)ptr - SIZE_HEADER));
	
	void* nuevoBloque = mm_malloc(size);

	memcpy(nuevoBloque , ptr, tamanioViejo );

	mm_free(((char*)ptr - SIZE_HEADER));

	return (void*) nuevoBloque;
	
return NULL;
}

