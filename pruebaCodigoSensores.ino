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


/*
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