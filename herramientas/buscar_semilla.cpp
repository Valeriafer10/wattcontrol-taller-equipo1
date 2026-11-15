// Herramienta auxiliar: busca semillas de rand() con las que WattControl muestra,
// en la primera simulacion, UNA falla de lectura (continue) y un precio mayor a 1500
// en la hora pico (para probar el break con embalses < 40 %).
// Sirve porque rand() da numeros distintos en cada computador/compilador.
//
// Compilar y correr:  g++ buscar_semilla.cpp -o buscar_semilla && ./buscar_semilla
// Luego se cambia  #define SEMILLA  en wattcontrol.cpp por una de las semillas de la lista.
#include <iostream>
#include <cstdlib>
using namespace std;

// Misma logica de Mercado::precioHora de wattcontrol.cpp
int precioHora(int hora) {
    int minimo, maximo;
    if (hora <= 6)       { minimo = 398; maximo = 640; }
    else if (hora <= 14) { minimo = 456; maximo = 970; }
    else if (hora < 18)  { minimo = 596; maximo = 1040; }
    else if (hora <= 20) { minimo = 705; maximo = 1612; }
    else                 { minimo = 565; maximo = 970; }
    return minimo + rand() % (maximo - minimo + 1);
}

int main() {
    int encontradas = 0;
    for (int semilla = 1; semilla <= 500 && encontradas < 10; semilla++) {
        srand(semilla);
        int fallas = 0, horaFalla = -1, horaPrecioAlto = -1;
        for (int h = 0; h < 24; h++) {
            int precio = precioHora(h);
            if (precio > 1500 && horaPrecioAlto == -1) horaPrecioAlto = h;
            if (rand() % 20 == 0) {          // falla de lectura
                if (fallas == 0) horaFalla = h;
                fallas++;
            }
        }
        // Una sola falla a mitad del dia y un precio > 1500 en hora pico
        if (fallas == 1 && horaFalla >= 6 && horaFalla <= 16 && horaPrecioAlto >= 18) {
            cout << "SEMILLA " << semilla << ": falla en la hora " << horaFalla
                 << ", precio > 1500 en la hora " << horaPrecioAlto << endl;
            encontradas++;
        }
    }
    return 0;
}
