# ACME Ascensor Inteligente — Actividad 2
**Equipos e Instrumentación Electrónica | MUIT UNIR**

Simulación en Arduino/Wokwi de un sistema de control para ascensor de 5 plantas con supervisión ambiental de temperatura e iluminación.

🔗 [Simulación en Wokwi](https://wokwi.com/projects/461746891975678977)

---

## Descripción del sistema

El sistema integra tres bloques funcionales sobre un Arduino UNO:

1. **Control del ascensor** — 5 pulsadores de llamada (uno por planta) y un servomotor SG90 que simula el desplazamiento de la cabina
2. **Control de temperatura** — algoritmo ON-OFF con zona muerta (setpoint 25°C ± 3°C) con actuadores de calefacción y refrigeración simulados con LEDs
3. **Control de iluminación** — 8 LEDs controlados via 74HC595 que se encienden proporcionalmente al déficit de luz (setpoint 80%)

---

## Hardware

| Componente | Pin Arduino | Función |
|---|---|---|
| DHT22 | 13 | Temperatura y humedad |
| Servo SG90 | A5 | Movimiento cabina |
| LCD 16x2 | RS:12 / E:11 / D4-D7: 5,4,3,6 | Display HMI |
| Pulsador P1 | A0 | Llamada planta 1 |
| Pulsador P2 | A1 | Llamada planta 2 |
| Pulsador P3 | A2 | Llamada planta 3 |
| Pulsador P4 | A3 | Llamada planta 4 |
| Pulsador P5 | A4 | Llamada planta 5 |
| 74HC595 | DS:10 / STCP:9 / SHCP:8 | Control 8 LEDs iluminación |
| LED Rojo | 7 | Actuador calefacción |
| LED Azul | 2 | Actuador refrigeración |

---

## Librerías necesarias

- `LiquidCrystal.h` — incluida en Arduino IDE
- `DHT.h` — instalar desde Library Manager: buscar "DHT sensor library" de Adafruit
- `Servo.h` — incluida en Arduino IDE

---

## Lógica de control

### Ascensor
Cada planta tiene un ángulo asignado en el servo:

| Planta | Ángulo servo |
|---|---|
| 1 | 0° |
| 2 | 36° |
| 3 | 72° |
| 4 | 108° |
| 5 | 144° |

Al pulsar el botón de una planta, el servo se mueve grado a grado (15ms/grado) desde la posición actual hasta el destino.

### Control de temperatura — ON-OFF con zona muerta
```
Setpoint: 25°C
Zona muerta: ±3°C

Si T > 28°C  → LED azul ON  (refrigeración)
Si T < 22°C  → LED rojo ON  (calefacción)
Si 22 ≤ T ≤ 28 → ambos OFF (zona muerta, sin acción)
```

### Control de iluminación — ON-OFF escalonado
```
Setpoint: 80%
Déficit = 80% - luz_medida
LEDs encendidos = déficit / 10  (máx 8)
```

---

## Display LCD

- **Fila 0:** planta actual o movimiento en curso (`Planta X -> Y`)
- **Fila 1:** temperatura anterior vs actual (`Ta:23 Tc:26C`)

---

## Log serie (ejemplo)
```
Planta:3 | T:24.0C | H:40.0% | Luz:65%
CTRL TEMP: OK zona muerta
CTRL LUZ: 65% -> LEDs: 2
```

---

## Estructura del repositorio

```
ACME-Ascensor-MUIT/
├── README.md
└── acme_ascensor_act2.ino
```

---

## Decisiones de diseño

- Se eliminó el HC-SR04 de la Act1 para liberar pines necesarios para los nuevos actuadores
- El 74HC595 permite controlar 8 LEDs con solo 3 pines, clave dado que el Arduino UNO no tiene pines suficientes para conexión directa
- El servo se conecta a A5 (pin analógico usado como digital) porque la librería Servo.h funciona sobre cualquier pin
- La luz se simula con variación aleatoria al no disponer de pin libre para LDR

---

## Referencias

- Cameron, N. (2019). *Arduino Applied*. Apress.
- Corona, L. (2014). *Sensores y actuadores. Aplicaciones con Arduino*. Grupo Editorial Patria.
- Llamas, L. Tutoriales Arduino. https://www.luisllamas.es/tutoriales-arduino/
- Torrente, O. (2013). *Arduino. Curso práctico de formación*. RC Libros.
