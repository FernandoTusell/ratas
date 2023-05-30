/* Todas estas dimensiones requieren verificación y, en
   su caso, adaptación a lo que finalmente envíen       */

#include <stdio.h>            // Input/Output 
#include <stdint.h>           // Para poder usar enteros de longitud fija
#include <queue>              // Hace uso STL; compilar con g++

#define XP     2400           // Pixels dimensión X
#define YP     3504           // Pixels dimensión Y
#define ZP       10           // Número de fotos apiladas

#define NOBJ   8000           // Número máximo de objetos; puede
                              // hacerse mayor.

using namespace std;          // Uso Standard Libraty de C++

const float DX=2.143 ;        // Dimensión X celda en nm
const float DY=2.143 ;        // Dimensión Y celda en nm
const float DZ=7.0 ;          // Dimensión Z celda en nm

// Para cada celda definimos una estructura,
// conteniendo la información que necesitamos.

struct celda {  
  bool  llena = false ;       // Celda con un "1" ("rojo")
  bool  visitada = false  ;   // Celda ya visitada
  bool  agotada = false ;     
};

// 'celdcoord' es una celda suplementada con las coordenadas; para
// guardar una celda en cola e iniciar posteriormente búsquedas a
// partir de ella, necesitamos la información de sus coordenadas.

struct celdcoord {
  celda infocelda ;
  int i ;
  int j ;
  int k ; 
};

// Cada objeto se describe por una etiqueta. Conservamos el número
// total de celdas, si se trata de un fragmento u objeto completo,
// y la celda desde la que arrancó su construcción. Esto último
// permite reconstruirlo si hace falta, e identificar píxeles sueltos
// (objeto de tamaño 1)

struct objeto {
  unsigned int etiqueta ;     // Número de etiqueta
  unsigned int nceldas = 0 ;  // Número de celdas con la etiqueta
  bool fragmento = false ;    // false (contenido) o true  (fragmento)
  celdcoord origen ;          // Celda "semilla", a partir de la cual se
                              // ha identificado el objeto
};

struct objeto hallados[NOBJ] ;


// 'cubo' es un array 3D de celdas como la definida 

struct celda cubo[XP][YP][ZP] ;

int main() {

  int i, j, k, i0, j0, k0, i1, j1, k1, cont=0, nobj=0 ;
  uint16_t uv[3] ;
  FILE * fp ;
  queue<celdcoord> pendientes ;
  celdcoord actual, vecina ;

  //  ==================  Lectura fichero externo de datos   ==================
  fp  = fopen("/home/etptupaf/ft/proyectos/ratbrains/datos/datos1.bin", "rb") ;
  printf("Leyendo fichero y construyendo el cubo\n") ;	
  while( fread(uv, sizeof(uv), 1, fp) ) {
    i = uv[0] -1 ; j = uv[1] - 1 ; k = uv[2] - 1 ;
    cubo[i][j][k].llena = true ;
    cont++ ;
  }
  printf("%d celdas llenas insertadas en el cubo.\n", cont) ;
  fclose(fp) ;
  // ===================  Fin lectura datos externos   =======================
  // 
  // =========  Generación de datos simulados de dimensión XP x YP x ZP ========
  // Definimos un cubo poniendo algunos objetos. Sin pérdida de generalidad, son
  // esferas o elipses, sin tener en cuenta por simplicidad las diferentes dimensiones
  // en cada dirección. En 'celdas' acumulamos el número de celdas "llenas", para
  // comparar con el resultado del recuento.

  // printf("Generando cubo %d x %d x %d de test.\n", XP, YP, ZP) ;
  // for (i=0; i<XP; i++) {
  //  for (j=0; j<YP; j++) {
  //    for (k=0; k<ZP; k++) {
  //      	// Una esfera de centro (101,31,6) y radio 3	
  //      if ( (i-100)*(i-100) + (j-30)*(j-30) + (k-5)*(k-5) <= 16 ) {
  // 	  cubo[i][j][k].llena = true ;
  // 	  cont++ ;
  // 	}
  // 	// Una esfera de centro (501,1001,5) y radio 3;
  // 	if ( ((i-500)*(i-500) + (j-1000)*(j-1000) + (k-4)*(k-4)) <= 9 ) {
  // 	  cubo[i][j][k].llena = true ;
  // 	  cont++ ;
  // 	}
  // 	// Una elipse tocando el contorno, para obtener un objeto de tipo 'F' (fragmento)
  // 	if ( (3*(i-2395)*(i-22395) + 2*(j-2000)*(j-2000) + 1*(k-2)*(k-2)) <= 144 ) {
  // 	  cubo[i][j][k].llena = true ;
  // 	  cont++ ;
  // 	}
  //    }
  //  }
  // }
  // printf("%d celdas llenas insertadas en el cubo.\n", cont) ;
  // ========== Fin generación de datos simulados  ===============
  
  for (i=0; i<XP; i++) {
    for (j=0; j<YP; j++) {
      for (k=0; k<ZP; k++) {
	if ( !cubo[i][j][k].llena |                // Si celda vacía o ya agotada
	     (cubo[i][j][k].agotada) )             // pasamos de largo
	  continue ;
	                                           // En caso contrario, inicializamos	                                            
	nobj++ ;                                   // un nuevo objeto
	hallados[nobj-1].etiqueta = nobj ;
	hallados[nobj-1].nceldas = 0 ;

	actual.infocelda  =  cubo[i][j][k] ;       // Guardamos la información de
	actual.i = i ;                             // la celda actual para introducirla en la 
	actual.j = j ;                             // cola del proceso
	actual.k = k ;

	if (i==0 | j==0 | k==0 |                   // Si alguna de las coordenadas toca los límites 
	    i==(XP-1) | j==(YP-1) | k==(ZP-1))     // el nuevo objeto es un fragmento 
	  hallados[nobj-1].fragmento = true ;

	hallados[nobj-1].origen = actual ;         // El origen o "semilla" de este nuevo objeto es
	                                           // la celda que tenemos entre manos ('actual').
	pendientes.push(actual) ;                  // Introducimos la celda en cola; ya no abandonaremos
	                                           // el 'while' a continuación hasta que el objeto al
	                                           // que pertenece haya sido totalmente explorado
	
	while( pendientes.size() > 0 ) {           // Mientras la cola tenga celdas a examinar,	    
	  actual = pendientes.front() ;            // las vamos procesando, comenzando por las
	  pendientes.pop() ;                       // más antiguas (cabeza de la cola, front).
	  
	                                           // Declaramos 'actual'agotada: todo lo alcanzable
                                                   // desde ella va a ser examinado a continuación
	  cubo[actual.i][actual.j][actual.k].agotada = true ;
	  cubo[actual.i][actual.j][actual.k].visitada = true ;   
	  hallados[nobj-1].nceldas++ ;             // Contamos las celdas al agotarlas
	  for (i1=-1; i1<=1; i1++) {               // Este triple bucle anidado obtiene todas las
	    for (j1=-1; j1<=1; j1++) {             // vecinas de 'actual'.
	      for (k1=-1; k1<=1; k1++) {
		if ( i1==0 & j1==0 & k1==0 )       // La celda actual no es una vec ina
		  continue ;
		i0 = actual.i + i1 ;               // Coordenadas de una vecina; cuando i1=j1=k1=0,
		j0 = actual.j + j1 ;               // se cuenta tambień la celda actual.
		k0 = actual.k + k1 ;               // 
		if (i0<0 | j0<0 | k0<0 |           // Si alguna de las vecinas "se sale" de la 
		    i0==XP | j0==YP | k0==ZP)      // imagen, pasamos de largo.
		  continue ;
		if ((!hallados[nobj-1].fragmento) &
		    (i0==0 | j0==0 | k0==0 |       // Si alguna de las coordenadas toca los límites 
		      i0==(XP-1) |                 // de la imagen y celda llena, es un fragmento;
		      j0==(YP-1) | k0==(ZP-1)) &   // tomar nota si previamente no constaba
		     cubo[i0][j0][k0].llena )
		  {
		  hallados[nobj-1].fragmento = true ;
	       	}	                                   
		if ( cubo[i0][j0][k0].llena &      // Si vecina dentro de la región, no visitada
		     !cubo[i0][j0][k0].visitada )  // y llena la recuperamos.
		  {
		    vecina.infocelda  =  cubo[i0][j0][k0] ;
		    vecina.i = i0 ;
		    vecina.j = j0 ;
		    vecina.k = k0 ;
		    cubo[i0][j0][k0].visitada=true ; // La marcamos como ya visitada...
		    pendientes.push(vecina) ;        //   ...y la incluimos en la cola de
		                                     // proceso para uso posterior.
		  }
	      }	    
	    }	    
	  }  
	}
      }
    }
  }

  // Resumen objetos hallados
  
  cont = 0 ;
  printf("                                Volumen              Origen\n") ;
  printf("   Obj     # celdas               nm^3      Tipo    (X, Y, Z=foto)\n") ;
  for (i=0; i<nobj ; i++) {
    printf("  %4d      %7d      %16.1f     %s     (%d,%d,%d)\n",
	   hallados[i].etiqueta,
	   hallados[i].nceldas,
	   hallados[i].nceldas*DX*DY*DZ,
	   hallados[i].fragmento?"F":"C",
	   hallados[i].origen.i+1,
	   hallados[i].origen.j+1,
	   hallados[i].origen.k+1) ;
    cont = cont + hallados[i].nceldas ;
  }
  printf("Total = %d celdas encontradas en objetos\n", cont ) ;
  
  return(0) ; 
} ;
