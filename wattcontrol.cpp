// =============================================================
// WattControl v2 - Gestor de energia de un taller de mecanizado
// Fenomeno de El Nino 2026 - Programacion Orientada a Objetos (EA2)
// Autora: Valeria Fernandez Vergara (trabajo individual)
// =============================================================
// Que hace: a partir de la hora, el precio de bolsa y el nivel de los embalses,
// decide que cargas del taller se encienden, cuales se aplazan, y simula un dia
// de 24 horas para medir cuanto se ahorra frente a trabajar sin gestor.
//
// Compilar:  g++ wattcontrol.cpp -o wattcontrol
// =============================================================

// --- Librerias --------------------------------------------------------------
// iostream: cout (escribir en pantalla) y cin (leer del teclado)
#include <iostream>
// iomanip: setw (ancho de columna) y setprecision (cantidad de decimales)
#include <iomanip>
// string: el tipo string, para guardar textos
#include <string>
// cstdlib: rand() y srand(), para generar numeros al azar
#include <cstdlib>
// limits: numeric_limits, para limpiar el teclado cuando se escribe una letra
#include <limits>

using namespace std;

// --- Directivas del preprocesador (#define) ------------------------------------
// Horas de la simulacion: un dia
#define HORAS_DIA 24
// Cuantas cargas puede tener el negocio, como maximo (tamano del arreglo)
#define MAX_CARGAS 5
// Semilla de rand(): con la misma semilla se obtienen los mismos precios al azar.
// (Elegida para que en la simulacion salga una falla de lectura y se pueda probar el break)
#define SEMILLA 103

// --- Constantes del negocio: TALLER DE MECANIZADO ----------------------------------
// Todos los valores del problema en un solo lugar. Si algo cambia, se cambia solo aqui.
const string NOMBRE_NEGOCIO = "Taller de mecanizado";

// Potencias (kW) de las tres cargas
const float POTENCIA_ILUMINACION = 1.5;   // critica: iluminacion y computadores
const float POTENCIA_TORNO = 7.5;         // aplazable 1: torno CNC
const float POTENCIA_COMPRESOR = 3.0;     // aplazable 2: compresor

// Potencia maxima que el taller tiene contratada con la empresa de energia (kW)
const float POTENCIA_CONTRATADA = 10.0;
// COP/kWh: si el precio pasa de aqui, la energia esta "cara" y se aplazan las cargas
const float PRECIO_CARO = 750.0;

// Hora pico: de las 18 h hasta las 20 h (la hora 20 esta incluida)
const int PICO_INICIO = 18;
const int PICO_FIN = 20;

// Nivel de alerta de los embalses (%): por debajo hay riesgo de racionamiento
const float NIVEL_ALERTA_EMBALSES = 40.0;
// Nivel de embalses con el que empieza el programa (%): dato real de septiembre 2026 (XM)
const float NIVEL_EMBALSES_INICIAL = 78.0;

// Horas minimas de funcionamiento por dia de cada carga aplazable
const int HORAS_MIN_TORNO = 6;
const int HORAS_MIN_COMPRESOR = 4;

// Emergencia de racionamiento: precio (COP/kWh) por encima del cual, con embalses bajos, se para todo
const float PRECIO_RACIONAMIENTO = 1500.0;
// 1 de cada 20 lecturas del precio falla
const int FALLA_CADA = 20;
// Cada paso de la simulacion dura 1 hora (kWh = kW x horas)
const float HORAS_POR_PASO = 1.0;

// Si es true, cuando hay sobrecarga y alguna aplazable aun no cumple sus horas minimas,
// se protege a la que mas horas le faltan (mejora propia de la regla 6; ver informe).
// Si es false, la regla 6 se aplica literal: siempre se apaga la de mayor potencia.
const bool PRIORIZAR_HORAS_MINIMAS = true;

// Limites para validar los datos que escribe el usuario
const int HORA_MIN = 0;
const int HORA_MAX = 23;
const float PRECIO_MIN = 0;
const float EMBALSES_MIN = 0;
const float EMBALSES_MAX = 100;

// Rangos reales de precio de bolsa por franja (COP/kWh), tomados del Anexo B (XM, 21-27 sep 2026)
const int FIN_MADRUGADA = 6;    // madrugada: 0 a 6 h
const int FIN_MANANA = 14;      // manana y mediodio: 7 a 14 h
// tarde: 15 h hasta antes del pico; pico: PICO_INICIO a PICO_FIN; noche: despues del pico
const int PRECIO_MADRUGADA_MIN = 398, PRECIO_MADRUGADA_MAX = 640;
const int PRECIO_MANANA_MIN = 456,    PRECIO_MANANA_MAX = 970;
const int PRECIO_TARDE_MIN = 596,     PRECIO_TARDE_MAX = 1040;
const int PRECIO_PICO_MIN = 705,      PRECIO_PICO_MAX = 1612;
const int PRECIO_NOCHE_MIN = 565,     PRECIO_NOCHE_MAX = 970;

// =============================================================
// Clase base ABSTRACTA Carga: lo que tienen en comun todas las cargas
// =============================================================
class Carga {
// protected: lo ven esta clase y sus hijas (CargaCritica, CargaAplazable), no el resto del programa
protected:
    string nombre;      // nombre completo: "Torno CNC"
    string etiqueta;    // nombre corto, para las columnas de la tabla: "Torno"
    float potenciaKW;   // potencia en kW
    bool encendida;     // true si esta encendida en esta hora
    int horasUso;       // horas que ha estado encendida

public:
    // Constructor: recibe nombre, etiqueta y potencia
    Carga(string n, string e, float p) {
        nombre = n;
        etiqueta = e;
        potenciaKW = p;
        encendida = false;   // toda carga empieza apagada...
        horasUso = 0;        // ...y sin horas de uso
    }

    // Destructor virtual: obligatorio en una clase base con metodos virtuales
    virtual ~Carga() {}

    // POLIMORFISMO: cada clase hija decide a su manera si la carga se enciende.
    // "= 0" lo vuelve virtual puro: Carga no lo implementa, por eso es ABSTRACTA
    virtual bool debeEncender(int hora, float precio, float embalses) = 0;

    // Por defecto una carga no es aplazable; CargaAplazable lo cambia
    virtual bool esAplazable() { return false; }
    // Por defecto no tiene horas minimas; CargaAplazable lo cambia
    virtual int getHorasMinimas() { return 0; }

    // Enciende la carga durante esta hora (y suma una hora de uso)
    void encender() {
        encendida = true;
        horasUso++;
    }

    // Apaga la carga durante esta hora
    void apagar() { encendida = false; }

    // Deja la carga como nueva: apagada y sin horas de uso (al empezar cada simulacion)
    void reiniciar() {
        encendida = false;
        horasUso = 0;
    }

    // Cuantas horas minimas le faltan por cumplir (nunca negativo)
    int horasPendientes() {
        int pendientes = getHorasMinimas() - horasUso;
        if (pendientes < 0) pendientes = 0;
        return pendientes;
    }

    // Getters
    string getNombre() { return nombre; }
    string getEtiqueta() { return etiqueta; }
    float getPotencia() { return potenciaKW; }
    bool estaEncendida() { return encendida; }
    int getHorasUso() { return horasUso; }
};

// =============================================================
// Herencia: CargaCritica ES UNA Carga (nunca se apaga)
// =============================================================
class CargaCritica : public Carga {
public:
    // Le pasa los datos al constructor de Carga
    CargaCritica(string n, string e, float p) : Carga(n, e, p) {}

    // Regla 1: la carga critica siempre permanece encendida
    bool debeEncender(int hora, float precio, float embalses) override {
        return true;
    }
};

// =============================================================
// Herencia: CargaAplazable ES UNA Carga (se aplaza con energia cara)
// =============================================================
class CargaAplazable : public Carga {
private:
    int horasMinimas;   // horas que debe funcionar como minimo al dia

public:
    CargaAplazable(string n, string e, float p, int minimas) : Carga(n, e, p) {
        horasMinimas = minimas;
    }

    bool esAplazable() override { return true; }
    int getHorasMinimas() override { return horasMinimas; }

    // Reglas 2 a 5 en una sola condicion compuesta (&&, ||, !)
    bool debeEncender(int hora, float precio, float embalses) override {
        // horaPico es true si la hora esta entre 18 y 20
        bool horaPico = hora >= PICO_INICIO && hora <= PICO_FIN;
        // Se APLAZA si: embalses bajos (regla 2) || hora pico (regla 3) || precio caro (regla 4).
        // Se ENCIENDE si no se cumple nada de eso (regla 5): por eso el ! de afuera
        return !(embalses < NIVEL_ALERTA_EMBALSES || horaPico || precio > PRECIO_CARO);
    }
};

// =============================================================
// Medidor: acumula los kWh y el costo. Es PARTE del Negocio (composicion)
// =============================================================
class Medidor {
private:
    float kwhTotal;     // energia acumulada (kWh)
    float costoTotal;   // costo acumulado (COP)

public:
    Medidor() {
        kwhTotal = 0;
        costoTotal = 0;
    }

    // Registra una hora: suma la energia y el costo; devuelve el costo de esa hora
    float registrar(float potenciaKW, float precio) {
        float costoHora = potenciaKW * precio;           // kW x COP/kWh
        kwhTotal = kwhTotal + potenciaKW * HORAS_POR_PASO;  // kW x horas = kWh
        costoTotal = costoTotal + costoHora;
        return costoHora;
    }

    // Vuelve los acumulados a cero (al empezar cada simulacion)
    void reiniciar() {
        kwhTotal = 0;
        costoTotal = 0;
    }

    float getKwh() { return kwhTotal; }
    float getCosto() { return costoTotal; }
};

// =============================================================
// Mercado: da el precio de cada hora y el nivel de los embalses.
// El Negocio lo CONOCE y le pide datos (asociacion)
// =============================================================
class Mercado {
private:
    float nivelEmbalses;   // % de los embalses del pais

public:
    Mercado(float nivelInicial) { nivelEmbalses = nivelInicial; }

    float getNivelEmbalses() { return nivelEmbalses; }
    void setNivelEmbalses(float nivel) { nivelEmbalses = nivel; }

    // Precio al azar dentro del rango real de la franja de esa hora
    float precioHora(int hora) {
        int minimo, maximo;
        if (hora <= FIN_MADRUGADA) {
            minimo = PRECIO_MADRUGADA_MIN; maximo = PRECIO_MADRUGADA_MAX;
        } else if (hora <= FIN_MANANA) {
            minimo = PRECIO_MANANA_MIN; maximo = PRECIO_MANANA_MAX;
        } else if (hora < PICO_INICIO) {
            minimo = PRECIO_TARDE_MIN; maximo = PRECIO_TARDE_MAX;
        } else if (hora <= PICO_FIN) {
            minimo = PRECIO_PICO_MIN; maximo = PRECIO_PICO_MAX;
        } else {
            minimo = PRECIO_NOCHE_MIN; maximo = PRECIO_NOCHE_MAX;
        }
        // rand() % (maximo - minimo + 1) da un numero de 0 a la diferencia;
        // al sumarle el minimo queda entre minimo y maximo
        return minimo + rand() % (maximo - minimo + 1);
    }
};

// =============================================================
// Negocio: junta todo y simula el dia
// =============================================================
class Negocio {
private:
    string nombre;

    // COMPOSICION: los medidores son atributos objeto. Nacen y mueren con el negocio
    Medidor medidorCon;   // lo que gasta CON gestor
    Medidor medidorSin;   // lo que gastaria SIN gestor (las tres cargas todo el dia)

    // AGREGACION: arreglo de punteros Carga*. Las cargas se crean afuera
    // y aqui solo se guarda su direccion
    Carga* cargas[MAX_CARGAS];
    int numCargas;

    // ASOCIACION: puntero al mercado. El negocio no lo crea: solo lo conoce y le pide datos
    Mercado* mercado;

    // Resultados de la ultima simulacion (para el resumen, opcion 4)
    int horasSimuladas, fallas, alertasPotencia;
    bool emergencia, produccionIncompleta;

    // Decide que cargas se encienden en una lectura (reglas 1 a 6).
    // Llena quiere[] (true = se enciende) y la potencia que se pedia antes de recortar.
    // Devuelve la posicion de la carga que la regla 6 apago, o -1 si no hubo recorte.
    int decidir(int hora, float precio, float embalses, bool quiere[],
                float &potenciaPedida, bool priorizarMinimas) {
        potenciaPedida = 0;
        // Reglas 1 a 5: el MISMO ciclo le pregunta a todas las cargas (polimorfismo)
        for (int i = 0; i < numCargas; i++) {
            quiere[i] = cargas[i]->debeEncender(hora, precio, embalses);
            if (quiere[i]) potenciaPedida += cargas[i]->getPotencia();
        }

        // Regla 6: si la potencia supera la contratada, se apaga una aplazable
        int recortada = -1;
        if (potenciaPedida > POTENCIA_CONTRATADA) {
            for (int i = 0; i < numCargas; i++) {
                if (quiere[i] && cargas[i]->esAplazable()) {
                    if (recortada == -1) {
                        recortada = i;
                    } else if (priorizarMinimas) {
                        // Se apaga la que menos horas minimas tiene pendientes;
                        // si empatan, la de mayor potencia
                        int pendienteI = cargas[i]->horasPendientes();
                        int pendienteR = cargas[recortada]->horasPendientes();
                        if (pendienteI < pendienteR ||
                            (pendienteI == pendienteR &&
                             cargas[i]->getPotencia() > cargas[recortada]->getPotencia())) {
                            recortada = i;
                        }
                    } else if (cargas[i]->getPotencia() > cargas[recortada]->getPotencia()) {
                        // Regla 6 literal: la aplazable de MAYOR potencia
                        recortada = i;
                    }
                }
            }
            if (recortada != -1) quiere[recortada] = false;
        }
        return recortada;
    }

public:
    // Constructor: recibe el nombre y la direccion del mercado
    Negocio(string n, Mercado* m) {
        nombre = n;
        mercado = m;
        numCargas = 0;
        horasSimuladas = 0; fallas = 0; alertasPotencia = 0;
        emergencia = false; produccionIncompleta = false;
    }

    // Agrega una carga al negocio (agregacion). Recibe su direccion
    void agregarCarga(Carga* c) {
        if (numCargas < MAX_CARGAS) {
            cargas[numCargas] = c;
            numCargas++;
        }
    }

    Mercado* getMercado() { return mercado; }

    // Muestra el negocio y sus cargas
    void mostrar() {
        cout << fixed << setprecision(1);
        cout << NOMBRE_NEGOCIO << " | potencia contratada " << POTENCIA_CONTRATADA << " kW" << endl;
        for (int i = 0; i < numCargas; i++) {
            cout << "  " << cargas[i]->getNombre() << " (" << cargas[i]->getPotencia() << " kW)"
                 << (cargas[i]->esAplazable() ? " - aplazable" : " - critica") << endl;
        }
    }

    // Opcion 2: que haria cada carga con una lectura (hora, precio, embalses)
    void evaluar(int hora, float precio, float embalses) {
        bool quiere[MAX_CARGAS];
        float potenciaPedida;
        // Aqui la regla 6 es literal (false): siempre se apaga la aplazable de mayor potencia
        int recortada = decidir(hora, precio, embalses, quiere, potenciaPedida, false);

        cout << fixed << setprecision(1);
        if (embalses < NIVEL_ALERTA_EMBALSES) {
            cout << "  ALERTA: riesgo de racionamiento. Solo funciona la carga critica." << endl;
        }
        for (int i = 0; i < numCargas; i++) {
            string decision;
            if (quiere[i]) decision = "ENCENDER";
            else if (i == recortada) decision = "APAGAR (supera la potencia contratada)";
            else decision = "APLAZAR";
            cout << "  " << setw(28) << left << cargas[i]->getNombre() << right << decision << endl;
        }
        if (recortada != -1) {
            cout << "  ALERTA: se pedian " << potenciaPedida << " kW y solo hay "
                 << POTENCIA_CONTRATADA << " kW contratados. Se apaga: "
                 << cargas[recortada]->getNombre() << "." << endl;
        }
    }

    // Opcion 3: simula 24 horas, con gestor y sin gestor
    void simularDia() {
        // Todo empieza en cero para cada simulacion
        medidorCon.reiniciar();
        medidorSin.reiniciar();
        horasSimuladas = 0; fallas = 0; alertasPotencia = 0;
        emergencia = false; produccionIncompleta = false;
        float potenciaTodas = 0;   // kW de las tres cargas encendidas (para el costo sin gestor)
        for (int i = 0; i < numCargas; i++) {
            cargas[i]->reiniciar();
            potenciaTodas += cargas[i]->getPotencia();
        }

        // Nivel de los embalses: se lo pide al mercado (asociacion)
        float embalses = mercado->getNivelEmbalses();
        cout << fixed << setprecision(0);
        cout << "Embalses: " << embalses << " %" << endl;
        if (embalses < NIVEL_ALERTA_EMBALSES) {
            cout << "ALERTA: riesgo de racionamiento. Solo funciona la carga critica." << endl;
        }

        // Encabezado de la tabla
        cout << setw(5) << "Hora" << setw(8) << "Precio";
        for (int i = 0; i < numCargas; i++) cout << setw(9) << cargas[i]->getEtiqueta();
        cout << setw(9) << "Costo" << endl;

        // Repite para cada hora del dia: h = 0, 1, ..., 23
        for (int h = 0; h < HORAS_DIA; h++) {
            // ASOCIACION: el negocio le pide el precio de esta hora al mercado
            float precio = mercado->precioHora(h);

            // break: emergencia de racionamiento (precio muy alto Y embalses bajos)
            if (precio > PRECIO_RACIONAMIENTO && embalses < NIVEL_ALERTA_EMBALSES) {
                emergencia = true;
                cout << setw(5) << h << setw(8) << precio
                     << "  EMERGENCIA DE RACIONAMIENTO: se detiene la simulacion" << endl;
                break;   // sale del for: no se simulan las horas que faltan
            }

            // Esta hora si se simula
            horasSimuladas++;
            // Sin gestor: las tres cargas encendidas toda la hora
            medidorSin.registrar(potenciaTodas, precio);

            // continue: 1 de cada FALLA_CADA lecturas del precio falla
            if (rand() % FALLA_CADA == 0) {
                float potenciaHora = 0;
                // Solo funciona la carga critica; las aplazables se apagan
                for (int i = 0; i < numCargas; i++) {
                    if (cargas[i]->esAplazable()) {
                        cargas[i]->apagar();
                    } else {
                        cargas[i]->encender();
                        potenciaHora += cargas[i]->getPotencia();
                    }
                }
                float costoHora = medidorCon.registrar(potenciaHora, precio);
                fallas++;
                cout << setw(5) << h << setw(8) << "--";
                for (int i = 0; i < numCargas; i++) {
                    cout << setw(9) << (cargas[i]->estaEncendida() ? "ON" : "off");
                }
                cout << setw(9) << costoHora << "  falla de lectura: solo la critica" << endl;
                continue;   // salta a la siguiente hora
            }

            // Con gestor: cada carga decide (polimorfismo) y la regla 6 recorta si hace falta
            bool quiere[MAX_CARGAS];
            float potenciaPedida;
            int recortada = decidir(h, precio, embalses, quiere, potenciaPedida,
                                    PRIORIZAR_HORAS_MINIMAS);
            float potenciaHora = 0;
            for (int i = 0; i < numCargas; i++) {
                if (quiere[i]) {
                    cargas[i]->encender();
                    potenciaHora += cargas[i]->getPotencia();
                } else {
                    cargas[i]->apagar();
                }
            }
            // El medidor acumula los kWh y el costo de la hora
            float costoHora = medidorCon.registrar(potenciaHora, precio);

            cout << setw(5) << h << setw(8) << precio;
            for (int i = 0; i < numCargas; i++) {
                cout << setw(9) << (cargas[i]->estaEncendida() ? "ON" : "off");
            }
            cout << setw(9) << costoHora;
            if (recortada != -1) {
                alertasPotencia++;
                cout << "  SOBRECARGA: se apaga " << cargas[recortada]->getEtiqueta();
            }
            cout << endl;
        }

        // Horas minimas: cada aplazable debe haber funcionado su minimo del dia
        for (int i = 0; i < numCargas; i++) {
            if (cargas[i]->horasPendientes() > 0) produccionIncompleta = true;
        }
        if (produccionIncompleta) cout << "ALERTA: produccion incompleta." << endl;
        cout << "Simulacion terminada. Vea el resumen con la opcion 4." << endl;
    }

    // Opcion 4: resumen y ahorro
    void resumen() {
        float ahorro = medidorSin.getCosto() - medidorCon.getCosto();
        cout << fixed << setprecision(0);
        cout << NOMBRE_NEGOCIO << " | embalses " << mercado->getNivelEmbalses() << " %" << endl;
        cout << "Horas simuladas: " << horasSimuladas << " de " << HORAS_DIA
             << " | fallas de lectura: " << fallas
             << " | alertas de sobrecarga: " << alertasPotencia << endl;
        // Una linea por carga: horas encendida y energia (horas x kW = kWh)
        for (int i = 0; i < numCargas; i++) {
            cout << "  " << setw(28) << left << cargas[i]->getNombre() << right
                 << setw(3) << cargas[i]->getHorasUso() << " h, " << fixed << setprecision(1)
                 << setw(6) << cargas[i]->getHorasUso() * cargas[i]->getPotencia() << " kWh";
            if (cargas[i]->esAplazable()) {
                cout << "  (minimo " << cargas[i]->getHorasMinimas() << " h: "
                     << (cargas[i]->horasPendientes() == 0 ? "cumplido" : "NO cumplido") << ")";
            }
            cout << endl;
        }
        cout << fixed << setprecision(1);
        cout << "Energia con gestor: " << medidorCon.getKwh() << " kWh | sin gestor: "
             << medidorSin.getKwh() << " kWh" << endl;
        cout << fixed << setprecision(0);
        cout << "Costo con gestor: $" << medidorCon.getCosto() << endl;
        cout << "Costo sin gestor: $" << medidorSin.getCosto() << endl;
        cout << "Ahorro:           $" << ahorro;
        // El porcentaje solo si el costo sin gestor es mayor que 0, para no dividir por 0
        if (medidorSin.getCosto() > 0) {
            cout << " (" << setprecision(1) << ahorro / medidorSin.getCosto() * 100 << " %)";
        }
        cout << endl;
        if (emergencia) cout << "ALERTA: hubo emergencia de racionamiento y la simulacion se detuvo." << endl;
        if (produccionIncompleta) cout << "ALERTA: produccion incompleta." << endl;
    }
};

// =============================================================
// main: el programa empieza aqui
// =============================================================
int main() {
    // Misma semilla = mismos precios al azar en cada ejecucion
    srand(SEMILLA);

    // El mercado existe por su cuenta: no es parte del negocio
    Mercado mercado(NIVEL_EMBALSES_INICIAL);

    // Las cargas tambien se crean afuera del negocio
    CargaCritica iluminacion("Iluminacion y computadores", "Ilum/PC", POTENCIA_ILUMINACION);
    CargaAplazable torno("Torno CNC", "Torno", POTENCIA_TORNO, HORAS_MIN_TORNO);
    CargaAplazable compresor("Compresor", "Compres", POTENCIA_COMPRESOR, HORAS_MIN_COMPRESOR);

    // Se crea el negocio con su mercado (asociacion) y se le agregan las cargas (agregacion)
    Negocio taller(NOMBRE_NEGOCIO, &mercado);
    taller.agregarCarga(&iluminacion);
    taller.agregarCarga(&torno);
    taller.agregarCarga(&compresor);
    taller.mostrar();
    // Numeros sin decimales en los mensajes del menu
    cout << fixed << setprecision(0);

    // Lectura manual: empieza con valores de ejemplo
    int hora = 12;
    float precio = 500, embalses = NIVEL_EMBALSES_INICIAL;
    bool haySimulacion = false;   // true cuando ya se simulo un dia
    int opcion;

    // do-while: el menu se muestra al menos una vez y se repite mientras la opcion no sea 0
    do {
        cout << endl << "===== WattControl v2 - " << NOMBRE_NEGOCIO << " =====" << endl;
        cout << "1. Ingresar lectura manual (hora, precio, embalses)" << endl;
        cout << "2. Evaluar decision de cada carga" << endl;
        cout << "3. Simular un dia completo (24 h)" << endl;
        cout << "4. Ver resumen de la ultima simulacion" << endl;
        cout << "0. Salir" << endl;
        cout << "Opcion: ";
        cin >> opcion;
        // Si el usuario escribio una letra, cin falla: se limpia y la opcion queda invalida
        if (cin.fail()) { cin.clear(); opcion = -1; }
        // Descarta lo que sobre de la linea (por ejemplo el Enter)
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (opcion) {
        // Opcion 1: pedir la lectura, validando cada dato con do-while
        case 1:
            do {
                cout << "Hora (" << HORA_MIN << " a " << HORA_MAX << "): ";
                cin >> hora;
                if (cin.fail()) { cin.clear(); hora = HORA_MIN - 1; }
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                if (hora < HORA_MIN || hora > HORA_MAX) cout << "  Hora invalida." << endl;
            } while (hora < HORA_MIN || hora > HORA_MAX);

            do {
                cout << "Precio de bolsa (COP/kWh, mayor que 0): ";
                cin >> precio;
                if (cin.fail()) { cin.clear(); precio = PRECIO_MIN; }
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                if (precio <= PRECIO_MIN) cout << "  El precio debe ser mayor que 0." << endl;
            } while (precio <= PRECIO_MIN);

            do {
                cout << "Nivel de los embalses (" << EMBALSES_MIN << " a " << EMBALSES_MAX << " %): ";
                cin >> embalses;
                if (cin.fail()) { cin.clear(); embalses = EMBALSES_MIN - 1; }
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                if (embalses < EMBALSES_MIN || embalses > EMBALSES_MAX) cout << "  Nivel invalido." << endl;
            } while (embalses < EMBALSES_MIN || embalses > EMBALSES_MAX);

            // El mercado tambien recuerda los embalses: la simulacion (opcion 3) los usa
            mercado.setNivelEmbalses(embalses);
            cout << "Lectura guardada (los embalses tambien se usan en la simulacion)." << endl;
            break;
        // Opcion 2: que haria cada carga con la lectura guardada
        case 2:
            cout << fixed << setprecision(0);
            cout << "Hora " << hora << " | precio " << precio << " | embalses " << embalses << " %" << endl;
            taller.evaluar(hora, precio, embalses);
            break;
        // Opcion 3: simular las 24 horas
        case 3:
            taller.simularDia();
            haySimulacion = true;
            break;
        // Opcion 4: el resumen, solo si ya se simulo un dia
        case 4:
            if (haySimulacion) taller.resumen();
            else cout << "Primero simule un dia con la opcion 3." << endl;
            break;
        // Opcion 0: salir
        case 0:
            cout << "Hasta luego." << endl;
            break;
        // default: cualquier otro valor
        default:
            cout << "Opcion invalida." << endl;
        }

        // Pausa para leer el resultado antes de volver al menu
        if (opcion != 0) {
            cout << endl << "Presione Enter para continuar...";
            cin.get();
        }
    } while (opcion != 0);

    return 0;
}
