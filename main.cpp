#include <MCUFRIEND_kbv.h>
#include <avr/sleep.h>
#include <avr/power.h>

// Definir pines
#define TFT_CS    10
#define TFT_DC    9
#define TFT_RST   8
#define FREQ_PIN  13  // Pin 13 para la señal de entrada
#define WAKE_PIN  12  // Pin para el botón de despertar

// Inicializar pantalla
MCUFRIEND_kbv tft;

bool sleepMode = false;
float frequency = 0;
float lastFrequency = -1;  // Almacena la última frecuencia mostrada


void wakeUp() {
  // Este código se ejecuta al despertar
  // Desactivar la interrupción para evitar que despierte repetidamente
  detachInterrupt(digitalPinToInterrupt(WAKE_PIN));

  // Reactivar la pantalla
  tft.pushCommand(0x29, NULL, 0);  // Comando para encender display (display on)
  delay(120);  // Tiempo para estabilizar la pantalla
  tft.fillScreen(TFT_BLACK);  // Resetear la pantalla
  Serial.println("Sistema despierto.");
}

void enterDeepSleep() {
  // Apagar el controlador de la pantalla
  tft.pushCommand(0x28, NULL, 0);  // Comando para apagar display (display off)

  Serial.println("Entrando en modo de suspensión profunda...");
  delay(500);

  // Habilitar la interrupción externa para despertar el Arduino
  attachInterrupt(digitalPinToInterrupt(WAKE_PIN), wakeUp, LOW);  // Interrumpir en flanco bajo

  // Configurar el modo de suspensión del Arduino en POWER_DOWN
  tft.fillScreen(TFT_BLACK);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Modo más profundo de suspensión
  sleep_enable();  // Habilitar modo de suspensión
  sleep_cpu();     // Entrar en modo de suspensión (requiere un reinicio para salir)

  // En este punto, el código no continuará hasta que se despierte
}


void displayFrequency() {
  if (abs(frequency - lastFrequency) > 150) {
    lastFrequency = frequency;
    tft.fillRect(50, 80, 200, 40, TFT_BLACK);
    tft.setCursor(50, 80);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.print("Freq: ");
    tft.print(frequency, 2);
    tft.println(" Hz");
  }
}

void detectFrequency() {
  unsigned long startTime = 0;
  unsigned long endTime = 0;
  unsigned long timeout = millis();

  while (digitalRead(FREQ_PIN) == LOW) {
    if (millis() - timeout > 500) {
      frequency = 0;
      return;
    }
  }
  startTime = micros();

  timeout = millis();
  while (digitalRead(FREQ_PIN) == HIGH) {
    if (millis() - timeout > 500) {
      frequency = 0;
      return;
    }
  }
  while (digitalRead(FREQ_PIN) == LOW) {
    if (millis() - timeout > 500) {
      frequency = 0;
      return;
    }
  }
  endTime = micros();

  unsigned long period = endTime - startTime;
  frequency = (period > 0) ? (1000000.0 / period) : 0;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

    // Configurar pin del botón como entrada
  // pinMode(WAKE_PIN, INPUT_PULLUP);  // Usar resistencia de pull-up interna

  tft.begin(tft.readID());
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(50, 50);
  tft.println("Running...");
  
  Serial.println("Sistema activo. Escribe 'sleep' para entrar en modo de suspensión o 'wake' para despertarlo.");

  pinMode(FREQ_PIN, INPUT);  // Configurar pin de frecuencia como entrada
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');

    if (command == "sleep") {
      sleepMode = true;
    }

    if (command == "wake") {
      sleepMode = false;
      Serial.println("Despertando el sistema...");
      wakeUp();

    // Si se presiona el botón, entrar en modo de suspensión profunda
    // if (digitalRead(WAKE_PIN) == LOW) {
    //   sleepMode = true;
    //   enterDeepSleep();
    // }
    
      }
  }


  if (sleepMode) {
    enterDeepSleep();
  }else {
    detectFrequency();
    
    displayFrequency();
  }
}
