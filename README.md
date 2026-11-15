# WattControl - Taller de mecanizado

Gestor de energia orientado a objetos en C++ para un taller de mecanizado ante El Nino 2026.
A partir de la hora, el precio de bolsa y el nivel de los embalses decide que cargas encender,
apagar o aplazar, y simula un dia de 24 horas para medir el ahorro.

**Autora:** Valeria Fernandez Vergara (trabajo individual) - Programacion Orientada a Objetos, IU Digital de Antioquia (EA2)

## Negocio y cargas

| Carga | Tipo | Potencia |
|---|---|---|
| Iluminacion y computadores | Critica (nunca se apaga) | 1,5 kW |
| Torno CNC | Aplazable (minimo 6 h/dia) | 7,5 kW |
| Compresor | Aplazable (minimo 4 h/dia) | 3,0 kW |

Potencia contratada: 10 kW. Precio "caro": mayor que 750 COP/kWh. Hora pico: 18 a 20 h.

## Clases y relaciones

- **Herencia:** `CargaCritica` y `CargaAplazable` heredan de `Carga` (abstracta).
- **Polimorfismo:** cada hija implementa `debeEncender()` con su propia regla.
- **Composicion:** `Medidor` es parte de `Negocio`.
- **Agregacion:** `Negocio` guarda `Carga*` creadas afuera.
- **Asociacion:** `Negocio` conoce a `Mercado` y le pide el precio y los embalses.

## Compilar y ejecutar

```
g++ wattcontrol.cpp -o wattcontrol
./wattcontrol
```

## Contenido

- `wattcontrol.cpp`: programa completo
- `diagrama/`: diagrama de clases (PNG y fuente .dot)
- `pruebas/`: capturas de la tabla de pruebas
- `herramientas/buscar_semilla.cpp`: busca semillas de `rand()` con falla de lectura y precio alto
- `Informe_EA2_WattControl.pdf`: informe de la evidencia
