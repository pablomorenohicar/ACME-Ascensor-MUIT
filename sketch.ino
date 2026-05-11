#include <LiquidCrystal.h>
#include <DHT.h>
#include <Servo.h>

// ── Pines ──────────────────────────────────────────
#define DHT_PIN        13
#define DHT_TIPO       DHT22
#define SERVO_PIN      A5
#define BTN_P1         A0
#define BTN_P2         A1
#define BTN_P3         A2
#define BTN_P4         A3
#define BTN_P5         A4
#define LED_CALOR      7
#define LED_FRIO       2
#define SR_DS          10
#define SR_STCP        9
#define SR_SHCP        8

// ── Control temperatura ────────────────────────────
#define TEMP_SETPOINT  25.0
#define TEMP_ZONA      3.0
#define LUZ_SETPOINT   80.0

// ── Ángulos servo por planta ───────────────────────
const int ANGULO[6] = {0, 0, 36, 72, 108, 144};

// ── Objetos ────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TIPO);
LiquidCrystal lcd(12, 11, 5, 4, 3, 6);
Servo servo;

// ── Estado ascensor ────────────────────────────────
int plantaActual  = 1;
bool moviendose   = false;

// ── Estado control temperatura ─────────────────────
float tempAnterior = 0;
float tempActual   = 0;
float humActual    = 0;

// ── Estado iluminación (simulada 0-100%) ───────────
float luzSimulada  = 50.0; // arranca al 50% para que el control actúe

// ──────────────────────────────────────────────────
// Escribe exactamente 16 chars en una fila del LCD
void escribirFila(int fila, const char* texto) {
  char buf[17];
  int len = strlen(texto);
  for (int i = 0; i < 16; i++) buf[i] = (i < len) ? texto[i] : ' ';
  buf[16] = '\0';
  lcd.setCursor(0, fila);
  lcd.print(buf);
}

// ──────────────────────────────────────────────────
// Envía un byte al 74HC595 para controlar los 8 LEDs
void enviarLEDs(byte valor) {
  digitalWrite(SR_STCP, LOW);
  shiftOut(SR_DS, SR_SHCP, MSBFIRST, valor);
  digitalWrite(SR_STCP, HIGH);
}

// ──────────────────────────────────────────────────
// Control ON-OFF con zona muerta para temperatura
// Setpoint: 25°C, zona muerta: ±3°C
void controlTemperatura(float temp) {
  float limMax = TEMP_SETPOINT + TEMP_ZONA;  // 28°C
  float limMin = TEMP_SETPOINT - TEMP_ZONA;  // 22°C

  if (temp > limMax) {
    // Demasiado calor → enfriar
    digitalWrite(LED_CALOR, LOW);
    digitalWrite(LED_FRIO,  HIGH);
    Serial.println(F("CTRL TEMP: Enfriando"));
  } else if (temp < limMin) {
    // Demasiado frío → calentar
    digitalWrite(LED_CALOR, HIGH);
    digitalWrite(LED_FRIO,  LOW);
    Serial.println(F("CTRL TEMP: Calentando"));
  } else {
    // Dentro de zona muerta → sin acción
    digitalWrite(LED_CALOR, LOW);
    digitalWrite(LED_FRIO,  LOW);
    Serial.println(F("CTRL TEMP: OK zona muerta"));
  }
}

// ──────────────────────────────────────────────────
// Control ON-OFF iluminación con 8 LEDs via 74HC595
// Setpoint: 80%. Cada LED representa ~12.5%
// Si luz < setpoint → encender más LEDs
// Si luz >= setpoint → apagar LEDs
void controlIluminacion(float luz) {
  int ledsEncendidos = 0;

  if (luz < LUZ_SETPOINT) {
    // Proporcional: cuanto menos luz, más LEDs
    float deficit = LUZ_SETPOINT - luz;        // 0-80
    ledsEncendidos = (int)(deficit / 10.0);    // 0-8
    if (ledsEncendidos > 8) ledsEncendidos = 8;
  }

  // Crear máscara de bits: encender N LEDs desde Q0
  byte mascara = 0;
  for (int i = 0; i < ledsEncendidos; i++) {
    mascara |= (1 << i);
  }
  enviarLEDs(mascara);

  Serial.print(F("CTRL LUZ: "));
  Serial.print(luz, 0);
  Serial.print(F("% → LEDs: "));
  Serial.println(ledsEncendidos);
}

// ──────────────────────────────────────────────────
// Mover servo a la planta destino grado a grado
void moverAscensor(int destino) {
  if (destino == plantaActual) return;

  moviendose = true;

  // LCD: mostrar movimiento
  char buf[17];
  buf[0]='P'; buf[1]='l'; buf[2]='a'; buf[3]='n'; buf[4]='t'; buf[5]='a'; buf[6]=' ';
  buf[7] = '0' + plantaActual;
  buf[8] = '-'; buf[9] = '>'; buf[10] = ' ';
  buf[11] = '0' + destino;
  buf[12] = '\0';
  escribirFila(0, buf);
  escribirFila(1, "Moviendo...");

  int angOrigen  = ANGULO[plantaActual];
  int angDestino = ANGULO[destino];
  int paso       = (angDestino > angOrigen) ? 1 : -1;

  for (int ang = angOrigen; ang != angDestino; ang += paso) {
    servo.write(ang);
    delay(15);
  }
  servo.write(angDestino);
  plantaActual = destino;
  moviendose   = false;

  // LCD: mostrar planta actual
  char buf2[17];
  buf2[0]='P'; buf2[1]='l'; buf2[2]='a'; buf2[3]='n'; buf2[4]='t'; buf2[5]='a'; buf2[6]=' ';
  buf2[7] = '0' + plantaActual;
  buf2[8] = '\0';
  escribirFila(0, buf2);

  // LCD fila 1: temperatura anterior vs actual
  char buf3[17];
  int tA = (int)tempAnterior;
  int tC = (int)tempActual;
  // "Ta:XX Tc:XX C"
  buf3[0]='T'; buf3[1]='a'; buf3[2]=':';
  buf3[3] = '0' + (tA / 10 % 10);
  buf3[4] = '0' + (tA % 10);
  buf3[5] = ' ';
  buf3[6]='T'; buf3[7]='c'; buf3[8]=':';
  buf3[9] = '0' + (tC / 10 % 10);
  buf3[10] = '0' + (tC % 10);
  buf3[11] = 'C'; buf3[12] = '\0';
  escribirFila(1, buf3);
}

// ──────────────────────────────────────────────────
int leerPulsador() {
  if (digitalRead(BTN_P1) == LOW) { delay(50); if (digitalRead(BTN_P1) == LOW) return 1; }
  if (digitalRead(BTN_P2) == LOW) { delay(50); if (digitalRead(BTN_P2) == LOW) return 2; }
  if (digitalRead(BTN_P3) == LOW) { delay(50); if (digitalRead(BTN_P3) == LOW) return 3; }
  if (digitalRead(BTN_P4) == LOW) { delay(50); if (digitalRead(BTN_P4) == LOW) return 4; }
  if (digitalRead(BTN_P5) == LOW) { delay(50); if (digitalRead(BTN_P5) == LOW) return 5; }
  return 0;
}

// ──────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16, 2);
  servo.attach(SERVO_PIN);

  pinMode(LED_CALOR, OUTPUT);
  pinMode(LED_FRIO,  OUTPUT);
  pinMode(SR_DS,     OUTPUT);
  pinMode(SR_STCP,   OUTPUT);
  pinMode(SR_SHCP,   OUTPUT);
  digitalWrite(LED_CALOR, LOW);
  digitalWrite(LED_FRIO,  LOW);
  enviarLEDs(0);

  pinMode(BTN_P1, INPUT_PULLUP);
  pinMode(BTN_P2, INPUT_PULLUP);
  pinMode(BTN_P3, INPUT_PULLUP);
  pinMode(BTN_P4, INPUT_PULLUP);
  pinMode(BTN_P5, INPUT_PULLUP);

  servo.write(ANGULO[1]);
  delay(500);

  escribirFila(0, "Sistema ACME");
  escribirFila(1, "Iniciando...");
  delay(2000);

  // Lectura inicial
  tempActual   = dht.readTemperature();
  humActual    = dht.readHumidity();
  tempAnterior = tempActual;
  if (isnan(tempActual)) tempActual = 25.0;
  if (isnan(humActual))  humActual  = 50.0;

  escribirFila(0, "Planta 1");
  escribirFila(1, "Listo");
}

// ──────────────────────────────────────────────────
void loop() {

  // ── 1. Leer pulsadores y mover ascensor ──────────
  int pulsado = leerPulsador();
  if (pulsado > 0 && pulsado != plantaActual && !moviendose) {
    tempAnterior = tempActual;
    moverAscensor(pulsado);
  }

  // ── 2. Leer sensores cada 2 segundos ─────────────
  static unsigned long ultimaLectura = 0;
  if (millis() - ultimaLectura > 2000) {
    ultimaLectura = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) tempActual = t;
    if (!isnan(h)) humActual  = h;

    // Log serie
    Serial.print(F("Planta:"));  Serial.print(plantaActual);
    Serial.print(F(" | T:"));    Serial.print(tempActual, 1);
    Serial.print(F("C | H:"));   Serial.print(humActual, 1);
    Serial.print(F("% | Luz:")); Serial.print(luzSimulada, 0);
    Serial.println(F("%"));

    // ── 3. Control temperatura ON-OFF zona muerta ──
    controlTemperatura(tempActual);

    // ── 4. Control iluminación ─────────────────────
    // Simulamos variación de luz: sube/baja 5% cada ciclo
    // En real vendría de un LDR por pin analógico
    luzSimulada += random(-8, 9);
    if (luzSimulada > 100) luzSimulada = 100;
    if (luzSimulada < 0)   luzSimulada = 0;
    controlIluminacion(luzSimulada);

    // ── 5. Actualizar LCD si no se está moviendo ───
    if (!moviendose) {
      char buf0[17];
      buf0[0]='P'; buf0[1]='l'; buf0[2]='a'; buf0[3]='n'; buf0[4]='t'; buf0[5]='a'; buf0[6]=' ';
      buf0[7] = '0' + plantaActual;
      buf0[8] = '\0';
      escribirFila(0, buf0);

      char buf1[17];
      int tA = (int)tempAnterior;
      int tC = (int)tempActual;
      buf1[0]='T'; buf1[1]='a'; buf1[2]=':';
      buf1[3] = (tA >= 10) ? ('0' + tA / 10 % 10) : ' ';
      buf1[4] = '0' + tA % 10;
      buf1[5] = ' ';
      buf1[6]='T'; buf1[7]='c'; buf1[8]=':';
      buf1[9] = (tC >= 10) ? ('0' + tC / 10 % 10) : ' ';
      buf1[10] = '0' + tC % 10;
      buf1[11] = 'C'; buf1[12] = '\0';
      escribirFila(1, buf1);
    }
  }

  delay(100);
}
// 

