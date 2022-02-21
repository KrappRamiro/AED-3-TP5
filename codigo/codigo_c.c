/*
 *  RECORDATORIO:
 *  Tabla de Verdad - relacion entre MUX y Frecuencia
 *  MUX_B  MUX_A // Frecuencia
 *  0      0        100 Hz a 1,920 MHz
 *  0      1        100 Hz a 480 KHz
 *  1      0        100 Hz a 120 KHz
 *  1      1        100 Hz a 30 KHz
 *
 * NOTA: el limite inferior de 100 Hz no es tan asi, puede llegar en teoria al divisor como limite inferior.
 * Es decir, para divisor de 64, frecuencia minima de 64 Hz o para divisor de 4, frecuencia minima de 4 Hz
 * Pero es teorico, en la practica puede variar un poco. Por eso lo mejor es minimo 100 Hz.
 */



#include <xc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ConfigurationBitsC.h"

#define _XTAL_FREQ          8000000
#define BCD_A               RA0
#define BCD_B               RA1
#define BCD_C               RA2
#define BCD_D               RA3
#define DEMUX_A             RA4
#define DEMUX_B             RB1
#define KHz                 RB2
#define MUX_A               RB3
#define MUX_B               RB4
#define MHz                 RB5
#define Displays            4
#define WindowSize          3
#define SampleTimeInMillis  166

//----------------------------
// Definición de variables:
typedef __uint24 natural; // Numeros enteros sin signo (16 bits).
typedef float real; // Numeros con decimal (24 bits).

//----------------------------
// Variables Globales:
natural flankChanges = 0;       // Cantidas de pulsos o flancos detectados por RB0.
natural window[WindowSize];     // Muestras para el calculo de la frecuencia.
int timerTicks = 0;             // Auxiliar para el traspaso a Hz.
int sampleTicks  = 0;           // Auxiliar para el tamaño de window (WindowSize).
bool flag_1mS = false;          // Booleano para detectar el paso de un 1 ms.
int auxiliar = 0;

//----------------------------
// Funcion de Interrupción:
void __interrupt() ISR(void) {
   // Clasificación de interrupciones:
	if (T0IF == true) { // Pregunto si se activo la interrupcion de 1ms
		flag_1mS = true;
		++timerTicks;
		//--------------
		TMR0 = 131; // Contador del Timer cero (0).
		T0IF = 0;
    }
    if ((INTF == true) && ((auxiliar % 2) == 0)) {
		//Esto de aca solamente se ejecuta cuando el valor de auxiliar es par, esto es
		//para tener una ventana para calcular la frecuencia y otra para mostrar en display.
		//Tambien necesita que se haya activado la interrupcion por cambio de flanco
        ++flankChanges;
      //--------------
		INTF = false;
    }
    else {
		INTF = false;
		INTE = false;
    }


	if (timerTicks == SampleTimeInMillis) {	// pregunto si la cantidad de interrupciones
		//del timer 0 es igual a 166, esto es para dividir en 6 ventanas de tiempo a lo
		//largo de 1 segundo
		if ((auxiliar % 2) == 0) {
			//Esto de aca solamente se ejecuta cuando el valor de auxiliar es par, esto es
			//para tener una ventana para calcular la frecuencia y otra para mostrar en display.
			if (WindowSize <= sampleTicks) {
				// esto de aca hace que el valor de sample ticks loopee desde 0 a 5, y no se vaya
				// a valores superiores a las ventanas de tiempo que tenemos en 1ms
				sampleTicks = 0;
			}
			window[sampleTicks] = flankChanges; //esto de aca es fundamental, guarda
			//las mediciones hasta el momento en una posicion del array window.
			// Si se suman todos los valores de window, se obtienen la cantidad de 
			// cambios de flancos
			flankChanges = 0;
			++sampleTicks;
			timerTicks = 0;
		}
		++auxiliar;
		if (auxiliar == 5) {
			auxiliar = 0;
		}
    }
    if ((auxiliar % 2) == 0) {
		INTE = true;
	}
}

void configuration(void) {
    KHz = false;
    MHz = false;
   //--------------
    TRISB = 0x19; //0001-1001 // el truco para entender hexa es saber que
	// 1 -> 0001
	// 9 -> 1001
	// si al hexa le paso 0x19, es lo mismo que pasar 0b 0001 1001 como haciamos en binario
    PORTB = 0x00;
    TRISA = 0x00;
    PORTA = 0x00;
    //nRBPU = 0;
    //--------------
    GIE = 1;
    //--------------
	T0IE = 1; // Habilito la interrupcion por Timer cero (0).
    T0CS = 0; // Aumenta el Timer por cada ciclo de intruccion.
    //T0IF = 1; // Flag que se activa cuando se desborda el contador del Timer cero (0).    
    TMR0 = 131; // Contador del Timer cero (0).
    PSA = 0; // Asigno la preescala al Timer cero (0).
    PS0 = 1; // Preescala = 16 (veces).
    PS1 = 1;
    PS2 = 0;
    //--------------
    INTE = 1; // Habilito el detector de flancos.
    INTEDG = 1; // Habilito el flanco ascendente.
    //INTF = 1; // Flag que se activa cuando ocurre una interrupcion por RB0.
}

void showNumber(int display, int number) { 
	/*
	 * Esta funcion recibe dos argumentos, uno es a que display se quiere imprimir,
	 * otro es que numero se quiere imprimir
	 */
    switch (display) {
        case 0:
            DEMUX_A = 0;
            DEMUX_B = 0;
            break;
        case 1:
            DEMUX_A = 0;
            DEMUX_B = 1;
            break;
        case 2:
            DEMUX_A = 1;
            DEMUX_B = 0;
            break;
        case 3:
            DEMUX_A = 1;
            DEMUX_B = 1;
            break;
    }
    switch (number) {
        case 0:
            BCD_A = 0;
            BCD_B = 0;
            BCD_C = 0;
            BCD_D = 0;
            break;
        case 1:
            BCD_A = 0;
            BCD_B = 0;
            BCD_C = 0;
            BCD_D = 1;
            break;
        case 2:
            BCD_A = 0;
            BCD_B = 0;
            BCD_C = 1;
            BCD_D = 0;
            break;
        case 3:
            BCD_A = 0;
            BCD_B = 0;
            BCD_C = 1;
            BCD_D = 1;
            break;
        case 4:
            BCD_A = 0;
            BCD_B = 1;
            BCD_C = 0;
            BCD_D = 0;
            break;
        case 5:
            BCD_A = 0;
            BCD_B = 1;
            BCD_C = 0;
            BCD_D = 1;
            break;
        case 6:
            BCD_A = 0;
            BCD_B = 1;
            BCD_C = 1;
            BCD_D = 0;
            break;
        case 7:
            BCD_A = 0;
            BCD_B = 1;
            BCD_C = 1;
            BCD_D = 1;
            break;
        case 8:
            BCD_A = 1;
            BCD_B = 0;
            BCD_C = 0;
            BCD_D = 0;
            break;
        case 9:
            BCD_A = 1;
            BCD_B = 0;
            BCD_C = 0;
            BCD_D = 1;
            break;
    }
}

int digitAt(int position, natural value) {
	/*
	 * Esta funcion hace dos cosas:
	 * con % 10 saca el resto,
	 * y con el (value / 10,100, o 1000) hace que se puedan imprimir valores
	 * con el formato 1000 en el display,
	 * o sea, con case 0 pasa un 2000 a 2, y te permite imprimir ese 2 en la posicion
	 * del 1000 en el display
	 */
    switch (position) {
        case 3: return value % 10;
        case 2: return (value / 10) % 10;
        case 1: return (value / 100) % 10;
        case 0: return (value / 1000) % 10;
        default: return 0;
    }
}

real average(natural values[], int length) {
	/* esta funcion hace un promedio entre todos los valores de un array, como
	* argumentos se necesita pasar el array y la longitud del mismo
	*
	* La misma retorna el promedio
	*/
	
    natural sum = 0;
    for (int i = 0; i < length; ++i) {
        sum += values[i];
    }
    return ((real) sum) / length;
}

natural toMHzOrToKHzOrToHz(real frequency) {
	/*
	 * Esta funcion recibe como argumento una frecuencia en Hz, y
	 * calcula si debe ser retornada como MHz, KHz o Hz
	 *
	 * Tambien es la encargada de prender y apagar los LED's de MHz y KHz
	 */
    if (MHz == true) {
        KHz = false;
    }
	if ((frequency > 9999) && (frequency <= 999999)) { // si la frecuencia es mayor a lo
		// que puede mostrar el display en Hz, y no llega a ser un Mhz, prende el led de
		// KHz y retorna la frecuencia en KHz
		KHz = true;
		return (natural) frequency / 1000;
	}
    else if (frequency > 999999) {
		// Si la frecuencia es mayor a lo que puede mostrar el display en KHz, es un MHz, entonces
		// prende el led de MHz y retorna el valor en MHz
		MHz = true;
		return ((natural) frequency) / ((natural) 1000000);
    }
    else {
		// en caso de que la frecuencia no llega a ser ni un KHz, ni un MHz, apaga los leds
		// de KHz y MHz y retorna la frecuencia
		KHz = false;
		MHz = false;
		return (natural) frequency;   
	}
}

int detectDivisor() {
	/*
	 * esta funcion es fundamental para el divisor de frecuencia,
	 * detecta si se tocaron los botones MUX_A y/ o MUX_B, y en base a eso
	 * sabe internamente por cuanto se esta dividiendo la frecuencia.
	 * 
	 * Este valor de divisor se usa internamente para luego multiplicar la frecuencia 
	 * calculada por este valor, antes de pasarlo al display LCD
	 */
    int divisor;
    if ((!MUX_A) && (!MUX_B)) {
        divisor = 64;
    }
    else if ((MUX_A) && (!MUX_B)) {
        divisor = 16;
    }
    else if ((!MUX_A) && (MUX_B)) {
        divisor = 4;
    }
    else {
        divisor = 1;
    }
    return divisor;
}

//------------------------------------------------------------------------------------
// Función Principal:
int main() {
    configuration();
    //----------------------------
    // Declaracion de variables:
    real samplesAverage = 0;
    real frequency = 0;
    int position = 0;
    int digit = 0;
    int divisor = 1;
    for (int i = 0; i < WindowSize; ++i) {
        window[i] = 0;
    }

	//----------------------------
	while (true) {
	// Codigo para actualizar samplesAverage (promedio) cada 322 mS.
		if ((auxiliar % 2) == 1) {
			/*
			* Esto se ejecuta en las ventanas de tiempo en las que no se esta calculando
			* la frecuencia actual
			*/
			divisor = detectDivisor();
			samplesAverage = average(window, WindowSize);
			frequency = ((samplesAverage) * 3000) / SampleTimeInMillis; // Convierto en Hz.
		}
		//------------------------
		// Codigo para actualizar el display cada 1 mS.
		if (flag_1mS == true) {
			flag_1mS = false;
			// la siguiente linea calcula la frecuencia que hay que imprimir en el display
			natural frequencyForDisplay = toMHzOrToKHzOrToHz(frequency * divisor);
			// la siguiente calcule el valor que corresponde imprimir en el display,
			// en base a la posicion que hay que imprimir
			digit = digitAt(position, frequencyForDisplay);
			// la siguiente linea va a imprimir en el display el numero digit, en la posicion position
			showNumber(position, digit);
			// la siguiente linea va a permitir que se imprima en los 4 displays LCD, se va a ir
			// loopeando position por position, el mismo va a loopear entre 0 y 3
			++position;
			if (Displays <= position) {
				position = 0;
			}
		}
		//------------------------
	}
    return (EXIT_SUCCESS);
}
