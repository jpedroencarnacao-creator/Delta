#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <math.h>

// joytsick
#define BTN_UP     4
#define BTN_DOWN   5
#define BTN_LEFT   6
#define BTN_RIGHT  7
#define BTN_MIDLE  8
#define BTN_SET    9
#define BTN_RESET  10

//float posX = 0.0F;
//float posY = 0.0F;
//float posZ = 62.0F;

float posX[] = {0.0,  0.0,   0.0,    0.0,   0.0,    0.0,      0.0,     0.0,  30.0};
float posY[] = {0.0,  0.0,   0.0,    0.0,   30.0,   60.0,   -30.0,   -60.0,   0.0};
float posZ[] = {70.0, 90.0,  110.0, 130.0,  90.0,   90.0,    90.0,    90.0,   90.0};

const float stepXYZ = 1.0F;

// robot geometry
const float e = 34.64;
const float f = 73.9;
const float re = 90.0;
const float rf = 50.0;

// trigonometric constants
const float sqrt3 = sqrt(3.0);
const float pi = 3.141592653;
const float sin120 = sqrt3 / 2.0;
const float cos120 = -0.5;
const float tan60 = sqrt3;
const float sin30 = 0.5;
const float tan30 = 1.0 / sqrt3;

int delta_calcForward(float theta1, float theta2, float theta3, float &x0, float &y0, float &z0);
int delta_calcAngleYZ(float x0, float y0, float z0, float &theta);
int delta_calcInverse(float x0, float y0, float z0, float &theta1, float &theta2, float &theta3);
float deltakinematic(float posX, float posY, float posZ, char servo);
void readJoystickButtons();
void LED_LOOP(); //Declaração de funções
void Servo_test();

Servo mg90s_1;  // cria objeto servo
Servo mg90s_2;  // cria objeto servo
Servo mg90s_3;  // cria objeto servo

#define SERVO_PIN_1 19
#define SERVO_PIN_2 20 
#define SERVO_PIN_3 21  
#define LED_COUNT 1

// Muda aqui o pino que queres testar
#define LED_PIN 38

// === STEPPER SIMPLES ===
//#define STEP_PIN 19
//#define DIR_PIN  21
//#define ENABLE_PIN 22

bool stepDirection = false;

Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  

  pinMode(BTN_UP, INPUT);
pinMode(BTN_DOWN, INPUT);
pinMode(BTN_LEFT, INPUT);
pinMode(BTN_RIGHT, INPUT);
pinMode(BTN_MIDLE, INPUT);
pinMode(BTN_SET, INPUT);
pinMode(BTN_RESET, INPUT);


  Serial.println("Teste LED RGB");

  pixel.begin();
  pixel.clear();
  pixel.show();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  Serial.println("Teste MG90S Servo");
  // Configurações específicas para servo MG90S (50Hz, pulsos 500-2400us)
  mg90s_1.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_1.attach(SERVO_PIN_1, 500, 2400);  // min=500us, max=2400us

  mg90s_2.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_2.attach(SERVO_PIN_2, 500, 2400);  // min=500us, max=2400us

  mg90s_3.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_3.attach(SERVO_PIN_3, 500, 2400);  // min=500us, max=2400us
  
  mg90s_1.write(90);  // posição inicial central
  mg90s_2.write(90);  // posição inicial central
  mg90s_3.write(90);  // posição inicial central
  delay(1000);
  Serial.println("Delta robot kinematics ready");
/*
  // Stepper
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);  // Ativa
  
  Serial.println("Pronto!");
  */
}


void loop() {

  float theta1, theta2, theta3;
    float x, y, z;

  //Serial.println("Servo -> 0°");
  //mg90s_1.write(5);
  //mg90s_2.write(25);
  //mg90s_3.write(45);
  //readJoystickButtons();
  LED_LOOP();
      //mg90s_1.write(10); //theta3 
      //mg90s_2.write(35);  //theta1
      //mg90s_3.write(55); //theta2
  //Servo_test();
  x = 0;
  y = -20;
  z = 80;   //minimo 61.2

  for (int i = 0; i < 9; i++) {
  float servoAngle_a = deltakinematic(posX[i], posY[i], posZ[i], 'A');
  float servoAngle_b = deltakinematic(posX[i], posY[i], posZ[i], 'B');
  float servoAngle_c = deltakinematic(posX[i], posY[i], posZ[i], 'C');

  Serial.print("X = "); Serial.print(posX[i]);
  Serial.print(" | Y = "); Serial.print(posY[i]);
  Serial.print(" | Z = "); Serial.println(posZ[i]);

  Serial.print("theta1 = "); Serial.println(servoAngle_a);
  Serial.print("theta2 = "); Serial.println(servoAngle_b);
  Serial.print("theta3 = "); Serial.println(servoAngle_c);

  mg90s_1.write(servoAngle_c);
  mg90s_2.write(servoAngle_a);
  mg90s_3.write(servoAngle_b);

  delay(500);
}
  /*
  float  servoAngle_a = deltakinematic(posX, posY, posZ, 'A');
  float  servoAngle_b = deltakinematic(posX, posY, posZ, 'B');
  float  servoAngle_c = deltakinematic(posX, posY, posZ, 'C');
  Serial.print("X = "); Serial.print(posX);
  Serial.print(" | Y = "); Serial.print(posY);
  Serial.print(" | Z = "); Serial.println(posZ);
        Serial.print("theta1 = "); Serial.println(servoAngle_a);
        Serial.print("theta2 = "); Serial.println(servoAngle_b);
        Serial.print("theta3 = "); Serial.println(servoAngle_c);


        mg90s_1.write(servoAngle_c);
     mg90s_2.write(servoAngle_a);
     mg90s_3.write(servoAngle_b);
  
  int statusInv = delta_calcInverse(40.0, 4<0.0, -89.0, theta1, theta2, theta3);

    if (statusInv == 0) {
        Serial.println("Inverse OK");
        Serial.print("theta1 = "); Serial.println(theta1);
        Serial.print("theta2 = "); Serial.println(theta2);
        Serial.print("theta3 = "); Serial.println(theta3);
      //mg90s_1.write(theta3);
     // mg90s_2.write(theta1);
     // mg90s_3.write(theta2);
        //int statusFwd = delta_calcForward(theta1, theta2, theta3, x, y, z);

        //if (statusFwd == 0) {
        //    Serial.println("Forward OK");
         //   Serial.print("x = "); Serial.println(x);
        //    Serial.print("y = "); Serial.println(y);
        //    Serial.print("z = "); Serial.println(z);
        //}
         //else {
         //   Serial.println("Forward error");
        //}
    } else {
        Serial.println("Point outside workspace");
    }
*/
    delay(100);

  /*
  // === STEPPER 200 PASSOS ===
  Serial.print("Stepper ");
  Serial.print(stepDirection ? "FRENTE" : "TRÁS");
  Serial.println(" 200 passos");
  
  digitalWrite(DIR_PIN, stepDirection);
  digitalWrite(ENABLE_PIN, LOW);
  
  for(int i = 0; i < 200; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(1000);  // Velocidade
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(1000);
  }
  
  digitalWrite(ENABLE_PIN, HIGH);  // Desativa
  stepDirection = !stepDirection;
  delay(500);
  */
}


void LED_LOOP(){
// Loop LED
  
  pixel.setPixelColor(0, pixel.Color(255, 0, 0));
  pixel.show();
  Serial.println("verde");
  
  delay(500);

  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();
  Serial.println("Desligado");

  delay(500);
  // Verde
  pixel.setPixelColor(0, pixel.Color(255, 255, 0));
  pixel.show();
  Serial.println("Amarelo");
  delay(500);

  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();
  Serial.println("Desligado");
  delay(500);

  pixel.setPixelColor(0, pixel.Color(0, 255, 0));
  pixel.show();
  Serial.println("vermelho");
  delay(500);

  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();
  Serial.println("Desligado");
  delay(500);

  // Azul
  pixel.setPixelColor(0, pixel.Color(0, 0, 255));
  pixel.show();
  Serial.println("azul");
  delay(500);

  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();
  Serial.println("Desligado");
  delay(500);

  // Azul
  pixel.setPixelColor(0, pixel.Color(255, 255, 255));
  pixel.show();
  Serial.println("branco");
  delay(500);

  // Desligado
  pixel.clear();
  pixel.show();
}

void Servo_test(){
  // === TESTE SERVO ===
  Serial.println("Servo -> 0°");
  mg90s_1.write(5);
  mg90s_2.write(5);
  mg90s_3.write(0);
  delay(1000);
  
  Serial.println("Servo -> 180°");
  mg90s_1.write(50);
  mg90s_2.write(50);
  mg90s_3.write(45);
  delay(1000);

  Serial.println("Servo -> 180°");
  mg90s_1.write(5);
  mg90s_2.write(5);
  mg90s_3.write(0);
  delay(1000);

  for(int i = 5; i < 70; i++) {
    mg90s_1.write(i+5);
  mg90s_2.write(i+5);
  mg90s_3.write(i);
  delay(20);
  i=i+2;
  }
  delay(100);
  for(int i = 70; i > 20; i--) {
    mg90s_1.write(i);
  mg90s_2.write(i);
  mg90s_3.write(i);
  delay(20);
  i=i-2;
 
  }
  mg90s_1.write(20);
  mg90s_2.write(20);
  mg90s_3.write(20);
  for(int i = 20; i < 70; i++) {
    mg90s_1.write(i);
  //mg90s_2.write(i);
  //mg90s_3.write(i);
  delay(20);
  i=i+2;
  }
  mg90s_1.write(20);
  mg90s_2.write(20);
  mg90s_3.write(20);
  for(int i = 20; i < 70; i++) {
  //mg90s_1.write(i);
  mg90s_2.write(i);
  //mg90s_3.write(i);
  delay(20);
  i=i+2;
  }
  mg90s_1.write(20);
  mg90s_2.write(20);
  mg90s_3.write(20);
  for(int i = 20; i < 70; i++) {
  //mg90s_1.write(i);
  //mg90s_2.write(i);
  mg90s_3.write(i);
  delay(20);
  i=i+2;
  }
}

float deltakinematic(float posX, float posY, float posZ, char servo)
{
    float length_a = 31.0F;
    float length_b = 50.0F;
    float length_c = 90.0F;
    float length_d = 15.0F;

    float x = 0.0F, y = 0.0F, z = 0.0F;
    float pi120 = 120.0F * (PI / 180.0F);
    float pi240 = 240.0F * (PI / 180.0F);

    if (servo == 'A')
    {
        x = posX;
        y = posY;
        z = posZ;
    }

    if (servo == 'B')
    {
        x =  (cos(pi120)*(posX)) + (sin(pi120)*(posY));
        y = -(sin(pi120)*(posX)) + (cos(pi120)*(posY));
        z = posZ;
    }

    if (servo == 'C')
    {
        x =  (cos(pi240)*(posX)) + (sin(pi240)*(posY));
        y = -(sin(pi240)*(posX)) + (cos(pi240)*(posY));
        z = posZ;
    }

    float length1 = (length_a - length_d - y);

    float alpha = (360.0F / (2.0F * PI)) * (atan2(z, length1));

    float length2 = sqrt(pow(length1, 2.0F) + pow(z, 2.0F));

    float lenght3 = sqrt(pow(length_c, 2.0F) - pow(x, 2.0F));

    float beta = (360.0F / (2.0F * PI)) * (acos((pow(lenght3, 2) - pow(length2, 2.0F) - pow(length_b, 2.0F)) / (-2.0F * length2 * length_b)));

    float gamma = 180.0F - alpha - beta;

    return gamma;
}


// forward kinematics
int delta_calcForward(float theta1, float theta2, float theta3, float &x0, float &y0, float &z0) {
    float t = (f - e) * tan30 / 2.0;
    float dtr = pi / 180.0;

    theta1 *= dtr;
    theta2 *= dtr;
    theta3 *= dtr;

    float y1 = -(t + rf * cos(theta1));
    float z1 = -rf * sin(theta1);

    float y2 = (t + rf * cos(theta2)) * sin30;
    float x2 = y2 * tan60;
    float z2 = -rf * sin(theta2);

    float y3 = (t + rf * cos(theta3)) * sin30;
    float x3 = -y3 * tan60;
    float z3 = -rf * sin(theta3);

    float dnm = (y2 - y1) * x3 - (y3 - y1) * x2;

    float w1 = y1 * y1 + z1 * z1;
    float w2 = x2 * x2 + y2 * y2 + z2 * z2;
    float w3 = x3 * x3 + y3 * y3 + z3 * z3;

    float a1 = (z2 - z1) * (y3 - y1) - (z3 - z1) * (y2 - y1);
    float b1 = -((w2 - w1) * (y3 - y1) - (w3 - w1) * (y2 - y1)) / 2.0;

    float a2 = -(z2 - z1) * x3 + (z3 - z1) * x2;
    float b2 = ((w2 - w1) * x3 - (w3 - w1) * x2) / 2.0;

    float a = a1 * a1 + a2 * a2 + dnm * dnm;
    float b = 2.0 * (a1 * b1 + a2 * (b2 - y1 * dnm) - z1 * dnm * dnm);
    float c = (b2 - y1 * dnm) * (b2 - y1 * dnm) + b1 * b1 + dnm * dnm * (z1 * z1 - re * re);

    float d = b * b - 4.0 * a * c;
    if (d < 0) return -1;

    z0 = -0.5 * (b + sqrt(d)) / a;
    x0 = (a1 * z0 + b1) / dnm;
    y0 = (a2 * z0 + b2) / dnm;
    return 0;
}

// helper
int delta_calcAngleYZ(float x0, float y0, float z0, float &theta) {
    float y1 = -0.5 * 0.57735 * f;
    y0 -= 0.5 * 0.57735 * e;

    float a = (x0 * x0 + y0 * y0 + z0 * z0 + rf * rf - re * re - y1 * y1) / (2.0 * z0);
    float b = (y1 - y0) / z0;

    float d = -(a + b * y1) * (a + b * y1) + rf * (b * b * rf + rf);
    if (d < 0) return -1;

    float yj = (y1 - a * b - sqrt(d)) / (b * b + 1.0);
    float zj = a + b * yj;
    theta = 180.0 * atan(-zj / (y1 - yj)) / pi + ((yj > y1) ? 180.0 : 0.0);
    return 0;
}

// inverse kinematics
int delta_calcInverse(float x0, float y0, float z0, float &theta1, float &theta2, float &theta3) {
    theta1 = theta2 = theta3 = 0.0;

    int status = delta_calcAngleYZ(x0, y0, z0, theta1);
    if (status == 0)
        status = delta_calcAngleYZ(x0 * cos120 + y0 * sin120, y0 * cos120 - x0 * sin120, z0, theta2);
    if (status == 0)
        status = delta_calcAngleYZ(x0 * cos120 - y0 * sin120, y0 * cos120 + x0 * sin120, z0, theta3);

    return status;
}




