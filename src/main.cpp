#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <math.h>

// declarar no topo do ficheiro (globais)
int last_delta2_s1 = 0;
int last_delta2_s2 = 0;
int last_delta2_s3 = 0;

struct Manipulador_Config {
    // geometria
    float L1;
    float L2;
    float L3;
    float servoOffsetX;
    float servoOffsetZ;
    float angleMin;
    float angleMax;
    float rotationOffsetZ;   // offset global de rotação da base

    // calibração de PWM
    int servo1MinPulse;
    int servo1MaxPulse;

    int servo2MinPulse;
    int servo2MaxPulse;

    int servo3MinPulse;
    int servo3MaxPulse;

    int theta1MinPulse;
    int theta1MaxPulse;

    int theta2MinPulse;
    int theta2MaxPulse;

    int theta3MinPulse;
    int theta3MaxPulse;

    // estado atual (pulsos gerados pela IK)
    int servo1Pulse;
    int servo2Pulse;
    int servo3Pulse;
};


Manipulador_Config delta_1_Cfg = {
    // geometria
    .L1 = 35.0f,
    .L2 = 60.0f,
    .L3 = 15.0f,
    .servoOffsetX = 32.0f,
    .servoOffsetZ = 0.0f,
    .angleMin = 0.78539816339744830961566084581988f, //45 degrees
    .angleMax = 3.9269908169872415480783042290994f, //225 degrees
    .rotationOffsetZ = -15.6f * PI / 180.0f, //-75 degrees in radians

    // calibração de PWM
    .servo1MinPulse = 520,
    .servo1MaxPulse = 2520,

    .servo2MinPulse = 550,
    .servo2MaxPulse = 2550,

    .servo3MinPulse = 560,
    .servo3MaxPulse = 2560,

    // estado atual (pulsos gerados pela IK)
    .servo1Pulse = 0,
    .servo2Pulse = 0,
    .servo3Pulse = 0
};

Manipulador_Config delta_2_Cfg = {
    // geometria
    .L1 = 35.0f,
    .L2 = 60.0f,
    .L3 = 15.0f,
    .servoOffsetX = 32.0f,
    .servoOffsetZ = 0.0f,
    .angleMin = 0.78539816339744830961566084581988f, //45 degrees
    .angleMax = 3.9269908169872415480783042290994f, //225 degrees
    .rotationOffsetZ = -44.4f * PI / 180.0f, //-45 degrees in radians

    // calibração de PWM
    .servo1MinPulse = 460,
    .servo1MaxPulse = 2460,

    .servo2MinPulse = 470,
    .servo2MaxPulse = 2470,

    .servo3MinPulse = 450,
    .servo3MaxPulse = 2450,

    // estado atual (pulsos gerados pela IK)
    .servo1Pulse = 0,
    .servo2Pulse = 0,
    .servo3Pulse = 0
};

//Motion_test -> Arrays of points for testing the motion functions
float X_test_calibration[] = {0.0,  0.0,  0.0,  0.0,  5.0, 10.0, 15.0, 0.0, 0.0};
float Y_test_calibration[] = {0.0,  0.0,  0.0,  0.0,  0.0, 0.0,  0.0,  5.0, 15.0};          // -> teste de movimento linear dos 3 eixos
float Z_test_calibration[] = {40.0, 40.0, 50.0, 60.0, 60.0, 60.0, 60.0, 60.0, 60.0};
const int N_test_calibration = 9;
float X_test[] = {15.607,  14.923,  13.995,  12.845,  11.503, 10.000,  6.665,  1.464,  -2.965,  -5.000, -5.659, -6.056, -6.180, -6.029, -5.607, -4.923, -3.995, -2.845, -1.503,  0.000,   3.335,  8.536,  12.965,  15.000, 15.659, 16.056, 16.180, 16.029};
//float Y_test[] = {  0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,    0.0};
float Y_test[] = {15.607,  16.029,  16.180,  16.056,  15.659, 15.000, 12.965,  8.536,   3.335,   0.000, -1.503, -2.845, -3.995, -4.923, -5.607, -6.029, -6.180, -6.056, -5.659, -5.000,  -2.965,  1.464,   6.665,  10.000, 11.503, 12.845, 13.995, 14.923};
//float Z_test[] = {15.607,  16.029,  16.180,  16.056,  15.659, 15.000, 12.965,  8.536,   3.335,   0.000, -1.503, -2.845, -3.995, -4.923, -5.607, -6.029, -6.180, -6.056, -5.659, -5.000,  -2.965,  1.464,   6.665,  10.000, 11.503, 12.845, 13.995, 14.923};
//float Z_test[] = {  40.0,    40.0,    40.0,    40.0,    40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,   40.0,    40.0};
float Z_test[] = {  0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,    0.0};

int Step_delay_t[] = {0, 0,  0,  0,   0,   0,    0,    0,  0};
const int N_test = 28;
float T_period = 0.3f; 
float T_pausedt = 0.6f; 


//float X_test2[] = {0.000, 1.333, 2.667, 4.000, 5.333, 6.667, 8.000, 9.333, 10.667, 12.000, 10.667, 9.333, 8.000, 6.667, 5.333, 4.000, 2.667,   1.333 };
//float Y_test2[] = {  0.0,   0.0,   0.0,    0.0,  0.0,   0.0,   0.0,   0.0,   0.0,     0.0,    0.0,    0.0,   0.0,   0.0,  0.0,    0.0,   0.0,  0.0 };
//float Z_test2[] = {0.000, 0.247, 0.988, 2.222, 3.951, 6.173, 8.889, 12.099, 15.802, 20.000, 15.802, 12.099, 8.889, 6.173, 3.951, 2.222, 0.988, 0.247};
float X_test2[] = {0.000, 1.333, 2.667, 4.000, 5.333, 6.667, 8.000, 9.333, 10.667, 12.000};
float Y_test2[] = {  0.0,   0.0,   0.0,    0.0,  0.0,   0.0,   0.0,   0.0,   0.0,     0.0};
float Z_test2[] = {0.000, 0.247, 0.988, 2.222, 3.951, 6.173, 8.889, 12.099, 15.802, 20.000};

//const int N_test2 = 18;
const int N_test2 = 10;
float T_period2 = 4.0f; 
float T_pausedt2 = 0.8f; 
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
    float period; 
    float T_pause;   
    float dtMs;                // duração do ciclo em segundos
    float dt_pauseMs;           // duração da pausa entre ciclos em segundos

    bool  bidirectional;   // true = vai-e-vem, false = ciclo fechado

    float getDtMs() const {
        if (nPoints > 1 && period > 0.0f) {
            int Ndt_pontos ;  // nº de segmentos por ciclo
            if (bidirectional==true) {
                // 0..N-1..0  -> 2*(N-1) segmentos
                Ndt_pontos  = (nPoints * 2) - 2;
            } else {
                // ciclo normal 0..N-1..0, como já tinhas
                Ndt_pontos = nPoints;
            }
            return (period * 1000.0f) / (float)Ndt_pontos;
        }
        return 0.0f;
    }

    void updateDtMs() {
    dtMs = getDtMs();    // aqui sim, mudas dtMs
    }

  float getD_PauseMs() const {
    if (nPoints > 1 && T_pause > 0.0f) {
        return (T_pause * 1000.0f);  // só lê nPoints e T_pause
        }
       return 0.0f;
    }

    void updateDt_PauseMs() {
    dt_pauseMs = getD_PauseMs();    // aqui sim, mudas dt_pauseMs
  }

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
    short int      state;
    unsigned long elapsed;
    unsigned long elapsed_2;
    unsigned long elapsed_3;

    int iIndex;  // novo: índice i para a interpolação
    int jIndex;  // novo: índice j para a interpolação
    int Direction; // novo: direção do movimento (1 ou -1)
};



// ------------------Temporario---------------------------------
MotionStorage Test_move; //Declaração de objetos para utilizar com as structures de movimento
MotionInstance Test_inst;
MotionStorage Test2_move; //Declaração de objetos para utilizar com as structures de movimento
MotionInstance Test2_inst;

void initTestMove() { //Serve para copiar os valores definidos mais acima para o objeto Test_move, que é do tipo MotionStorage. 
                        // ->    Isto é só para facilitar a criação de movimentos de teste, ou seja, para não ter que copiar os valores manualmente para o Test_move cada vez que quiseres testar algo.
                        // ->    No futuro será utilizado para copiar os movimentos pré definidos da EPPROM ou do PI
    Test_move.nPoints = N_test;
    Test_move.period  = T_period;
    Test_move.T_pause = T_pausedt;
    for (int i = 0; i < N_test; i++) {
        Test_move.X[i] = X_test[i];
        Test_move.Y[i] = -1 * Y_test[i];
        Test_move.Z[i] = Z_test[i];

    }
    Test_move.bidirectional = false; // Define o movimento como vai-e-vem 
    Test_move.updateDtMs();
    Test_move.updateDt_PauseMs();
}

void initTestMove2() { //Serve para copiar os valores definidos mais acima para o objeto Test2_move, que é do tipo MotionStorage. 
                        // ->    Isto é só para facilitar a criação de movimentos de teste, ou seja, para não ter que copiar os valores manualmente para o Test2_move cada vez que quiseres testar algo.
                        // ->    No futuro será utilizado para copiar os movimentos pré definidos da EPPROM ou do PI
    Test2_move.nPoints = N_test2;
    Test2_move.period  = T_period2;
    Test2_move.T_pause = T_pausedt2;
    for (int i = 0; i < N_test2; i++) {
        Test2_move.X[i] = X_test2[i];
        Test2_move.Y[i] = -1 * Y_test2[i];
        Test2_move.Z[i] = Z_test2[i];
    }
    Test2_move.bidirectional = true; // Define o movimento como vai-e-vem
    Test2_move.updateDtMs();
    Test2_move.updateDt_PauseMs();
}
    
void initTestInstance() { //Serve para arrancar a structure de temporaria de runtime
    Test_inst.def           = &Test_move;
    Test_inst.currentIndex  = 0;
    Test_inst.lastStepMs    = 0;
    Test_inst.segmentStartMs = 0; //millis(); // ou 0, e tratamos no update
    Test_inst.active        = true;
    Test_inst.state         = 0;
    Test_inst.elapsed       = 0;
    Test_inst.elapsed_2     = 0;
    Test_inst.elapsed_3     = 0;
    Test_inst.iIndex        = 0;
    Test_inst.jIndex        = 0;
    Test_inst.Direction     = 1; // novo: inicializa a direção como positiva
}

void initTestInstance2() { //Serve para arrancar a structure de temporaria de runtime
    Test2_inst.def           = &Test2_move;
    Test2_inst.currentIndex  = 0;
    Test2_inst.lastStepMs    = 0;
    Test2_inst.segmentStartMs = 0; //millis(); // ou 0, e tratamos no update
    Test2_inst.active        = true;
    Test2_inst.state         = 0;
    Test2_inst.elapsed       = 0;
    Test2_inst.elapsed_2     = 0;
    Test2_inst.elapsed_3     = 0;
    Test2_inst.iIndex        = 0;
    Test2_inst.jIndex        = 0;
    Test2_inst.Direction     = 1; // novo: inicializa a direção como positiva
}



//-------------------Fim Temporario---------------------------------

struct Coordinate {
    float X;
    float Y;
    float Z;
};




int Clock_loop = 0;
const float pi = 3.141592653;



//---------Declaração das variaveis------------------
void LED_LOOP(); //Declaração de funções
void Servo_test();
//-------Novas-------------
float boundFloat(float, float, float);
void attach_servos(void);
bool inverse_kinematics_1(float, float, float, float, float&);
bool inverse_kinematics_2(float, float, float, float, float&);
bool inverse_kinematics_3(float, float, float, float, float&);
bool inverse_kinematics(Manipulador_Config& cfg, float, float, float);
void move_servos(void);
double mapNumber(double x, double in_min, double in_max, double out_min, double out_max);
int roundMapNumber(double x, double in_min, double in_max, double out_min, double out_max);
double degToRads(double deg);
double radsToDeg(double rads);
void debugPrintMove(const MotionStorage& move, const char* name);
void debugPrintInstance(const MotionInstance& inst, const char* name);
float lerp(float a, float b, float t);
float applyEasing(float alpha, char mode);
float spline3(float p0, float p1, float p2, float t);
void Intermed_position(MotionInstance& inst, unsigned long nowMs, Coordinate& xyz, boolean trig_serial);

//------------------FSM-------------------------------
void FSM_Motion_Update(boolean start, MotionInstance& inst, unsigned long nowMs);
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

volatile boolean start_comand = 1;    

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

Coordinate Lerp_Respi_OUT;
Coordinate Lerp_Batim_OUT;
Coordinate IK2_IN;
Coordinate home_position;



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

bool Motion_RESP_Position(MotionInstance& inst, unsigned long nowMs,float& outX, float& outY, float& outZ)
{
    if (!inst.active || inst.def == nullptr) return false;

    MotionStorage& m = *(inst.def);
    if (m.nPoints <= 1 || m.period <= 0.0f) return false;

    if (inst.segmentStartMs == 0) {
        inst.segmentStartMs = nowMs;
    }

    unsigned long elapsed = nowMs - inst.segmentStartMs;

    if (elapsed >= m.dtMs) {
        inst.currentIndex++;
        if (inst.currentIndex >= m.nPoints) {
            inst.currentIndex = 0;
        }
        inst.segmentStartMs = nowMs;
        elapsed = 0;
    }

    // aqui podes escolher:
    // A) usar só o ponto i (sem interpolação)
    // B) interpolar linearmente dentro do segmento

    float alpha = (float)elapsed / m.dtMs;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    int i = inst.currentIndex;
    int j = (i + 1) % m.nPoints;

    outX = lerp(m.X[i], m.X[j], alpha);
    outY = lerp(m.Y[i], m.Y[j], alpha);
    outZ = lerp(m.Z[i], m.Z[j], alpha);

    return true;
}

bool Motion_BAT_Position(MotionInstance& inst, unsigned long nowMs, float& outX, float& outY, float& outZ)
{
    if (!inst.active || inst.def == nullptr) return false;

    MotionStorage& m = *(inst.def);
    if (m.nPoints <= 1 || m.period <= 0.0f) return false;

    if (inst.segmentStartMs == 0) {
        inst.segmentStartMs = nowMs;
    }

    unsigned long elapsed = nowMs - inst.segmentStartMs;

    if (elapsed >= m.dtMs) {
        inst.currentIndex++;
        if (inst.currentIndex >= m.nPoints) {
            inst.currentIndex = 0;
        }
        inst.segmentStartMs = nowMs;
        elapsed = 0;
    }

    float alpha = (float)elapsed / m.dtMs;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    int i = inst.currentIndex;
    int j = (i + 1) % m.nPoints;

    outX = lerp(m.X[i], m.X[j], alpha);
    outY = lerp(m.Y[i], m.Y[j], alpha);
    outZ = lerp(m.Z[i], m.Z[j], alpha);

    return true;
}

boolean trig_display_fsm(float at, short int Td){
static short int state; //state internal to the state machine
static float pt; //var. internal to the state machine
boolean trig; // output
//Serial.println(state);
//Serial.print("AT = ");Serial.println(at);
//Serial.print("pt = ");Serial.println(pt);
//Serial.print("at-pt = ");Serial.println(at-pt);
switch (state) {
case 0:
if (at-pt >= Td) {state = 1;}
trig = 0; // output
return trig;
case 1:
if (1) {state = 0;}
trig = 1; pt = at; // output
return trig;
}
return trig;
}

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

delta_1_Cfg.theta1MinPulse = delta_1_Cfg.servo1MinPulse;
delta_1_Cfg.theta1MaxPulse = delta_1_Cfg.servo1MaxPulse;

delta_1_Cfg.theta2MinPulse = delta_1_Cfg.servo2MinPulse;
delta_1_Cfg.theta2MaxPulse = delta_1_Cfg.servo2MaxPulse;

delta_1_Cfg.theta3MinPulse = delta_1_Cfg.servo3MinPulse;
delta_1_Cfg.theta3MaxPulse = delta_1_Cfg.servo3MaxPulse;

delta_2_Cfg.theta1MinPulse = delta_2_Cfg.servo1MaxPulse;
delta_2_Cfg.theta1MaxPulse = delta_2_Cfg.servo1MinPulse; 

delta_2_Cfg.theta2MinPulse = delta_2_Cfg.servo2MaxPulse;
delta_2_Cfg.theta2MaxPulse = delta_2_Cfg.servo2MinPulse;

delta_2_Cfg.theta3MinPulse = delta_2_Cfg.servo3MaxPulse;
delta_2_Cfg.theta3MaxPulse = delta_2_Cfg.servo3MinPulse;

  Serial.println("Teste MG90S Servo");
  // Configurações específicas para servo MG90S (50Hz, pulsos 500-2400us)
  mg90s_1.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_1.attach(SERVO_PIN_1, delta_1_Cfg.servo1MinPulse, delta_1_Cfg.servo1MaxPulse);  

  mg90s_2.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_2.attach(SERVO_PIN_2, delta_1_Cfg.servo2MinPulse, delta_1_Cfg.servo2MaxPulse);  

  mg90s_3.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_3.attach(SERVO_PIN_3, delta_1_Cfg.servo3MinPulse, delta_1_Cfg.servo3MaxPulse); 


  mg90s_4.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_4.attach(SERVO_PIN_4, delta_2_Cfg.servo1MinPulse, delta_2_Cfg.servo1MaxPulse);  // Servo_4 -> thetta3 //2460

  mg90s_5.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_5.attach(SERVO_PIN_5, delta_2_Cfg.servo2MinPulse, delta_2_Cfg.servo2MaxPulse);  // Servo_5 -> thetta1 ->2490

  mg90s_6.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_6.attach(SERVO_PIN_6, delta_2_Cfg.servo3MinPulse, delta_2_Cfg.servo3MaxPulse);  // Servo_6 -> thetta2 -2450
  delay(1000);

  Serial.println("Delta robot kinematics ready");

  //----------------Inicialização do movimento de teste-----------------
    initTestMove();
    initTestMove2();
    initTestInstance();
    initTestInstance2();

    
    debugPrintMove(Test_move,  "Test_move");
    debugPrintMove(Test2_move, "Test2_move"); //Função para imprimir o estado atual do Test2_inst (instância do movimento, ou seja, onde está no ciclo, etc.)
    debugPrintInstance(Test_inst,  "Test_inst");
    debugPrintInstance(Test2_inst, "Test2_inst");

    Serial.println("Setup completo, pronto para correr updateMotion.");
}

/*
void loop() {
static boolean trig_serial_IN; static boolean trig_serial_OUT;
float actual_time = millis();
float tempo_inicial = millis();

  
    unsigned long now1 = millis();
    float xr=0, yr=0, zr=0;
    float xb=0, yb=0, zb=0;
   // trig_serial_OUT = trig_display_fsm(actual_time,200);
    
    bool okResp = Motion_RESP_Position(Test2_inst,  now1, xr, yr, zr);

    bool okBat  = Motion_BAT_Position(Test_inst, now1, xb, yb, zb);


 //----------para aqui o outro circuito
    if (!okResp && !okBat) {
        return;  // nada válido
    }

    if (okResp) { xr = yr = zr = 0.0f; }
    if (!okBat)  { xb = yb = zb = 0.0f; }

    float x_total = xr + xb + -10.0f;
    float y_total = yr + yb + 0.0f;
    float z_total = zr + zb + 50.0f;

    //printf("\n Total position: (%f, %f, %f)", x_total, y_total, z_total);
    bool ik1 = inverse_kinematics(delta_1_Cfg, x_total, y_total, z_total);
    float Delta1 = millis();
    bool ik2 = inverse_kinematics(delta_2_Cfg, -1 * x_total, y_total, z_total);
    float Delta2 = millis();

    if (ik1) {
        servo_1_pulse_count = delta_1_Cfg.servo1Pulse;
        servo_2_pulse_count = delta_1_Cfg.servo2Pulse;
        servo_3_pulse_count = delta_1_Cfg.servo3Pulse;
    }
    if (ik2) {
        servo_4_pulse_count = delta_2_Cfg.servo1Pulse;
        servo_5_pulse_count = delta_2_Cfg.servo2Pulse;
        servo_6_pulse_count = delta_2_Cfg.servo3Pulse;
    }
    
    if (ik1 || ik2) {
        move_servos();
    }

//----------para aqui o outro circuito





float tempo_final = millis();
float demora = tempo_final - tempo_inicial;
Clock_loop++;
if(trig_serial_IN == 1){

    Serial.print("0º -> Millis():");
    //Serial.println(now1);
    Serial.print("------------------------------------------------------------> Index Atual: ");
    Serial.println(Test_inst.currentIndex);
    //Serial.print("1º -> Tempo de execução do Motion_RESP_Position:");
    //Serial.println(REsp_time);
    //Serial.print("2º ------->  Tempo de execução do Motion_BAT_Position:");
    //Serial.println(BAT_time);
    //Serial.print("3º ------------> Tempo de execução do IKdelta1:");
   // Serial.println(Delta1);
   // Serial.print("4º ------------------->  Tempo de execução IKdelta2:");
    //Serial.println(Delta2);
    Serial.print("Tempo de execução Total: ");
    Serial.println(demora);
    Serial.print("Clock loop: ");
    Serial.println(Clock_loop);
    Serial.print("--------------> dados de posição Y ao entrar no IK: ");
        Serial.print(Lerp_Batim_OUT.Y); 
        Serial.println("");
  }

trig_serial_IN = trig_serial_OUT;

}
*/
/*
void loop() {
static boolean trig_serial_IN; static boolean trig_serial_OUT;
float actual_time = millis();
float tempo_inicial = millis();
/*
    mg90s_1.writeMicroseconds(500);
    mg90s_2.writeMicroseconds(500);
    mg90s_3.writeMicroseconds(500);

   delay(500);
   mg90s_1.writeMicroseconds(2500);
    mg90s_2.writeMicroseconds(2500);
    mg90s_3.writeMicroseconds(2500);
    delay(500);
*/
/*
    for (int i = 0; i < 18; i++) {
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
  //bool ik1 = inverse_kinematics(delta_1_Cfg, X_test_calibration[i], Y_test_calibration[i], Z_test_calibration[i]);
  //mg90s_1.writeMicroseconds(servo_1_pulse_count); 
  //mg90s_2.writeMicroseconds(servo_2_pulse_count);
  //mg90s_3.writeMicroseconds(servo_3_pulse_count);
Serial.print("Teste do ponto: ");   
Serial.print(i);
Serial.print(" -> X: ");
Serial.print(-1 * X_test2[i]);
Serial.print(" Y: ");
Serial.print(Y_test2[i]);
Serial.print(" Z: ");
Serial.println( 40 + Z_test2[i]);
  bool ik2 = inverse_kinematics(delta_2_Cfg, -1 * X_test2[i], Y_test2[i], 40 + Z_test2[i]);
  
  //mg90s_4.writeMicroseconds(servo_4_pulse_count); 
  //mg90s_5.writeMicroseconds(servo_5_pulse_count);
  //mg90s_6.writeMicroseconds(servo_6_pulse_count);
  //if (ik1) {
      //  servo_1_pulse_count = delta_1_Cfg.servo1Pulse;
      //  servo_2_pulse_count = delta_1_Cfg.servo2Pulse;
      //  servo_3_pulse_count = delta_1_Cfg.servo3Pulse;
   // }
    if (ik2) {
        servo_4_pulse_count = delta_2_Cfg.servo1Pulse;
        servo_5_pulse_count = delta_2_Cfg.servo2Pulse;
        servo_6_pulse_count = delta_2_Cfg.servo3Pulse;
    }
    move_servos();
    //if (ik1 || ik2) {
      //  move_servos();
    //}
  delay(500);
  
  }
  
    // Atualizar movimentos ativos
         //updateMotion(Test_inst, now); //Variavel que controla o update do movimento conforme o tempo que tenha passado, a posição atual no movimento, etc.

  //LED_LOOP();
  
  //Servo_test();
  
    unsigned long now1 = millis();
    FSM_Motion_Update(start_comand , Test_inst, now1);
    FSM_Motion_Update(start_comand , Test2_inst, now1);
    float xr=0, yr=0, zr=0;
    float xb=0, yb=0, zb=0;
   // trig_serial_OUT = trig_display_fsm(actual_time,200);
    // Movimento 1 (respiração)Test2_inst
    //bool okResp = Motion_RESP_Position(Test2_inst,  now1, xr, yr, zr);
float REsp_time = millis();
    // Movimento 2 (batimento)
    //bool okBat  = Motion_BAT_Position(Test_inst, now1, xb, yb, zb);
float BAT_time = millis();
float demora_RESP = REsp_time - now1;
float demora_BAT = BAT_time - REsp_time;
/*
 //----------para aqui o outro circuito
    if (!okResp && !okBat) {
        return;  // nada válido
    }

    if (!okResp) { xr = yr = zr = 0.0f; }
    if (!okBat)  { xb = yb = zb = 0.0f; }

    float x_total = xr + xb + -10.0f;
    float y_total = yr + yb + 0.0f;
    float z_total = zr + zb + 50.0f;

    //printf("\n Total position: (%f, %f, %f)", x_total, y_total, z_total);
    bool ik1 = inverse_kinematics(delta_1_Cfg, x_total, y_total, z_total);
    float Delta1 = millis();
    bool ik2 = inverse_kinematics(delta_2_Cfg, -1 * x_total, y_total, z_total);
    float Delta2 = millis();

    if (ik1) {
        servo_1_pulse_count = delta_1_Cfg.servo1Pulse;
        servo_2_pulse_count = delta_1_Cfg.servo2Pulse;
        servo_3_pulse_count = delta_1_Cfg.servo3Pulse;
    }
    if (ik2) {
        servo_4_pulse_count = delta_2_Cfg.servo1Pulse;
        servo_5_pulse_count = delta_2_Cfg.servo2Pulse;
        servo_6_pulse_count = delta_2_Cfg.servo3Pulse;
    }
    
    if (ik1 || ik2) {
        move_servos();
    }
*/
//----------para aqui o outro circuito


/*


unsigned long now2 = millis();
Intermed_position(Test_inst, now2, Lerp_Batim_OUT, trig_serial_IN);
Intermed_position(Test2_inst, now2, Lerp_Respi_OUT, trig_serial_IN);
float X_total = 0 * Lerp_Respi_OUT.X +  Lerp_Batim_OUT.X + 0.0f;
float Y_total = 0 * Lerp_Respi_OUT.Y +  Lerp_Batim_OUT.Y + 0.0f;
float Z_total = 0 * Lerp_Respi_OUT.Z +   Lerp_Batim_OUT.Z + 40.0f;
//float X_total =  10.0f;
//float Y_total =  0.0f;
//float Z_total =  45.0f;
Serial.print(" -> X: ");
Serial.print(Lerp_Respi_OUT.X);
Serial.print(" Y: ");
Serial.print(Lerp_Respi_OUT.Y);
Serial.print(" Z: ");
Serial.println(Lerp_Respi_OUT.Z);
bool ik2 = inverse_kinematics(delta_2_Cfg, -1 * X_total, Y_total, Z_total);
servo_4_pulse_count = delta_2_Cfg.servo1Pulse;
servo_5_pulse_count = delta_2_Cfg.servo2Pulse;
servo_6_pulse_count = delta_2_Cfg.servo3Pulse;
move_servos();

*/

/* //----------para aqui o outro circuito
int i = Test_inst.currentIndex;
int j = Test2_inst.currentIndex;
float x_i=X_test[i];
float y_i=Y_test[i];    
float z_i= Z_test[i];
float x_j=X_test2[j];
float y_j=Y_test2[j];    
float z_j= Z_test2[j];
float x_total = x_i + x_j + 0.0f;
float y_total = y_i + y_j + 0.0f;
float z_total = z_i + z_j + 50.0f;
bool ik2 = inverse_kinematics(delta_2_Cfg, -1 * x_total, -1* y_total, z_total);

servo_4_pulse_count = delta_2_Cfg.servo1Pulse;
servo_5_pulse_count = delta_2_Cfg.servo2Pulse;
servo_6_pulse_count = delta_2_Cfg.servo3Pulse;
move_servos();
 
//----------para aqui o outro circuito



float tempo_final = millis();
float demora = tempo_final - tempo_inicial;
Clock_loop++;
if(trig_serial_IN == 1){

    Serial.print("0º -> Millis():");
    //Serial.println(now1);
    Serial.print("------------------------------------------------------------> Index Atual: ");
    Serial.println(Test_inst.currentIndex);
    //Serial.print("1º -> Tempo de execução do Motion_RESP_Position:");
    //Serial.println(REsp_time);
    //Serial.print("2º ------->  Tempo de execução do Motion_BAT_Position:");
    //Serial.println(BAT_time);
    //Serial.print("3º ------------> Tempo de execução do IKdelta1:");
   // Serial.println(Delta1);
   // Serial.print("4º ------------------->  Tempo de execução IKdelta2:");
    //Serial.println(Delta2);
    Serial.print("Tempo de execução Total: ");
    Serial.println(demora);
    Serial.print("Clock loop: ");
    Serial.println(Clock_loop);
    Serial.print("--------------> dados de posição Y ao entrar no IK: ");
        Serial.print(Lerp_Batim_OUT.Y); 
        Serial.println("");
  }

trig_serial_IN = trig_serial_OUT;

}
*/
int pulses_1, pulses_2, pulses_3;
void loop() {
static boolean trig_serial_IN; static boolean trig_serial_OUT;
float actual_time = millis();
float tempo_inicial = millis();

   

  
    unsigned long now1 = millis();

    trig_serial_OUT = trig_display_fsm(actual_time,100);
    FSM_Motion_Update(start_comand , Test_inst, now1);
    unsigned long now2 = millis();
    Intermed_position(Test_inst, now2, Lerp_Batim_OUT, trig_serial_IN);
    FSM_Motion_Update(start_comand , Test2_inst, now1);
    Intermed_position(Test2_inst, now2, Lerp_Respi_OUT, trig_serial_IN);

float X_total_d1 = 1 * Lerp_Respi_OUT.X + 0.3 * Lerp_Batim_OUT.X + -15.0f;
float Y_total_d1 = 1 * Lerp_Respi_OUT.Y + 0.3 * Lerp_Batim_OUT.Y + -10.0f;
float Z_total_d1 = 1 * Lerp_Respi_OUT.Z + 0.3 *  Lerp_Batim_OUT.Z + 50.0f;

float X_total_d2 = 1 * Lerp_Respi_OUT.X + 0.0 * Lerp_Batim_OUT.X + -15.0f;
float Y_total_d2 = 1 * Lerp_Respi_OUT.Y + 0.0 * Lerp_Batim_OUT.Y + -10.0f;
float Z_total_d2 = 1 * Lerp_Respi_OUT.Z + 0.0 *  Lerp_Batim_OUT.Z + 50.0f;

int i = Test_inst.currentIndex;
int j = Test2_inst.currentIndex;
float x_i=X_test[i];
float y_i=Y_test[i];    
float z_i= Z_test[i];
float x_j=X_test2[j];
float y_j=Y_test2[j];    
float z_j= Z_test2[j];
//float x_total =  x_i +0* x_j + 0.0f;
float y_t =  y_i +0* y_j + 0.0f;
//float y_total = -1 * y_t; 
//float z_total =  z_i +0* z_j + 50.0f;


bool ik1 = inverse_kinematics(delta_1_Cfg, X_total_d1, Y_total_d1, Z_total_d1);
bool ik2 = inverse_kinematics(delta_2_Cfg, -1 * X_total_d2, Y_total_d2, Z_total_d2);
servo_1_pulse_count = delta_1_Cfg.servo1Pulse;
servo_2_pulse_count = delta_1_Cfg.servo2Pulse;
servo_3_pulse_count = delta_1_Cfg.servo3Pulse;

servo_4_pulse_count = delta_2_Cfg.servo1Pulse;
servo_5_pulse_count = delta_2_Cfg.servo2Pulse;
servo_6_pulse_count = delta_2_Cfg.servo3Pulse;
move_servos();


float tempo_final = millis();
float demora = tempo_final - tempo_inicial;
Clock_loop++;
//Serial.print("Clock loop: ");
   // Serial.println(Clock_loop);
int Safe_comand = 0;
if(trig_serial_IN == 1 && Safe_comand == 1){
   // Serial.print("------------------------------------------------------------> Index Atual:"); Serial.print(Test_inst.currentIndex);
    //Serial.print("  X_total="); Serial.print(X_total);
    //Serial.print("  Y_total="); Serial.print(Y_total);
    //Serial.print("  Z_total="); Serial.println(Z_total);
    int pulse = pulses_1 - pulses_2;
    Serial.print(" ---- Pulse 1: "); Serial.println(pulses_1);
    Serial.print(" -------------- Pulse 2: "); Serial.println(pulses_2);
    Serial.print(" ------------------------- Pulse servo 4: "); Serial.println(pulse);
    //Serial.print("0º -> Millis():");
    //Serial.println(now1);
    Serial.print("-------------------------------------------------------posição pulso:");
    Serial.println(servo_4_pulse_count);
    //Serial.print("1º -> Tempo de execução do Motion_RESP_Position:");
    //Serial.println(REsp_time);
    //Serial.print("2º ------->  Tempo de execução do Motion_BAT_Position:");
    //Serial.println(BAT_time);
    //Serial.print("3º ------------> Tempo de execução do IKdelta1:");
   // Serial.println(Delta1);
   // Serial.print("4º ------------------->  Tempo de execução IKdelta2:");
    //Serial.println(Delta2);
    //Serial.print("Tempo de execução Total: ");
    //Serial.println(demora);
    //Serial.print("Clock loop: ");
    //Serial.println(Clock_loop);
    
  }



trig_serial_IN = trig_serial_OUT;


/*
for (int i = 500; i < 2500; i++) {
    mg90s_1.writeMicroseconds(i);
    Serial.print("pwm: ");
    Serial.println(i);
    delay(5);
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

bool inverse_kinematics(Manipulador_Config& cfg, float xt, float yt, float zt){    
    float servo_Theta1_angle; 
    float servo_Theta2_angle;
    float servo_Theta3_angle;
    
    if(axis_direction == 1){//if axis are inverted
        xt = -xt;
        zt = -zt;
    }
  
    bool ok1 = inverse_kinematics_1(xt, yt, zt, cfg.rotationOffsetZ, servo_Theta1_angle);
    bool ok2 = inverse_kinematics_2(xt, yt, zt, cfg.rotationOffsetZ, servo_Theta2_angle);
    bool ok3 = inverse_kinematics_3(xt, yt, zt, cfg.rotationOffsetZ, servo_Theta3_angle);
    
     if (!(ok1 && ok2 && ok3)) {
        return false;
    }
       
    cfg.servo1Pulse = round(mapNumber(servo_Theta1_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, cfg.theta1MinPulse, cfg.theta1MaxPulse));
    cfg.servo2Pulse = round(mapNumber(servo_Theta2_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, cfg.theta2MinPulse, cfg.theta2MaxPulse));
    cfg.servo3Pulse = round(mapNumber(servo_Theta3_angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, cfg.theta3MinPulse, cfg.theta3MaxPulse));
    int new4 = cfg.servo1Pulse;
    int new5 = cfg.servo2Pulse;
    int new6 = cfg.servo3Pulse;


    if (new4 != last_delta2_s1) {
    //Serial.print("-----------------------------------------------------------D2: ");
    //Serial.print(last_delta2_s1); Serial.print("->"); Serial.print(new4);
    //Serial.println("");
    

    // atualizar “anteriores”
    last_delta2_s1 = new4;
    
}

    // só se houve mesmo alteração é que consideramos “novo”


    return true;
}


bool inverse_kinematics_1(float xt, float yt, float zt, float rotation_offset_Z, float& Servo_angle){
      //Serial.print("x:");
       // Serial.print(xt);
       // Serial.print(" y:");
       // Serial.print(yt);
       // Serial.print(" z:");
       // Serial.print(zt);
       //  Serial.println("");
       //  delay (100);
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
        printf("ERROR: Ball joint 1 out of range: l2pAngle = %f", radsToDeg(l2pAngle));
        Serial.println("");
        Serial.print("x:");
    Serial.print(x);
        Serial.print(" y:");
        Serial.print(y);
        Serial.print(" z:");
        Serial.print(zt);
        return false;
    }

    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2)); 
    if(ext <= l2p - L1 || ext >= L1 + l2p){ 
        printf("\n ERROR: Extension 1 out of range: ext = %f", ext);
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext)); 
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x); 
    float theta = phi + omega; 

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){ 
        printf("\n ERROR: Servo angle 1 out of range: Angle = %f", radsToDeg(theta));
        return false;
    }
    //printf("\n servo_1_angle = %f", radsToDeg(theta));
    //servo_1_angle = theta;
    Servo_angle=theta;
    return true;
}

bool inverse_kinematics_2(float xt, float yt, float zt, float rotation_offset_Z, float& Servo_angle){
    
      float xn = xt;
    float yn = yt;
    float zn = zt;
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
        //printf("ERROR: Ball joint 2 out of range: l2pAngle = %f", radsToDeg(l2pAngle));
         Serial.print("ERROR: Ball joint 2 out of range: l2pAngle =");
        Serial.print(radsToDeg(l2pAngle));
        Serial.println("");
        Serial.print(radsToDeg(0.59341194567807205615405486128613f));
        Serial.println("");
        Serial.print("x:");
        Serial.print(xn);
        Serial.print(" y:");
        Serial.print(yn);
        Serial.print(" z:");
        Serial.print(zn);
        Serial.println("");
        return false;
    }
    
    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2));

    if(ext <= l2p - L1 || ext >= L1 + l2p){ 
       // printf("ERROR: Extension 2 out of range: ext = ", ext);
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext));
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x);
    float theta = phi + omega;

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){
       // printf("ERROR: Servo angle 2 out of range: Angle = ", radsToDeg(theta));
        return false;
    }
    //printf("\n servo_2_angle = %f", radsToDeg(theta));
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
        printf("ERROR: Ball joint 3 out of range: l2pAngle = ", radsToDeg(l2pAngle));
        //Serial.println("");
        //Serial.print("x:");
        //Serial.print(x);
        //Serial.print(" y:");
        //Serial.print(y);
        //Serial.print(" z:");
        //Serial.print(zt);
       // Serial.println("");
        return false;
    }
    
    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2));

    if(ext <= l2p - L1 || ext >= L1 + l2p){ //This limit assumes that L2 is greater than L1
        //printf("ERROR: Extension 3 out of range: ext = ", ext);
        //Serial.println("");
        return false;
    }
       
    float phi = acos((pow(L1, 2) + pow(ext, 2) - pow(l2p, 2)) / (2 * L1 * ext));
    float omega = atan2(zt, SERVO_OFFSET_X - arm_end_x);
    float theta = phi + omega;

    if(!(theta >= SERVO_ANGLE_MIN && theta <= SERVO_ANGLE_MAX)){
        //printf("ERROR: Servo angle 3 out of range: Angle = ", radsToDeg(theta));
        //Serial.println("");
        return false;
    }
    //printf("\n servo_3_angle = %f", radsToDeg(theta));
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

void Intermed_position(MotionInstance& inst, unsigned long nowMs, Coordinate& xyz, boolean trig_serial) { //Interpolação linear entre dois pontos a e b com t em [0,1] -> de momento não está a ser utilizada
MotionStorage& m = *(inst.def);

if (!inst.active || m.nPoints <= 1 || m.period <= 0.0f) {
       xyz.X = 0.0f;
       xyz.Y = 0.0f;
       xyz.Z = 0.0f;
        return;
    }

    // calcula alpha com base em elapsed e dtMs
    //inst.elapsed = nowMs - inst.segmentStartMs;

    int i = inst.iIndex;
    int j = inst.jIndex;
    float elapsed;

    // Se estiveres a ir para trás, inverte os extremos do segmento
    if(inst.Direction == 1){
        elapsed = (float)inst.elapsed;
    }
    if (inst.Direction == -1) {
        elapsed = (float)inst.elapsed_3;

    }

    float alpha = elapsed / m.dtMs;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    xyz.X = lerp(m.X[i], m.X[j], alpha);
    xyz.Y = lerp(m.Y[i], m.Y[j], alpha);
    xyz.Z = lerp(m.Z[i], m.Z[j], alpha);
    
    return;
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



void debugPrintMove(const MotionStorage& move, const char* name) {
    Serial.print("=== ");
    Serial.print(name);
    Serial.println(" contents ===");

    Serial.print("nPoints: ");
    Serial.println(move.nPoints);
    Serial.print("period (sec): ");
    Serial.println(move.period);
    Serial.print("dtMs: ");
    Serial.println(move.dtMs);
    Serial.print("T_pause (sec): ");
    Serial.println(move.T_pause);
    Serial.print("dt_PauseMs: ");
    Serial.println(move.dt_pauseMs);

    for (int i = 0; i < move.nPoints; i++) {
        Serial.print("i=");
        Serial.print(i);
        Serial.print("  X=");
        Serial.print(move.X[i]);
        Serial.print("  Y=");
        Serial.print(move.Y[i]);
        Serial.print("  Z=");
        Serial.print(move.Z[i]);
        Serial.println("");

    }
}


void debugPrintInstance(const MotionInstance& inst, const char* name) {
    Serial.print("=== ");
    Serial.print(name);
    Serial.println(" state ===");

    Serial.print("active: ");
    Serial.println(inst.active ? "true" : "false");

    Serial.print("currentIndex: ");
    Serial.println(inst.currentIndex);

    Serial.print("lastStepMs: ");
    Serial.println(inst.lastStepMs);

    Serial.print("segmentStartMs: ");
    Serial.println(inst.segmentStartMs);

    Serial.print("def pointer: ");
    Serial.println((uintptr_t)inst.def, HEX);
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




void FSM_Motion_Update(boolean start, MotionInstance& inst, unsigned long nowMs){
//unsigned long elapsed;
MotionStorage& m = *(inst.def);
/*
Têm em atenção constituição de cada caso:
case x: 
  - Ações a executar
  - Condições de transição por hierarquia
*/
//Serial.print("Estado atual: ");
//Serial.println(inst.state);

switch (inst.state) {

    case 0:
      if (start==0) {inst.state = 0;}
      if (!inst.active || inst.def == nullptr) {inst.state = 0;}
      if(start==1 && inst.active && inst.def != nullptr){inst.state = 1;}
    break;

    case 1:
      if (m.nPoints <= 1 || m.period <= 0.0f) {inst.state = 1;}
      else {inst.state = 2;}
    break;

    case 2:
      inst.segmentStartMs = nowMs;
        inst.jIndex = (inst.iIndex + 1) % m.nPoints;
      if (inst.Direction == 1) {inst.state = 3;}
    break;

    case 3: 
      inst.elapsed = nowMs - inst.segmentStartMs;
      //Serial.print("index: ");
       // Serial.print(inst.iIndex);
       // Serial.print("Elapsed: ");
       // Serial.println(inst.elapsed);
      if(inst.elapsed >= m.dtMs) {inst.state = 4;}
    break;

    case 4: 
      inst.iIndex++;
      inst.jIndex = (inst.iIndex + 1) % m.nPoints;
      inst.elapsed = 0;
      inst.segmentStartMs = nowMs;
      if(inst.iIndex < m.nPoints -1) {inst.state = 3;}
      if(inst.iIndex >= m.nPoints -1 ) {inst.state = 5;}
    break;

    case 5: //função trás
      inst.segmentStartMs = nowMs;
      inst.jIndex = inst.iIndex;

      if (m.bidirectional == true && inst.Direction == 1) {
        inst.state = 8;          // RESP: ignora pausa no topo, vai logo para trás
      }
      else {
        if (m.dt_pauseMs != 0) { inst.state = 6; }
        if (m.dt_pauseMs == 0) { inst.state = 7; }
      }
    break;


    case 6: 
      inst.elapsed_2 = nowMs - inst.segmentStartMs;
      inst.iIndex = 0;
      inst.jIndex = 0;

      if(inst.elapsed_2 >= m.dt_pauseMs) {inst.state = 7;}
      //mais tarde este estado vai definir o tipo de reciclagem do movimento, ou seja, se volta ao inicio, se inverte o ciclo, etc.
    break;

    case 7: 
      inst.iIndex = 0;
      inst.Direction = 1;
      inst.elapsed_2 = 0;
  
      if(!inst.active || start == 0) {inst.state = 0;}
      else {inst.state = 2;}
    break;


    case 8: //função trás
      inst.Direction = -1;
      inst.jIndex = (inst.iIndex - 1);
      inst.segmentStartMs = nowMs; 
      if(1){inst.state = 9;}
    break;

    case 9: //função trás
      inst.elapsed_3 = nowMs - inst.segmentStartMs;
   
      if(inst.elapsed_3 >= m.dtMs) {inst.state = 10;}

    break;

    case 10: //função trás
      inst.iIndex--;
      inst.jIndex = (inst.iIndex - 1);
      inst.segmentStartMs = nowMs;
      inst.elapsed_3 = 0;

      if(inst.iIndex > 0) {inst.state = 9;}
      if(inst.iIndex <= 0) {inst.state = 5;}

    break;

    

    }
    return;
}


