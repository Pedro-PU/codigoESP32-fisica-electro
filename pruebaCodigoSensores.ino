#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// Configuración WiFi
const char* WIFI_SSID = "CNT_GPON_PESANTEZ";
const char* WIFI_PASSWORD = "Lampara2016";

// Configuración Firebase
const char* API_KEY = "AIzaSyBGQwBXar9deS2CCicS_nXLrNIyvRaegg4";
const char* DATABASE_URL = "https://tren-44bd7-default-rtdb.firebaseio.com/";
const char* USER_EMAIL = "tren@tren.com";
const char* USER_PASSWORD = "holamundo";

// Objetos Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pines
const int hallSensorPin = 25;
const int hallSensorAnalog = 34;
const int hallSensorUni = 27;
const int ledPin = 26;

// Motor
const int motorIN1 = 13;
const int motorIN2 = 5;
const int pwmChannelIN1 = 0;
const int pwmChannelIN2 = 1;
const int pwmFreq = 5000;
const int pwmResolution = 8;

// Variables globales compartidas
int cont = 0;
int hallValue = 0;
int hallUni = 0;
int valorAnalogico = 0;
int velocidadPWM = 0;
float velocidadMedia = 0;
float velocidadLineal = 0;  // mm/s
unsigned long tiempoUltimoConteo = 0;
bool estadoAnteriorUni = HIGH;

// Firebase en segundo plano
void enviarAFirebase(void *parameter) {
  for (;;) {
    if (Firebase.ready()) {
      Serial.println("Enviando datos a Firebase...");
      FirebaseJson json;

      json.set("contador", cont);
      json.set("sensor_digital", hallValue);
      json.set("sensor_lineal", valorAnalogico);
      json.set("sensor_unipolar", hallUni);
      json.set("velocidad_pwm", velocidadPWM);
      json.set("velocidad_media", velocidadMedia);
      json.set("velocidad_lineal", velocidadLineal);

      if (!Firebase.RTDB.setJSON(&fbdo, "/sensores", &json)) {
        Serial.println("Error al subir JSON: " + fbdo.errorReason());
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Espera 1 segundo
  }
}

// Conexión WiFi
void setup_WIFI() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Conectado con IP: ");
  Serial.println(WiFi.localIP());
}

// Configuración Firebase
void setupFirebase() {
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectNetwork(true);
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
  config.timeout.serverResponse = 10 * 1000;
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
}

// Configuración general
void setup() {
  Serial.begin(115200);

  pinMode(hallSensorPin, INPUT);
  pinMode(hallSensorUni, INPUT);
  pinMode(hallSensorAnalog, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // PWM para motor
  ledcSetup(pwmChannelIN1, pwmFreq, pwmResolution);
  ledcSetup(pwmChannelIN2, pwmFreq, pwmResolution);
  ledcAttachPin(motorIN1, pwmChannelIN1);
  ledcAttachPin(motorIN2, pwmChannelIN2);

  setup_WIFI();
  setupFirebase();

  // Crear tarea secundaria para Firebase
  xTaskCreatePinnedToCore(
    enviarAFirebase,      // Función de la tarea
    "FirebaseTask",        // Nombre de la tarea
    10000,                 // Stack size
    NULL,                  // Parámetro
    1,                     // Prioridad
    NULL,                  // Handle
    1                      // Núcleo del ESP32
  );
}

// Loop principal
void loop() {
  hallValue = digitalRead(hallSensorPin);
  hallUni = digitalRead(hallSensorUni);
  valorAnalogico = analogRead(hallSensorAnalog);

  velocidadPWM = map(valorAnalogico, 0, 4095, 0, 255);
  velocidadPWM = constrain(velocidadPWM, 0, 255);

  int PWM_MINIMO = 220; // Mínimo para el motor, de 250 sería el máximo a subir, pero no se nota la diferencia
  if (velocidadPWM < PWM_MINIMO) {
    velocidadPWM = PWM_MINIMO;
  }

  // Contador unipolar
  if (hallUni == LOW && estadoAnteriorUni == HIGH) {
    cont++;
    unsigned long tiempoActual = millis();
    if (tiempoUltimoConteo > 0) {
      float tiempoEntrePulsos = tiempoActual - tiempoUltimoConteo;  // ms
      if (tiempoEntrePulsos > 0) {
        velocidadMedia = 1000.0 / tiempoEntrePulsos;  // Hz
        velocidadLineal = velocidadMedia * 3.1416 * 35.0;  // mm/s
      }
    }
    tiempoUltimoConteo = tiempoActual;
  }
  estadoAnteriorUni = hallUni;

  // Apagar velocidad si no hay pulsos en 1 segundo
  unsigned long tiempoActual = millis();
  if (tiempoUltimoConteo > 0 && tiempoActual - tiempoUltimoConteo > 1000) {
    velocidadMedia = 0;
    velocidadLineal = 0;
  }


  // Mover motor según hallValue
  if (hallValue == 1) {
    ledcWrite(pwmChannelIN1, velocidadPWM);
    ledcWrite(pwmChannelIN2, 0);
    digitalWrite(ledPin, HIGH);
    Serial.println("→ Giro sentido 1");
  } else {
    ledcWrite(pwmChannelIN1, 0);
    ledcWrite(pwmChannelIN2, velocidadPWM);
    digitalWrite(ledPin, LOW);
    Serial.println("→ Giro sentido 2");
  }

  float diametroRuedaMM = 35.0;  // mm
  float perimetro = 3.1416 * diametroRuedaMM;  // mm
  velocidadLineal = velocidadMedia * perimetro;  // mm/s

  // Mostrar datos
  Serial.print("Sensor digital: "); Serial.print(hallValue);
  Serial.print(" | Sensor lineal: "); Serial.print(valorAnalogico);
  Serial.print(" | Sensor unipolar: "); Serial.print(hallUni);
  Serial.print(" - Cont: "); Serial.print(cont);
  Serial.print(" | PWM: "); Serial.print(velocidadPWM);
  Serial.print(" | Velocidad media: "); Serial.println(velocidadMedia);Serial.print(" Hz");
  Serial.print(" | Velocidad lineal: "); Serial.println(velocidadLineal);Serial.print(" mm/s");
  delay(50);
}


/*
const int hallSensorPin = 25;
const int hallSensorAnalog = 2;
const int hallSensorUni = 27;
const int ledPin = 26;

// Pines del motor
const int motorIN1 = 13;
const int motorIN2 = 5;

const int pwmChannelIN1 = 0;
const int pwmChannelIN2 = 1;
const int pwmFreq = 5000;
const int pwmResolution = 8;

int cont = 0;
bool estadoAnteriorUni = HIGH;

void setup() {
  pinMode(hallSensorPin, INPUT);
  pinMode(hallSensorUni, INPUT);
  pinMode(ledPin, OUTPUT);

  // Configurar PWM en ESP32
  ledcSetup(pwmChannelIN1, pwmFreq, pwmResolution);
  ledcSetup(pwmChannelIN2, pwmFreq, pwmResolution);

  ledcAttachPin(motorIN1, pwmChannelIN1);
  ledcAttachPin(motorIN2, pwmChannelIN2);

  Serial.begin(115200);
}

void loop() {
  int hallValue = digitalRead(hallSensorPin);
  int hallUni = digitalRead(hallSensorUni);
  int valorAnalogico = analogRead(hallSensorAnalog);

  int velocidadPWM = map(valorAnalogico, 0, 4095, 0, 255);
  velocidadPWM = constrain(velocidadPWM, 0, 255);

  int PWM_MINIMO = 220; // Mínimo para el motor, de 250 sería el máximo a subir, pero no se nota la diferencia
  if (velocidadPWM < PWM_MINIMO) {
    velocidadPWM = PWM_MINIMO;
  }
  
  if (hallUni == LOW && estadoAnteriorUni == HIGH) {
    cont++;
  }
  estadoAnteriorUni = hallUni;

  // Imprimir datos
  Serial.print("Sensor digital: ");
  Serial.print(hallValue);
  Serial.print(" | Sensor lineal: ");
  Serial.print(valorAnalogico);
  Serial.print(" | Sensor unipolar: ");
  Serial.print(hallUni);
  Serial.print(" - ");
  Serial.print(cont);
  Serial.print(" | PWM: ");
  Serial.println(velocidadPWM);

  if (hallValue == 1) {
    // Giro en un sentido
    ledcWrite(pwmChannelIN1, velocidadPWM);
    ledcWrite(pwmChannelIN2, 0);
    digitalWrite(ledPin, HIGH);
    Serial.println("→ Giro sentido 1");
  } else {
    // Giro en el otro sentido
    ledcWrite(pwmChannelIN1, 0);
    ledcWrite(pwmChannelIN2, velocidadPWM);
    digitalWrite(ledPin, LOW);
    Serial.println("→ Giro sentido 2");
  }

  delay(100);
}

const int hallSensorPin = 25;
const int ledPin = 26;

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  int hallValue = analogRead(hallSensorPin); 
  
  int pwmValue = map(hallValue, 2970, 4095, 0, 255);

  analogWrite(ledPin, pwmValue);
  Serial.print("Valor sensor: ");
  Serial.print(hallValue);
  Serial.print(" => Intensidad LED: ");
  Serial.println(pwmValue);

  delay(100);
}

const int hallSensorPin = 25;
const int ledPin = 26; 
void setup() {
  pinMode(hallSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  int hallValue = digitalRead(hallSensorPin);

  if (hallValue == 1) {
    digitalWrite(ledPin, HIGH);
    Serial.println("¡Imán no detectado!");
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("¡Imán detectado!");
  }

  delay(100);
}


const int hallSensorPin = 25;
const int ledPin = 26;

// Pines del motor
const int motorIN1 = 13;
const int motorIN2 = 5;
const int motorNSLEEP = 27;
void setup() {
  pinMode(hallSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);

  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  pinMode(motorNSLEEP, OUTPUT);
  digitalWrite(motorNSLEEP, HIGH);

  Serial.begin(115200);
}

void loop() {
  int hallValue = digitalRead(hallSensorPin);

  if (hallValue == 1) {
    digitalWrite(motorIN1, HIGH);
    digitalWrite(motorIN2, LOW);

    digitalWrite(ledPin, HIGH);
    Serial.println("¡Imán no detectado! → Giro sentido 1");
  } else {
    digitalWrite(motorIN1, LOW);
    digitalWrite(motorIN2, HIGH);

    digitalWrite(ledPin, LOW);
    Serial.println("¡Imán detectado! → Giro sentido 2");
  }

  delay(100);
}


const int hallSensorPin = 25;
const int ledPin = 26;

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  int hallValue = analogRead(hallSensorPin); 
  
  int pwmValue = map(hallValue, 2970, 4095, 0, 255);

  analogWrite(ledPin, pwmValue);
  Serial.print("Valor sensor: ");
  Serial.print(hallValue);
  Serial.print(" => Intensidad LED: ");
  Serial.println(pwmValue);

  delay(100);
}
*/