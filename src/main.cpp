#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <math.h>

//Motion_test -> Arrays of points for testing the motion functions
float X_test_calibration[] = {0.0,  0.0,  0.0,  0.0,  5.0, 10.0, 15.0, 0.0, 0.0};
float Y_test_calibration[] = {0.0,  0.0,  0.0,  0.0,  0.0, 0.0,  0.0,  5.0, 15.0};          // -> teste de movimento linear dos 3 eixos
float Z_test_calibration[] = {35.0, 40.0, 50.0, 60.0, 60.0, 60.0, 60.0, 60.0, 60.0};
const int N_test_calibration = 9;
float X_test[] = {15.607,  14.923,  13.995,  12.845,  11.503, 10.000,  6.665,  1.464,  -2.965,  -5.000, -5.659, -6.056, -6.180, -6.029, -5.607, -4.923, -3.995, -2.845, -1.503,  0.000,   3.335,  8.536,  12.965,  15.000, 15.659, 16.056, 16.180, 16.029};
float Y_test[] = {  0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,    0.0};
//float Y_test[] = {15.607,  16.029,  16.180,  16.056,  15.659, 15.000, 12.965,  8.536,   3.335,   0.000, -1.503, -2.845, -3.995, -4.923, -5.607, -6.029, -6.180, -6.056, -5.659, -5.000,  -2.965,  1.464,   6.665,  10.000, 11.503, 12.845, 13.995, 14.923};
float Z_test[] = {15.607,  16.029,  16.180,  16.056,  15.659, 15.000, 12.965,  8.536,   3.335,   0.000, -1.503, -2.845, -3.995, -4.923, -5.607, -6.029, -6.180, -6.056, -5.659, -5.000,  -2.965,  1.464,   6.665,  10.000, 11.503, 12.845, 13.995, 14.923};
//float Z_test[] = {  40.0,    40.0,    40.0,    40.0,    40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,   40.0,    40.0};

int Step_delay_t[] = {0, 0,  0,  0,   0,   0,    0,    0,  0};
const int N_test = 28;
float T_period = 1.0f; //Duration of the cycle in seconds (for now, not used for anything)

float X_test2[] = {15.607,  14.923,  13.995,  12.845,  11.503, 10.000,  6.665,  1.464,  -2.965,  -5.000, -5.659, -6.056, -6.180, -6.029, -5.607, -4.923, -3.995, -2.845, -1.503,  0.000,   3.335,  8.536,  12.965,  15.000, 15.659, 16.056, 16.180, 16.029};
float Y_test2[] = {  0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,    0.0};
float Z_test2[] = {15.607,  16.029,  16.180,  16.056,  15.659, 15.000, 12.965,  8.536,   3.335,   0.000, -1.503, -2.845, -3.995, -4.923, -5.607, -6.029, -6.180, -6.056, -5.659, -5.000,  -2.965,  1.464,   6.665,  10.000, 11.503, 12.845, 13.995, 14.923};
const int N_test2 = 28;
float T_period2 = 4.5f; //Duration of the cycle in seconds (for now, not used for anything)

//Link lengths (mm)
#define L1 35
#define L2 60
#define L3 15

#define END_EFFECTOR_Z_OFFSET 0
#define SERVO_OFFSET_X 32
#define SERVO_OFFSET_Y 0
#define SERVO_OFFSET_Z (0.0 + END_EFFECTOR_Z_OFFSET)
//#define SERVO_OFFSET_Z_INVERTED -293

#define SERVO_ANGLE_MIN 0.78539816339744830961566084581988f //45 degrees
#define SERVO_ANGLE_MAX 3.9269908169872415480783042290994f //225 degrees

#define ROTATION_OFFSET_Z (-75.0f * PI / 180.0f) //0.26179938779914943f (15° em radianos = π/12):

#define ROTATION_OFFSET_Z_Delta1 (-75.0f * PI / 180.0f)
#define ROTATION_OFFSET_Z_Delta2 (-45.0f * PI / 180.0f)

#define MIN 'm'
#define MAX 'M'

//Servo microsecond pulse limits (Calibration)-------Valores normais: Min = 500 ; Max = 2500]--------------------------
#define SERVO_1_MIN 620
#define SERVO_1_MAX 2520
#define SERVO_2_MIN 615
#define SERVO_2_MAX 2480
#define SERVO_3_MIN 620
#define SERVO_3_MAX 2550
  //Servo_1 -> thetta3 min 550, max 2450
  //Servo_2 -> thetta1 min 545, max 2410
  //Servo_3 -> thetta2 min 520, max 2450

  // variáveis globais para valores suavizados
int servo_1_pulse_smooth = 0;
int servo_2_pulse_smooth = 0;
int servo_3_pulse_smooth = 0;

// fator de suavização [0 - 1]]. tipo 0.2 para começar
const float SERVO_SMOOTH_ALPHA = 0.2f;



#define SERVO_4_MIN 550
#define SERVO_4_MAX 2400
#define SERVO_5_MIN 550
#define SERVO_5_MAX 2400
#define SERVO_6_MIN 550
#define SERVO_6_MAX 2400

  // variáveis globais para valores suavizados
int servo_4_pulse_smooth = 0;
int servo_5_pulse_smooth = 0;
int servo_6_pulse_smooth = 0;
//-----------------------------------------------------------------------------------------

#define INVERTED -1

// Máximo de pontos por movimento (ajusta mais tarde se precisares)
const int MAX_POINTS = 100;

// Armazenamento Bruto: dados completos de um movimento - Será utilizada para armazenar os movimentos que o Pi enviar, ou movimentos pré-definidos, etc.
struct MotionStorage {
    int   nPoints;                   // nº de pontos válidos em X/Y/Z/easing
    float X[MAX_POINTS];            // coordenadas X dos pontos do movimento
    float Y[MAX_POINTS];            // coordenadas Y dos pontos do movimento
    float Z[MAX_POINTS];            // coordenadas Z dos pontos do movimento
    byte  easing[MAX_POINTS];      // 0=linear (para já) uint8_t é o mesmo que byte, ou seja, um número entre 0 e 255  -> 
                                 //      -> será utilizado para indicar o tipo de interpolação a usar entre os pontos (linear, ease-in, ease-out, etc.)
                                 //      -> Basicamente o tipo de suavimento que queremos entre os pontos.
    float period;                    // duração do ciclo em segundos
    // no futuro podes pôr mais meta-informação aqui (tipo ID, nome, etc.)
};

// Armazenamento temporário: runtime que executa um MotionStorage
struct MotionInstance {
    MotionStorage* def;         // ponteiro para MotionStorage (a definição do Armazenamento Bruto )
    int            currentIndex;// índice i (início do segmento i -> i+1) -> índice atual no movimento
    unsigned int   lastStepMs;  // (já não vamos usar muito, podes deixar para debug!!) // última vez que avançou de ponto
                                                                                        //    -> é uma variavel importante para controlar a velocidade do movimento, ou seja, quando é que deve avançar para o próximo ponto do movimento
    unsigned int   segmentStartMs; // timestamp do início do segmento atual
                                   //    -> é uma variavel importante para controlar a velocidade do movimento, ou seja, quando é que deve avançar para o próximo ponto do movimento
    bool           active;      // se este movimento está ativo
};



// ------------------Temporario---------------------------------
MotionStorage Test_move; //Declaração de objetos para utilizar com as structures de movimento

void initTestMove() { //Serve para copiar os valores definidos mais acima para o objeto Test_move, que é do tipo MotionStorage. 
                        // ->    Isto é só para facilitar a criação de movimentos de teste, ou seja, para não ter que copiar os valores manualmente para o Test_move cada vez que quiseres testar algo.
                        // ->    No futuro será utilizado para copiar os movimentos pré definidos da EPPROM ou do PI
    Test_move.nPoints = N_test;
    Test_move.period  = T_period;

    for (int i = 0; i < N_test; i++) {
        Test_move.X[i] = X_test[i];
        Test_move.Y[i] = -1 * Y_test[i];
        Test_move.Z[i] = 65 + Z_test[i];
        Test_move.easing[i] = 0;
    }
}


MotionInstance Test_inst;

void initTestInstance() { //Serve para arrancar a structure de temporaria de runtime
    Test_inst.def           = &Test_move;
    Test_inst.currentIndex  = 0;
    Test_inst.lastStepMs    = 0;
    Test_inst.segmentStartMs = 0; //millis(); // ou 0, e tratamos no update
    Test_inst.active        = true;
}

//-------------------Fim Temporario---------------------------------

struct Coordinate_f {
    float x;
    float y;
    float z;
};


const float stepXYZ = 1.0F;


// trigonometric constants
const float sqrt3 = sqrt(3.0);
const float pi = 3.141592653;
const float sin120 = sqrt3 / 2.0;
const float cos120 = -0.5;
const float tan60 = sqrt3;
const float sin30 = 0.5;
const float tan30 = 1.0 / sqrt3;


//---------Declaração das variaveis------------------
void LED_LOOP(); //Declaração de funções
void Servo_test();
//-------Novas-------------
float boundFloat(float, float, float);
void attach_servos(void);
bool inverse_kinematics_1(float, float, float, float, float&);
bool inverse_kinematics_2(float, float, float, float, float&);
bool inverse_kinematics_3(float, float, float, float, float&);
bool inverse_kinematics(float, float, float, float, int&, int&, int&);
void move_servos(void);
double mapNumber(double x, double in_min, double in_max, double out_min, double out_max);
int roundMapNumber(double x, double in_min, double in_max, double out_min, double out_max);
double degToRads(double deg);
double radsToDeg(double rads);
void updateMotion(MotionInstance& inst, unsigned long nowMs);
void debugPrintTestMove();
void debugPrintTestInstance();
float lerp(float a, float b, float t);
float applyEasing(float alpha, char mode);
float spline3(float p0, float p1, float p2, float t);
//----------------------------------------------------

//Delta1 --------------------------------
Servo mg90s_1;  // cria objeto servo
Servo mg90s_2;  // cria objeto servo
Servo mg90s_3;  // cria objeto servo

float servo_1_angle;
float servo_2_angle;
float servo_3_angle;

int servo_1_pulse_count = 0;
int servo_2_pulse_count = 0;
int servo_3_pulse_count = 0;

    
//Delta2 --------------------------------
Servo mg90s_4;  // cria objeto servo
Servo mg90s_5;  // cria objeto servo        
Servo mg90s_6;  // cria objeto servo

float servo_4_angle;
float servo_5_angle;
float servo_6_angle;


int servo_4_pulse_count = 0;
int servo_5_pulse_count = 0;
int servo_6_pulse_count = 0;

Coordinate_f end_effector;
Coordinate_f home_position;



byte axis_direction = 0; 
float servo_offset_z = SERVO_OFFSET_Z;

//-----------------Declaração da localização dos pinos para cada objeto ------------------
//-------------------Servos-----------------
#define SERVO_PIN_1 21 //19 //Servo_1 -> thetta3
#define SERVO_PIN_2 47 //20 //Servo_2 -> thetta1
#define SERVO_PIN_3 35  //21 //Servo_3 -> thetta2

#define SERVO_PIN_4 36  //22 // Servo_4 -> thetta3
#define SERVO_PIN_5 37  //23 // Servo_5 -> thetta1
#define SERVO_PIN_6 38  //24 // Servo_6 -> thetta2

//----------------Led_Informação------------
#define LED_PIN 48
#define LED_COUNT 1


//------------------------------------------------------------------------------------------
bool stepDirection = false;
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);



void setup() {

  Serial.begin(115200);
  
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
  mg90s_1.attach(SERVO_PIN_1, SERVO_1_MIN, SERVO_1_MAX);  

  mg90s_2.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_2.attach(SERVO_PIN_2, SERVO_2_MIN, SERVO_2_MAX);  

  mg90s_3.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_3.attach(SERVO_PIN_3, SERVO_3_MIN, SERVO_3_MAX); 


  mg90s_4.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_4.attach(SERVO_PIN_4, 500, 2460);  // Servo_4 -> thetta3 //2460

  mg90s_5.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_5.attach(SERVO_PIN_5, 500, 2470);  // Servo_5 -> thetta1 ->2490

  mg90s_6.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_6.attach(SERVO_PIN_6, 500, 2450);  // Servo_6 -> thetta2 -2450
  delay(1000);

  Serial.println("Delta robot kinematics ready");

  //----------------Inicialização do movimento de teste-----------------
    initTestMove();
    initTestInstance();

    debugPrintTestMove();     //Função para imprimir o conteúdo do Test_move (definição do movimento)
    debugPrintTestInstance(); //Função para imprimir o estado atual do Test_inst (instância do movimento, ou seja, onde está no ciclo, etc.)
    

    Serial.println("Setup completo, pronto para correr updateMotion.");
}


void loop() {

  float theta1, theta2, theta3;
    float x, y, z;
  x = 0.0;    //minimo 0.0
  y = 0.0;   //minimo 0.0
  z = 35.0;   //minimo 61.2

    unsigned long now = millis();
    //printf("\n Valor de now = %d", now);
    //debugPrintTestMove();     //Função para imprimir o conteúdo do Test_move (definição do movimento)
    //debugPrintTestInstance(); //Função para imprimir o estado atual do Test_inst (instância do movimento, ou seja, onde está no ciclo, etc.)
    // LED_LOOP();

    // Atualizar movimentos ativos
    updateMotion(Test_inst, now); //Variavel que controla o update do movimento conforme o tempo que tenha passado, a posição atual no movimento, etc.
    // updateMotion(respiracaoEsquerdaInst, now);
    // updateMotion(batimentoCardiacoInst, now);
    // updateMotion(tosseInst, now);

    // Aqui no futuro: leitura de comandos do Pi, etc.




  
  //LED_LOOP();
  
  //Servo_test();
  
   
//------------test de cinematica só com inverse_kinematics--------------
 /*
for (int i = 0; i < 9; i++) {
    //bool verification = inverse_kinematics(X_test_calibration[i], Y_test_calibration[i], Z_test_calibration[i], ROTATION_OFFSET_Z_Delta2);
  //if(verification == 1){
   // printf("\n Success Inverse Kinematics");
   // printf("\n Angulo do servo 1: %f", servo_1_angle);
   // printf("\n Angulo do servo 2: %f", servo_2_angle);
   // printf("\n Angulo do servo 3: %f", servo_3_angle);
 // }
  //if(verification == 0){
   // printf("\n Erro Inverse Kinematics");
  //}
  inverse_kinematics(X_test_calibration[i], Y_test_calibration[i], Z_test_calibration[i], ROTATION_OFFSET_Z_Delta1, servo_1_pulse_count, servo_2_pulse_count, servo_3_pulse_count);
  mg90s_1.writeMicroseconds(servo_1_pulse_count); 
  mg90s_2.writeMicroseconds(servo_2_pulse_count);
  mg90s_3.writeMicroseconds(servo_3_pulse_count);
  delay(50);
  inverse_kinematics(X_test_calibration[i], Y_test_calibration[i], Z_test_calibration[i], ROTATION_OFFSET_Z_Delta2, servo_4_pulse_count, servo_5_pulse_count, servo_6_pulse_count);
  mg90s_4.writeMicroseconds(servo_4_pulse_count); 
  mg90s_5.writeMicroseconds(servo_5_pulse_count);
  mg90s_6.writeMicroseconds(servo_6_pulse_count);
  delay(2000);
  }
  */

}


//---------------------------Funções---------------------------------------------
float boundFloat(float value, float lower, float upper){
    if(value < lower){
        value = lower;
    }
    else if(value > upper){
        value = upper;
    }
    return value;
}




/*------------------------------------------------------------------------------------------------------------------------------------------------------*/

void attach_servos(void){ //Serve apenas para definir os valores de PWM maixos e minimos, estes foram calibrados mais em cima no codigo 
    mg90s_1.attach(SERVO_PIN_1, SERVO_1_MIN, SERVO_1_MAX);
    mg90s_2.attach(SERVO_PIN_2, SERVO_2_MIN, SERVO_2_MAX);
    mg90s_3.attach(SERVO_PIN_3, SERVO_3_MIN, SERVO_3_MAX);
}

/*
bool inverse_kinematics_1(float xt, float yt, float zt){
    //printf("\n x= %f, y=%f, z=%f", xt, yt, zt);
    zt -= servo_offset_z; //Remove the differance in height from ground level to the centre of rotation of the servos
    float x = xt;
    float y = yt;
    xt = x * cos(ROTATION_OFFSET_Z) - y * sin(ROTATION_OFFSET_Z);
    yt = x * sin(ROTATION_OFFSET_Z) + y * cos(ROTATION_OFFSET_Z);

    float arm_end_x = xt + L3; //Adding the distance between the end effector centre and ball joints to the target x coordinate
    //printf("\n Arm_end_x= %f", arm_end_x);
    float l2p = sqrt(pow(L2, 2) - pow(yt, 2)); //The length of link 2 when projected onto the XZ plane
    //printf("\n l2p= %f", l2p);
    
    float l2pAngle = asin(yt / L2); //Gives the angle between link2 and the ball joints. (Not actually necessary to calculate the inverse kinematics. Just used to prevent the arms ripping themselves apart.)
    //printf("\n rad l2pAngle= %f", l2pAngle);
    //printf("\n rad l2pAngle= %f", radsToDeg(l2pAngle));
    if(!(abs(l2pAngle) < 0.59341194567807205615405486128613f)){ //Prevents the angle between the ball joints and link 2 (L2) going out of range. (Angle was determined by emprical testing.)
        //printf("ERROR: Ball joint 1 out of range: l2pAngle = %f", radsToDeg(l2pAngle));
        return false;
    }

    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2)); //Extension of the arm from the centre of the servo rotation to the end ball joint of link2

    if(ext <= l2p - L1 || ext >= L1 + l2p){ //Checks the extension in the reachable range (This limit assumes that L2 is greater than L1)
       //printf("\n ERROR: Extension 1 out of range: ext = %f", ext);
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext)); //Cosine rule that calculates the angle between the ext line and L1
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x); //Calculates the angle between horizontal (X) the ext line with respect to its quadrant
    float theta = phi + omega; //Theta is the angle between horizontal (X) and L1

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){ //Checks the angle is in the reachable range
        //printf("\n ERROR: Servo angle 1 out of range: Angle = %f", radsToDeg(theta));
        return false;
    }
    //printf("\n servo_1_angle = %f", radsToDeg(theta));
    servo_1_angle = theta;
    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------

bool inverse_kinematics_2(float xt, float yt, float zt){
    zt -= servo_offset_z;
    float x = xt;
    float y = yt;
    float angle = 2.0943951023931954923084289221863f + ROTATION_OFFSET_Z; // 120° + offset
    xt = x * cos(angle) - y * sin(angle);
    yt = x * sin(angle) + y * cos(angle);
    //xt = x * cos(2.0943951023931954923084289221863f) - y * sin(2.0943951023931954923084289221863f); //Rotate coordinate frame 120 degrees
    //yt = x * sin(2.0943951023931954923084289221863f) + y * cos(2.0943951023931954923084289221863f);
    
    float arm_end_x = xt + L3;
    float l2p = sqrt(pow(L2, 2) - pow(yt, 2));
    
    float l2pAngle = asin(yt / L2);
    if(!(abs(l2pAngle) < 0.59341194567807205615405486128613f)){ //Prevents the angle between the ball joints and link 2 (L2) going out of range.
        //printf("ERROR: Ball joint 2 out of range: l2pAngle = ", radsToDeg(l2pAngle));        
        return false;
    }
    
    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2));

    if(ext <= l2p - L1 || ext >= L1 + l2p){ //This limit assumes that L2 is greater than L1
        //printf("ERROR: Extension 2 out of range: ext = ", ext);
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext));
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x);
    float theta = phi + omega;

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){
        //printf("ERROR: Servo angle 2 out of range: Angle = ", radsToDeg(theta));
        return false;
    }
    //printf("\n servo_2_angle = %f", radsToDeg(theta));
    servo_2_angle = theta;
    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------

bool inverse_kinematics_3(float xt, float yt, float zt){
    zt -= servo_offset_z;

    float x = xt;
    float y = yt;
    float angle = 4.1887902047863909846168578443727f + ROTATION_OFFSET_Z; // 120° + offset
    xt = x * cos(angle) - y * sin(angle);
    yt = x * sin(angle) + y * cos(angle);
    //xt = x * cos(4.1887902047863909846168578443727f) - y * sin(4.1887902047863909846168578443727f); //Rotate coordinate frame 240 degrees
    //yt = x * sin(4.1887902047863909846168578443727f) + y * cos(4.1887902047863909846168578443727f);

    float arm_end_x = xt + L3;
    float l2p = sqrt(pow(L2, 2) - pow(yt, 2));
    
    float l2pAngle = asin(yt / L2);
    if(!(abs(l2pAngle) < 0.59341194567807205615405486128613f)){ //Prevents the angle between the ball joints and link 2 (L2) going out of range.
        //printf("ERROR: Ball joint 1 out of range: l2pAngle = ", radsToDeg(l2pAngle));
        return false;
    }
    
    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2));

    if(ext <= l2p - L1 || ext >= L1 + l2p){ //This limit assumes that L2 is greater than L1
        //printf("ERROR: Extension 3 out of range: ext = ", ext);
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext));
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x);
    float theta = phi + omega;

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){
        //printf("ERROR: Servo angle 3 out of range: Angle = ", radsToDeg(theta));
        return false;
    }
    //printf("\n servo_3_angle = %f", radsToDeg(theta));
    servo_3_angle = theta;
    return true;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------

bool inverse_kinematics(float xt, float yt, float zt){    
    if(axis_direction == 1){//if axis are inverted
        xt = -xt;
        zt = -zt;
    }
    
    if(inverse_kinematics_1(xt, yt, zt) && inverse_kinematics_2(xt, yt, zt) && inverse_kinematics_3(xt, yt, zt)){ //Calculates and checks the positions are valid.
        if(axis_direction == 1){//if axis are inverted
            end_effector.x = -xt;
            end_effector.z = -zt;
        }
        else{
            end_effector.x = xt;
            end_effector.z = zt;
        }
        end_effector.y = yt;
        
        servo_1_pulse_count = round(mapNumber(servo_1_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, SERVO_1_MAX, SERVO_1_MIN));
        servo_2_pulse_count = round(mapNumber(servo_2_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, SERVO_2_MAX, SERVO_2_MIN));
        servo_3_pulse_count = round(mapNumber(servo_3_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, SERVO_3_MAX, SERVO_3_MIN));

        return true;
    }
    return false;
}
*/
bool inverse_kinematics(float xt, float yt, float zt, float rotation_offset_Z, int& servo_Theta1_pulse, int& servo_Theta2_pulse, int& servo_Theta3_pulse){    
    float servo_Theta1_angle; 
    float servo_Theta2_angle;
    float servo_Theta3_angle;
    if(axis_direction == 1){//if axis are inverted
        xt = -xt;
        zt = -zt;
    }
    
    if(inverse_kinematics_1(xt, yt, zt, rotation_offset_Z, servo_Theta1_angle) && inverse_kinematics_2(xt, yt, zt, rotation_offset_Z, servo_Theta2_angle) && inverse_kinematics_3(xt, yt, zt, rotation_offset_Z, servo_Theta3_angle)){ 
       
        
        servo_Theta1_pulse = round(mapNumber(servo_Theta1_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, SERVO_1_MAX, SERVO_1_MIN));
        servo_Theta2_pulse = round(mapNumber(servo_Theta2_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, SERVO_2_MAX, SERVO_2_MIN));
        servo_Theta3_pulse = round(mapNumber(servo_Theta3_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, SERVO_3_MAX, SERVO_3_MIN));

        return true;
    }
    return false;
}


bool inverse_kinematics_1(float xt, float yt, float zt, float rotation_offset_Z, float& Servo_angle){
    zt -= servo_offset_z; 
    float x = xt;
    float y = yt;
    xt = x * cos(rotation_offset_Z) - y * sin(rotation_offset_Z);
    yt = x * sin(rotation_offset_Z) + y * cos(rotation_offset_Z);

    float arm_end_x = xt + L3; 
    //printf("\n Arm_end_x= %f", arm_end_x);
    float l2p = sqrt(pow(L2, 2) - pow(yt, 2)); 
    //printf("\n l2p= %f", l2p);
    
    float l2pAngle = asin(yt / L2); 
    //printf("\n rad l2pAngle= %f", radsToDeg(l2pAngle));
    if(!(abs(l2pAngle) < 0.59341194567807205615405486128613f)){ 
        //printf("ERROR: Ball joint 1 out of range: l2pAngle = %f", radsToDeg(l2pAngle));
        return false;
    }

    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2)); 
    if(ext <= l2p - L1 || ext >= L1 + l2p){ 
       //printf("\n ERROR: Extension 1 out of range: ext = %f", ext);
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext)); 
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x); 
    float theta = phi + omega; 

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){ 
        //printf("\n ERROR: Servo angle 1 out of range: Angle = %f", radsToDeg(theta));
        return false;
    }
    printf("\n servo_1_angle = %f", radsToDeg(theta));
    //servo_1_angle = theta;
    Servo_angle=theta;
    return true;
}

bool inverse_kinematics_2(float xt, float yt, float zt, float rotation_offset_Z, float& Servo_angle){
    zt -= servo_offset_z;
    float x = xt;
    float y = yt;
    float angle = 2.0943951023931954923084289221863f + rotation_offset_Z; // 120° + offset
    xt = x * cos(angle) - y * sin(angle);
    yt = x * sin(angle) + y * cos(angle);
    
    float arm_end_x = xt + L3;
    float l2p = sqrt(pow(L2, 2) - pow(yt, 2));
    
    float l2pAngle = asin(yt / L2);
    if(!(abs(l2pAngle) < 0.59341194567807205615405486128613f)){ 
        //printf("ERROR: Ball joint 2 out of range: l2pAngle = ", radsToDeg(l2pAngle));        
        return false;
    }
    
    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2));

    if(ext <= l2p - L1 || ext >= L1 + l2p){ 
        //printf("ERROR: Extension 2 out of range: ext = ", ext);
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext));
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x);
    float theta = phi + omega;

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){
        //printf("ERROR: Servo angle 2 out of range: Angle = ", radsToDeg(theta));
        return false;
    }
    printf("\n servo_2_angle = %f", radsToDeg(theta));
    //servo_2_angle = theta;
    Servo_angle=theta;
    return true;
}

bool inverse_kinematics_3(float xt, float yt, float zt, float rotation_offset_Z, float& Servo_angle){
    zt -= servo_offset_z;

    float x = xt;
    float y = yt;
    float angle = 4.1887902047863909846168578443727f + rotation_offset_Z; // 120° + offset
    xt = x * cos(angle) - y * sin(angle);
    yt = x * sin(angle) + y * cos(angle);
    //xt = x * cos(4.1887902047863909846168578443727f) - y * sin(4.1887902047863909846168578443727f); //Rotate coordinate frame 240 degrees
    //yt = x * sin(4.1887902047863909846168578443727f) + y * cos(4.1887902047863909846168578443727f);

    float arm_end_x = xt + L3;
    float l2p = sqrt(pow(L2, 2) - pow(yt, 2));
    
    float l2pAngle = asin(yt / L2);
    if(!(abs(l2pAngle) < 0.59341194567807205615405486128613f)){ //Prevents the angle between the ball joints and link 2 (L2) going out of range.
        //printf("ERROR: Ball joint 1 out of range: l2pAngle = ", radsToDeg(l2pAngle));
        return false;
    }
    
    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2));

    if(ext <= l2p - L1 || ext >= L1 + l2p){ //This limit assumes that L2 is greater than L1
        //printf("ERROR: Extension 3 out of range: ext = ", ext);
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext));
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x);
    float theta = phi + omega;

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){
        //printf("ERROR: Servo angle 3 out of range: Angle = ", radsToDeg(theta));
        return false;
    }
    printf("\n servo_3_angle = %f", radsToDeg(theta));
    //servo_3_angle = theta;
    Servo_angle=theta;
    return true;
}

 //Funçaõ simples para atualizar o valor de PWM dos servos.
void move_servos(void){
    mg90s_1.writeMicroseconds(servo_1_pulse_count);
    mg90s_2.writeMicroseconds(servo_2_pulse_count);
    mg90s_3.writeMicroseconds(servo_3_pulse_count);

    mg90s_4.writeMicroseconds(servo_4_pulse_count);
    mg90s_5.writeMicroseconds(servo_5_pulse_count);
    mg90s_6.writeMicroseconds(servo_6_pulse_count);
}

/*
void move_servos(void){ //Função V1 de atualização do PWM dos Servos com um filtro de suavização
    // primeira vez, inicia suavizados nos atuais
    static bool initialized = false;
    if (!initialized) {
        servo_1_pulse_smooth = servo_1_pulse_count;
        servo_2_pulse_smooth = servo_2_pulse_count;
        servo_3_pulse_smooth = servo_3_pulse_count;
        initialized = true;
    }

    // filtro exponencial simples (low-pass) //basicamente faz uma pequena media das ultimas posições para suavizar o movimento, 
    servo_1_pulse_smooth = servo_1_pulse_smooth
                           + SERVO_SMOOTH_ALPHA * (servo_1_pulse_count - servo_1_pulse_smooth);
    servo_2_pulse_smooth = servo_2_pulse_smooth
                           + SERVO_SMOOTH_ALPHA * (servo_2_pulse_count - servo_2_pulse_smooth);
    servo_3_pulse_smooth = servo_3_pulse_smooth
                           + SERVO_SMOOTH_ALPHA * (servo_3_pulse_count - servo_3_pulse_smooth);

    mg90s_1.writeMicroseconds(servo_1_pulse_smooth);
    mg90s_2.writeMicroseconds(servo_2_pulse_smooth);
    mg90s_3.writeMicroseconds(servo_3_pulse_smooth);
}
*/
//-----------------Funções_Uteis_no_Dia_a_Dia------------
double mapNumber(double x, double in_min, double in_max, double out_min, double out_max) {//Remaps a number to a given range
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int roundMapNumber(double x, double in_min, double in_max, double out_min, double out_max) {//Remaps a number to a given range and rounds it
	return round((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

double radsToDeg(double rads) {
	return rads * 180 / PI;
}

double degToRads(double deg) {
	return deg * PI / 180;
}
//------------------------------------------------------

float lerp(float a, float b, float t) { //Interpolação linear entre dois pontos a e b com t em [0,1] -> de momento não está a ser utilizada
    return a + t * (b - a);
}


float spline3(float p0, float p1, float p2, float t) { //Interpolação entre 3 pontos
    // t em [0,1]

    // calculo datangente aproximada com Pi, pi+1 e Pi+2 (basicamente vê os valores futuros e prepara uma linha tangente que passe o mais perto e suavemente de Pi+1)
    float m1 = 0.35f * (p2 - p0);

    float t2 = t * t; //t^2
    float t3 = t2 * t; //t^3

    // funções base (heurísticas simples)
    float h0 = 2.0f * t3 - 3.0f * t2 + 1.0f; // peso de p0
    float h1 = -2.0f * t3 + 3.0f * t2;       // peso de p1
    float h2 = t3 - t2;                      // peso da tangente m1

    float value = h0 * p0 + h1 * p1 + h2 * m1;
    return value;
}

/*
//-----------------Função de controlo de movimento em função do tempo------------------
void updateMotion(MotionInstance& inst, unsigned long nowMs) { //V1 -> versão original
    if (!inst.active || inst.def == nullptr) return; //Verifica se a instância está ativa e se tem uma definição de movimento associada

    MotionStorage& m = *(inst.def);//Cria uma referência para a definição do movimento para facilitar o acesso aos seus dados e a leitura do utilizador

    if (m.nPoints <= 0 || m.period <= 0.0f) return; //Verifica se o movimento tem um número válido de pontos e um período positivo. Se não tiver, não faz nada.

    float dtMs = (m.period * 1000.0f) / (float)m.nPoints; //Calcula o tempo que deve passar entre cada ponto do movimento, (em milissegundos). 
                                                          //   -> O período é dividido pelo número de pontos para obter o tempo por ponto.

    if (nowMs - inst.lastStepMs < dtMs) {
        return; // ainda não é tempo de avançar
    }

    //inst.lastStepMs = nowMs;
    // Em vez de inst.lastStepMs = nowMs;
    inst.lastStepMs += (unsigned long)dtMs; //Atualiza a última vez que avançou para o próximo ponto, somando o tempo por ponto ao último tempo registrado. 
                                    //    -> Isso ajuda a manter um ritmo constante mesmo que haja atrasos ou variações no loop, evitando que o movimento acelere ou desacelere devido a atrasos.

    inst.currentIndex++; //Avança para o próximo ponto do movimento, incrementando o índice atual.
    if (inst.currentIndex >= m.nPoints) { //Caso seja atinjido o final do ciclo, volta ao inicio 
        inst.currentIndex = 0;
    }

    int i = inst.currentIndex; 

    float x = m.X[i];
    float y = m.Y[i];
    float z = m.Z[i];

    bool ok = inverse_kinematics(x, y, z); // é acionado a função inverse kinematics para calcular os ângulos dos servos necessários para alcançar a posição (x, y, z) do ponto atual. 
    if (ok) {
        move_servos(); //Se a cinemática inversa for bem-sucedida, os servos são movidos para as posições calculadas.
    }
}

void updateMotion(MotionInstance& inst, unsigned long nowMs) { //V2 -> versão com interpolação linear entre os pontos (função Easing)
    if (!inst.active || inst.def == nullptr) return;

    MotionStorage& m = *(inst.def);
    if (m.nPoints <= 1 || m.period <= 0.0f) return;

    // tempo total por segmento (i -> i+1)
    float dtMs = (m.period * 1000.0f) / (float)m.nPoints;

    // inicializar segmentStartMs na primeira chamada
    if (inst.segmentStartMs == 0) {
        inst.segmentStartMs = nowMs;
    }

    // quanto tempo passou desde o início do segmento atual
    unsigned long elapsed = nowMs - inst.segmentStartMs;

    // se passou do dt, avançar para o próximo segmento
    if (elapsed >= dtMs) {
        // avançar índice
        inst.currentIndex++;
        if (inst.currentIndex >= m.nPoints) {
            inst.currentIndex = 0;
        }
        // reiniciar segmento
        inst.segmentStartMs = nowMs;
        elapsed = 0;
    }

    // calcular alpha [0,1] dentro do segmento atual
    float alpha = (float)elapsed / dtMs;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    // escolher modo de easing aqui
    //   "C" -> sinusoidal
    //   "L" -> linear
    //   "Q" -> quadratica
    //   "A" -> Ease-out (arranca rápido, abranda no fim)
    //   "S" -> Cubic ease-in-out mais agressivo (S mais vincado)
    char easingMode = 'A';  // 'L', 'C', 'Q', 'A', 'S'
    
    // aplicar easing global
    float a = applyEasing(alpha, easingMode);

    int i  = inst.currentIndex;
    int j  = (i + 1) % m.nPoints; // próximo ponto (ciclo)

    // interpolação suave entre ponto i e ponto j
    float x = lerp(m.X[i], m.X[j], a);
    float y = lerp(m.Y[i], m.Y[j], a);
    float z = lerp(m.Z[i], m.Z[j], a);

    bool ok = inverse_kinematics(x, y, z);
    if (ok) {
        move_servos();
    }
}
*/

void updateMotion(MotionInstance& inst, unsigned long nowMs) { //V3 -> versão com interpolação por spline cúbica entre os pontos (função Spline)
    if (!inst.active || inst.def == nullptr) return; //Verifica se a instância está ativa e se tem uma definição de movimento associada

    MotionStorage& m = *(inst.def); //Cria uma referência para a definição do movimento para facilitar o acesso aos seus dados e a leitura do utilizador
    if (m.nPoints <= 2 || m.period <= 0.0f) return;  //Verifica se o ciclo tem um número válido de pontos (pelo menos 3) e um período positivo. Se não tiver, sai da função

    
    float dtMs = (m.period * 1000.0f) / (float)m.nPoints; //Calcula o tempo que deve passar entre cada ponto do movimento (i -> i+1), (em milissegundos). 
                                                          //   -> O período é dividido pelo número de pontos para obter o tempo por ponto.

    // inicializar segmentStartMs na primeira chamada, para o valor atual de millis() 
    if (inst.segmentStartMs == 0) {
        inst.segmentStartMs = nowMs;
    }

    // quanto tempo passou desde o início do segmento atual
    unsigned long elapsed = nowMs - inst.segmentStartMs;

    // se passou do dt, avançar para o próximo segmento
    if (elapsed >= dtMs) { //Se já tiver passado o tempo dt avança para o proximo segmento 
        inst.currentIndex++;
        if (inst.currentIndex >= m.nPoints) { //caso tenha chegado ao final do ciclo, volta ao inicio
            inst.currentIndex = 0;
        }
        inst.segmentStartMs = nowMs; //dá reset ao temporizador interno 
        elapsed = 0; 
    }

    // calcular alpha [0,1] dentro do segmento atual
    float alpha = (float)elapsed / dtMs; //basicamente isto calcula um valor interno entre 0 e 1 (entre Pi e Pi+1) que será utilizando para  calcula a interpolação no spline
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

  
    float a = alpha; //pode ser utilizado no Easing ou no spline
    // Caso seja utilizado o easing novamente:
    // char easingMode = 'C'; // 'L','C','Q','A','S'
    // float a = applyEasing(alpha, easingMode);

    // índices dos 3 pontos: Pi, Pi+1, Pi+2 (com wrap circular)
    int i0 = inst.currentIndex;                      // Pi
    int i1 = (inst.currentIndex + 1) % m.nPoints;    // Pi+1
    int i2 = (inst.currentIndex + 2) % m.nPoints;    // Pi+2

    // calculo do spline por eixo
    float x = spline3(m.X[i0], m.X[i1], m.X[i2], a);
    float y = spline3(m.Y[i0], m.Y[i1], m.Y[i2], a);
    float z = spline3(m.Z[i0], m.Z[i1], m.Z[i2], a);

    bool ok1 = inverse_kinematics(x, y, z, ROTATION_OFFSET_Z_Delta1, servo_1_pulse_count, servo_2_pulse_count, servo_3_pulse_count);
    bool ok2 = inverse_kinematics(x, y, z, ROTATION_OFFSET_Z_Delta2, servo_4_pulse_count, servo_5_pulse_count, servo_6_pulse_count);
    if (ok1 || ok2 ) { //|| ok2
        move_servos();
    }
}


void debugPrintTestMove() {
    Serial.println("=== Test_move contents ===");
    Serial.print("nPoints: ");
    Serial.println(Test_move.nPoints);
    Serial.print("period (sec): ");
    Serial.println(Test_move.period);

    for (int i = 0; i < Test_move.nPoints; i++) {
        Serial.print("i=");
        Serial.print(i);
        Serial.print("  X=");
        Serial.print(Test_move.X[i]);
        Serial.print("  Y=");
        Serial.print(Test_move.Y[i]);
        Serial.print("  Z=");
        Serial.print(Test_move.Z[i]);
        Serial.print("  easing=");
        Serial.println(Test_move.easing[i]);
    }
}


void debugPrintTestInstance() {
    Serial.println("=== Test_inst state ===");
    Serial.print("active: ");
    Serial.println(Test_inst.active ? "true" : "false");

    Serial.print("currentIndex: ");
    Serial.println(Test_inst.currentIndex);

    Serial.print("lastStepMs: ");
    Serial.println(Test_inst.lastStepMs);

    Serial.print("def pointer: ");
    Serial.println((uintptr_t)Test_inst.def, HEX); // endereço em hex
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


// easing do arranque do servo motor: //de momento não está a ser utilizada. É algo parecido ao funcionamento de ramp.h ou Easing_servo.h
float applyEasing(float alpha, char mode) {
 
//   "C" -> sinusoidal
//   "L" -> linear
//   "Q" -> quadratica
//   "A" -> Ease-out (arranca rápido, abranda no fim)
//   "S" -> Cubic ease-in-out mais agressivo (S mais vincado)

    // clamp
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    switch (mode) {
        case 'L':  // Linear
            return alpha;

        case 'C':  // Sinusoidal ease-in-out
            // começa devagar, acelera a meio, abranda no fim
            return 0.5f - 0.5f * cosf(pi * alpha);

        case 'Q':  // Quadrática ease-in (arranca devagar, acelera)
            return alpha * alpha;

        case 'A':  // Quadrática ease-out (arranca rápido, abranda)
            return 1.0f - (1.0f - alpha) * (1.0f - alpha);

        case 'S':  // Cubic ease-in-out mais agressivo (S mais vincado)
            if (alpha < 0.5f) {
                // primeira metade
                return 4.0f * alpha * alpha * alpha;
            } else {
                // segunda metade
                float t = 2.0f * alpha - 2.0f;
                return 0.5f * t * t * t + 1.0f;
            }

        default:   // fallback: comporta-se como linear
            return alpha;
    }
}
//------------------------------------------------------
