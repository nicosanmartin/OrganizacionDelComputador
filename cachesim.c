#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef struct{
	int setIndex;  // decimal rango 0-S
	int tagIndex; // debe estar en hexadecimal
	int cacheLine; // numero de linea
	int lineTag;  // tag presente anteriormente en la linea
	int validBit;
	int dirtyBit;
	int lastUsed;
} lineaCache;


typedef struct{
	lineaCache *linea;
} setCache;


typedef struct{
	int tag;
	int set;
	int tamanioBloque;
} datosDireccion;


typedef struct{
	int lecturas;
        int escrituras; // loads
        int totalAccesos; // stores
        int missesLectura; // rmiss
        int missesEscritura; // wmiss
        int totalMisses; // rmiss + wmiss
	int dirtyReadMiss; // dirty read misses
	int dirtyWriteMiss; // dirty write misses
        int bytesLeidos;
        int bytesEscritos;
    	int tiempoLecturas;
        int tiempoEscrituras;
        int missRateTotal;
	int totalAsoc;
	int totalSets;
	setCache *set;
} cache;


/// PROTOTIPOS ///

datosDireccion* cargarDireccion(int direccion, int asociatividad, int sets, int cacheTamanio);
cache* crearCache(int asociatividad, int set);
int leerTraza(cache *cacheCreada, char* archivoTrazas, int modoVerboso, int asociatividad, int sets, int cacheTamanio, int n, int m); // modoverboso = 1 (activado), 0 (desactivado)
void borrarCache(cache *cacheCreada, int set);
int validar (int n);
void cargarCache(cache* cacheCreada, datosDireccion* direccion, int numOperacion, char* rw, int modoVerboso, int n, int m);
void inicializarCache (cache* cacheCreada, int sets, int asoc);
void imprimirCache(cache* cacheCreada, int tamanioCache);

/////////////////


int validar (int n) {

    while ((n % 2 == 0) && n > 1) {
        	n = n/2;
	}	
	return (n == 1);
}


void inicializarCache (cache* cacheCreada, int asoc, int sets) {
	
	int i, j;	
	cacheCreada->lecturas = 0;
	cacheCreada->escrituras = 0;	
	cacheCreada->totalAccesos = 0;
	cacheCreada->missesLectura = 0;
	cacheCreada->missesEscritura = 0;
	cacheCreada->totalMisses = 0;
	cacheCreada->dirtyReadMiss = 0;
	cacheCreada->dirtyWriteMiss = 0;
	cacheCreada->bytesLeidos = 0;
	cacheCreada->bytesEscritos = 0;
	cacheCreada->tiempoLecturas = 0;
	cacheCreada->tiempoEscrituras = 0;	
	cacheCreada->missRateTotal = 0;

	for (i = 0; i < sets; i++) {
		for (j = 0; j < asoc; j++) {
			cacheCreada->set[i].linea[j].setIndex = -1;
			cacheCreada->set[i].linea[j].tagIndex = -1;
			cacheCreada->set[i].linea[j].cacheLine = 0;
			cacheCreada->set[i].linea[j].lineTag = -1;
			cacheCreada->set[i].linea[j].validBit = 0;
			cacheCreada->set[i].linea[j].dirtyBit = 0;
			cacheCreada->set[i].linea[j].lastUsed = 0;
		}
	}
}


cache* crearCache(int asociatividad, int sets) {
	
	// Los set actuan como filas, las columnas son 9*asociatividad
	cache* cacheCreada = (cache*) malloc(sizeof(cache));
	cacheCreada->totalSets = sets;
	cacheCreada->totalAsoc = asociatividad;
	cacheCreada->set = (setCache*) malloc(sets*sizeof(setCache));
	
	int i;
	for (i = 0; i < sets; i++) {
		cacheCreada->set[i].linea = (lineaCache*) malloc(asociatividad * sizeof(lineaCache));
	}
	return cacheCreada;
}


void cargarCache(cache* cacheCreada, datosDireccion* direccion, int numOperacion, char* rw, int modoVerboso, int n, int m) {

	int i, validBit, lastUsed, cacheLine, dirtyBit, lineTag = 0;

	char* idCaso = "0";

	int tag = direccion->tag;
	int set = direccion->set;
	int asociatividad = cacheCreada->totalAsoc;

	if (strcmp(rw, "R") == 0) {
		cacheCreada->lecturas++;
		for (i = 0; i < asociatividad; i++) {
			if (cacheCreada->set[set].linea[i].tagIndex == tag) {
				idCaso = "1";
				lineTag = cacheCreada->set[set].linea[i].tagIndex;
				lastUsed = cacheCreada->set[set].linea[i].lastUsed;
				dirtyBit = cacheCreada->set[set].linea[i].dirtyBit;
				validBit = 1;
				
				cacheCreada->set[set].linea[i].lastUsed = numOperacion;
				cacheLine = i;
				cacheCreada->tiempoLecturas++;
				break;
			}
		}
		
		if (strcmp(idCaso, "0") == 0) {
			cacheCreada->missesLectura++;
			cacheCreada->bytesLeidos += direccion->tamanioBloque;
			for (i = 0; i < asociatividad; i++) {
				if (cacheCreada->set[set].linea[i].validBit == 0) {

					validBit = cacheCreada->set[set].linea[i].validBit;
					lineTag = cacheCreada->set[set].linea[i].tagIndex;
					dirtyBit = cacheCreada->set[set].linea[i].dirtyBit;
					lastUsed = cacheCreada->set[set].linea[i].lastUsed;
					cacheLine = i;
					idCaso = "2a"; 
					cacheCreada->tiempoLecturas += 1 + 100;
					cacheCreada->set[set].linea[i].validBit = 1;					
					cacheCreada->set[set].linea[i].tagIndex = tag;
					cacheCreada->set[set].linea[i].setIndex = set;
					cacheCreada->set[set].linea[i].lastUsed = numOperacion;
					break;
				}
			}
		}
		if (strcmp(idCaso, "0") == 0) {
			int nroLinea = 0;	
			int comparador = cacheCreada->set[set].linea[0].lastUsed;

			for (i = 0; i < asociatividad; i++) {
				if (cacheCreada->set[set].linea[i].lastUsed < comparador) {
					comparador = cacheCreada->set[set].linea[i].lastUsed;					
					nroLinea = i;
				}
			}

			cacheLine = nroLinea;
			dirtyBit = cacheCreada->set[set].linea[nroLinea].dirtyBit;
			lineTag = cacheCreada->set[set].linea[nroLinea].tagIndex;
			lastUsed = cacheCreada->set[set].linea[nroLinea].lastUsed;
			validBit = cacheCreada->set[set].linea[nroLinea].validBit;

			cacheCreada->set[set].linea[nroLinea].validBit = 1;
			cacheCreada->set[set].linea[nroLinea].tagIndex = tag;
			cacheCreada->set[set].linea[nroLinea].setIndex = set;
			cacheCreada->set[set].linea[nroLinea].lastUsed = numOperacion;
					
			if (dirtyBit == 1) {
				idCaso = "2b"; 
				cacheCreada->dirtyReadMiss++;
				cacheCreada->bytesEscritos += direccion->tamanioBloque;
				cacheCreada->set[set].linea[nroLinea].dirtyBit = 0;
				cacheCreada->tiempoLecturas += 201;
			} else {
				idCaso = "2a"; 
				cacheCreada->tiempoLecturas += 101;
			}
		}
	} else { //rw == 'W'
		cacheCreada->escrituras = cacheCreada->escrituras + 1;

		for (i = 0; i < asociatividad; i++) {
			if (cacheCreada->set[set].linea[i].tagIndex == tag) {
				idCaso = "1";
				lineTag = cacheCreada->set[set].linea[i].tagIndex;
				lastUsed = cacheCreada->set[set].linea[i].lastUsed;
				dirtyBit = cacheCreada->set[set].linea[i].dirtyBit;
				cacheLine = i;
				validBit = 1;
				
				cacheCreada->tiempoEscrituras++;
				cacheCreada->set[set].linea[i].lastUsed = numOperacion;
				cacheCreada->set[set].linea[i].dirtyBit = 1;
				cacheCreada->set[set].linea[i].validBit = 1;

				break;
			}
		}
		if (strcmp(idCaso, "0") == 0) {
			cacheCreada->missesEscritura++;
			cacheCreada->bytesLeidos += direccion->tamanioBloque;
			for (i = 0; i < asociatividad; i++) {
				if (cacheCreada->set[set].linea[i].validBit == 0) {

					dirtyBit = cacheCreada->set[set].linea[i].dirtyBit;
					lineTag = cacheCreada->set[set].linea[i].tagIndex;
					lastUsed = cacheCreada->set[set].linea[i].lastUsed;
					validBit = cacheCreada->set[set].linea[i].validBit;
					idCaso = "2a"; 
					cacheLine = i;

					cacheCreada->tiempoEscrituras += 1 + 100;
					cacheCreada->set[set].linea[i].validBit = 1;
					cacheCreada->set[set].linea[i].dirtyBit = 1;
					cacheCreada->set[set].linea[i].tagIndex = tag;
					cacheCreada->set[set].linea[i].setIndex = set;
					cacheCreada->set[set].linea[i].lastUsed = numOperacion;

					break;
				}
			}
		}
		if (strcmp(idCaso, "0") == 0) {
			int nroLinea = 0;	
			int comparador = cacheCreada->set[set].linea[0].lastUsed;

			for (i = 0; i < asociatividad; i++) {
				if (cacheCreada->set[set].linea[i].lastUsed < comparador) {
					comparador = cacheCreada->set[set].linea[i].lastUsed;					
					nroLinea = i;
				}
			}

			cacheLine = nroLinea;
			dirtyBit = cacheCreada->set[set].linea[nroLinea].dirtyBit;
			lineTag = cacheCreada->set[set].linea[nroLinea].tagIndex;
			lastUsed = cacheCreada->set[set].linea[nroLinea].lastUsed;
			validBit = cacheCreada->set[set].linea[nroLinea].validBit;

			cacheCreada->set[set].linea[nroLinea].validBit = 1;
			cacheCreada->set[set].linea[nroLinea].tagIndex = tag;
			cacheCreada->set[set].linea[nroLinea].setIndex = set;
			cacheCreada->set[set].linea[nroLinea].lastUsed = numOperacion;
			cacheCreada->set[set].linea[nroLinea].dirtyBit = 1;

			if (dirtyBit == 1) {
				idCaso = "2b"; 
				cacheCreada->bytesEscritos += direccion->tamanioBloque;
				cacheCreada->dirtyWriteMiss++;
				cacheCreada->tiempoEscrituras +=  201;
			} else {
				idCaso = "2a"; 
				cacheCreada->tiempoEscrituras += 101;
			}
		}		
	}

	if (modoVerboso == 1) {
		if (numOperacion >= n && numOperacion <= m) {
			if (asociatividad > 1) {
				if (lineTag == -1){
				printf("%d %s %x %x %d -1 %d %d %d\n", numOperacion, idCaso, set, tag, cacheLine, validBit, dirtyBit, lastUsed);
				}
				else{
				printf("%d %s %x %x %d %x %d %d %d\n", numOperacion, idCaso, set, tag, cacheLine, lineTag, validBit, dirtyBit, lastUsed);	
				}
			} else {
				if (lineTag == -1 || lineTag == 0){
					printf("%d %s %x %x %d -1 %d %d\n", numOperacion, idCaso, set, tag, cacheLine, validBit, dirtyBit);
				}
				else {
					printf("%d %s %x %x %d %x %d %d\n", numOperacion, idCaso, set, tag, cacheLine, lineTag, validBit, dirtyBit);	
				}
			}
		}
	}
}


int leerTraza(cache* cacheCreada, char* archivoTrazas, int modoVerboso, int asociatividad, int sets, int cacheTamanio, int n, int m) {

	int direccion, bytes, datos;
	char instruccion[11];
	char operacion[2];
	

	FILE* traza;
	traza = fopen(archivoTrazas, "r");
	int numOperac = 0;

	if (traza == NULL) {
		return -1;
	} else {
		// Toma la direccion a la que hay que entrar

		while (fscanf(traza, "%s %s %x %d %x", instruccion, operacion, &direccion, &bytes, &datos) != EOF && strcmp(instruccion, "#eof") != 0) {
			datosDireccion* direccionCargada = cargarDireccion(direccion, asociatividad, sets, cacheTamanio);
			cargarCache(cacheCreada, direccionCargada, numOperac, operacion, modoVerboso, n, m);
			numOperac++; 
		}
	}

	fclose(traza);
	return 0;
}


int loga(int x) {

	int i = 0;
	for (i = 0; i < 31; i++) {
		if((x >> i) == 1) {
			break;
		}
	}
	return i;
}


datosDireccion* cargarDireccion(int direccion, int asociatividad, int sets, int cacheTamanio) {
	
	int setBits = loga(sets);
	int blockBytes = cacheTamanio / (sets * asociatividad); // B
	int blockBits = loga(blockBytes);

	int tagBits = 32 - blockBits - setBits;

	int tag = direccion >> (setBits + blockBits);
  
	tag = ~((-1U) << tagBits) & tag; // bits del tag de la direccion

	int set = direccion >> blockBits;
	set = ~((-1U) << setBits) & set; // bits del set de la direccion

	datosDireccion* direccionCargada = (datosDireccion*)malloc(sizeof(datosDireccion*));
	direccionCargada->tag = tag;
	direccionCargada->set = set;
	direccionCargada->tamanioBloque = blockBytes;
	
	return direccionCargada;
}


void borrarCache(cache *cacheCreada, int sets){
	// se libera la memoria reservada para cada columba/fila
	
	int i;
	for(i = 0; i < sets; i ++){
		free(cacheCreada->set[i].linea);
	}
	
	free(cacheCreada->set);
    free(cacheCreada);
}



int main (int argc, char *argv[]) {
	int cacheTamanio, asoc, set, n, m, aux1, aux2, aux3, aux4, modoVerboso;
	n = 0;
	m = 0;
	modoVerboso = 0;

	////// CONDICIONES DE ERROR /////

	if (argc != 5 && argc != 8) {
   		printf("Error en la cantidad de argumentos\n");
    	return -1;	
	}
	if (argv[5]) {
		n = atoi(argv[6]);
		m = atoi(argv[7]);
		modoVerboso = 1;
		if (n < 0 || n > m) {
			printf("Valores n y m incorrectos\n");
			return -1;
		}
	} 
	
	//////////////

	cacheTamanio = atoi(argv[2]);
	asoc = atoi(argv[3]);
	set = atoi(argv[4]);

	aux1 = validar(cacheTamanio);
	aux2 = validar(asoc);
	aux3 = validar(set); 
	
	
	int blockBytes = cacheTamanio / (set * asoc); // B
	aux4 = validar(blockBytes);
	
	if (aux1 != 1 || aux2 != 1 || aux3 != 1){
		fprintf(stderr, "Alguno de los parametros (cache, asoc o set) no es potencia de dos.\n");
		return -1;
	}	
	if (aux4 != 1){
		fprintf(stderr, "Los bytes del bloque no son validos.\n");
		return -1;
	}

	cache* cacheCreada = crearCache(asoc, set);
	inicializarCache(cacheCreada, asoc, set);
	
	int chequearArchivo = leerTraza(cacheCreada, argv[1], modoVerboso, asoc, set, cacheTamanio, n, m);

	if (chequearArchivo == -1){
		fprintf(stderr, "No se pudo abrir el archivo.\n");
		return -1;
	}

	imprimirCache(cacheCreada, cacheTamanio);

	borrarCache(cacheCreada, set);
	return 0;
}

void imprimirCache(cache* cacheCreada, int tamanioCache){

	
	cacheCreada->totalAccesos = cacheCreada->lecturas + cacheCreada->escrituras;
	cacheCreada->totalMisses = cacheCreada->missesLectura + cacheCreada->missesEscritura;

	int size = tamanioCache/1024;

	if(cacheCreada->totalAsoc > 1){
		printf("%d-way, %d sets, size = %dKB\n", cacheCreada->totalAsoc, cacheCreada->totalSets, size);
	}
	else{
		printf("direct-mapped, %d sets, size = %dKB\n", cacheCreada->totalSets, size);
	}
	printf("loads %d stores %d total %d\n", cacheCreada->lecturas, cacheCreada->escrituras, cacheCreada->totalAccesos);
	printf("rmiss %d wmiss %d total %d\n", cacheCreada->missesLectura, cacheCreada->missesEscritura, cacheCreada->totalMisses);
	printf("dirty rmiss %d dirty wmiss %d\n", cacheCreada->dirtyReadMiss, cacheCreada->dirtyWriteMiss);
	printf("bytes read %d bytes written %d\n", cacheCreada->bytesLeidos, cacheCreada->bytesEscritos);
	float missrate = (float)cacheCreada->totalMisses / (float)cacheCreada->totalAccesos;
	printf("read time %d write time %d\n", cacheCreada->tiempoLecturas, cacheCreada->tiempoEscrituras);
	printf("miss rate %f\n", missrate);
}
