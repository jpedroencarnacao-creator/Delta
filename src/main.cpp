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
    //Servo_1 -> thetta3
    //Servo_2 -> thetta1
    //Servo_3 -> thetta2
    .servo1MinPulse = 460,
    .servo1MaxPulse = 2460,

    .servo2MinPulse = 470,
    .servo2MaxPulse = 2470,

    .servo3MinPulse = 420,
    .servo3MaxPulse = 2420,

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

//bat
float X_test[] = {15.607,  14.923,  13.995,  12.845,  11.503, 10.000,  6.665,  1.464,  -2.965,  -5.000, -5.659, -6.056, -6.180, -6.029, -5.607, -4.923, -3.995, -2.845, -1.503,  0.000,   3.335,  8.536,  12.965,  15.000, 15.659, 16.056, 16.180, 16.029};
//float Y_test[] = {  0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,    0.0};
float Y_test[] = {15.607,  16.029,  16.180,  16.056,  15.659, 15.000, 12.965,  8.536,   3.335,   0.000, -1.503, -2.845, -3.995, -4.923, -5.607, -6.029, -6.180, -6.056, -5.659, -5.000,  -2.965,  1.464,   6.665,  10.000, 11.503, 12.845, 13.995, 14.923};
//float Z_test[] = {15.607,  16.029,  16.180,  16.056,  15.659, 15.000, 12.965,  8.536,   3.335,   0.000, -1.503, -2.845, -3.995, -4.923, -5.607, -6.029, -6.180, -6.056, -5.659, -5.000,  -2.965,  1.464,   6.665,  10.000, 11.503, 12.845, 13.995, 14.923};
//float Z_test[] = {  40.0,    40.0,    40.0,    40.0,    40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,   40.0,   40.0,   40.0,   40.0,   40.0,   40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,    40.0,   40.0,    40.0};
float Z_test[] = {  0.0,    0.0,    0.0,    0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,    0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0,   0.0,    0.0};
const int N_test = 28;
float T_period = 0.3f; 
float T_pausedt = 0.6f; 

//resp
//float X_test2[] = {0.000, 1.333, 2.667, 4.000, 5.333, 6.667, 8.000, 9.333, 10.667, 12.000, 10.667, 9.333, 8.000, 6.667, 5.333, 4.000, 2.667,   1.333 };
//float Y_test2[] = {  0.0,   0.0,   0.0,    0.0,  0.0,   0.0,   0.0,   0.0,   0.0,     0.0,    0.0,    0.0,   0.0,   0.0,  0.0,    0.0,   0.0,  0.0 };
//float Z_test2[] = {0.000, 0.247, 0.988, 2.222, 3.951, 6.173, 8.889, 12.099, 15.802, 20.000, 15.802, 12.099, 8.889, 6.173, 3.951, 2.222, 0.988, 0.247};
float X_test2[] = {0.000, 1.333, 2.667, 4.000, 5.333, 6.667, 8.000, 9.333, 10.667, 12.000};
float Y_test2[] = {  0.0,   0.0,   0.0,    0.0,  0.0,   0.0,   0.0,   0.0,   0.0,     0.0};
float Z_test2[] = {0.000, 0.247, 0.988, 2.222, 3.951, 6.173, 8.889, 12.099, 15.802, 20.000};

//const int N_test2 = 18;
const int N_test2 = 10;
float T_period2 = 4.0f; 
float T_pausedt2_Ini = 0.6f; 
float T_pausedt2_End = 0.1f; 
//Link lengths (mm)
#define L1 35
#define L2 60
#define L3 15

#define END_EFFECTOR_Z_OFFSET 0
#define SERVO_OFFSET_X 32
#define SERVO_OFFSET_Y 0
#define SERVO_OFFSET_Z (0.0 + END_EFFECTOR_Z_OFFSET)
//#define SERVO_OFFSET_Z_INVERTED -293


//#define SERVO_ANGLE_MIN (100.0f * PI / 180.0f)
//#define SERVO_ANGLE_MAX (190.0f * PI / 180.0f)

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

// fator de suavização [0 - 1]. tipo 0.2 para começar
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
bool Tosse_signal = 0;
bool sinal_tossir = false;  
bool Tosse_em_preparacao = false; 
float Resp_period_padrao = 0.0f;
float Resp_T_pause_Ini_padrao = 0.0f;
// Máximo de pontos por movimento (ajusta mais tarde se precisares)
const int MAX_POINTS = 100;

// Armazenamento Bruto: dados completos de um movimento - Será utilizada para armazenar os movimentos que o Pi enviar, ou movimentos pré-definidos, etc.
struct MotionStorage {
    int   n_Points;                   // nº de pontos do ciclo
    int N_points;                     // nº de pontos do movimento completo
    float X[MAX_POINTS];            // coordenadas X dos pontos do movimento
    float Y[MAX_POINTS];            // coordenadas Y dos pontos do movimento
    float Z[MAX_POINTS];            // coordenadas Z dos pontos do movimento
    float period; 
    float T_pause_Ini;   
    float T_pause_End;
    float dtMs;                // duração do ciclo em segundos
    float Dt_pauseMs_Ini;           // duração da pausa entre ciclos em segundos
    float Dt_pauseMs_End;
    int  start_Index;  // índice de ponto que será o "0,0,0" lógico
    bool  bidirectional;   // true = vai-e-vem, false = ciclo fechado

    float getDtMs(){
        if (n_Points > 1 && period > 0.0f) {
            if (bidirectional==true) {
                // 0..N-1..0  -> 2*(N-1) segmentos
                N_points  = (n_Points * 2) - 2;
            } else {
                // ciclo normal 0..N-1..0, como já tinhas
                N_points = n_Points-1;
                
            }
            return (period * 1000.0f) / (float)N_points;
        }
        return 0.0f;
    }

    void updateDtMs() {
    dtMs = getDtMs();    // aqui sim, mudas dtMs
    }

  float getD_PauseMs(float T_pause) const {
    if (n_Points > 1 && T_pause > 0.0f) {
        return (T_pause * 1000.0f);  // só lê nPoints e T_pause
        }
       return 0.0f;
    }

    void updateDt_PauseMs() {
    Dt_pauseMs_Ini = getD_PauseMs(T_pause_Ini);    // aqui sim, mudas dt_pauseMs
    Dt_pauseMs_End = getD_PauseMs(T_pause_End);    // aqui sim, mudas dt_pauseMs
  }

};




// Armazenamento temporário: runtime que executa um MotionStorage
struct MotionInstance {
    MotionStorage* def;         // ponteiro para MotionStorage (a definição do Armazenamento Bruto )
    int            currentIndex;// índice i (início do segmento i -> i+1) -> índice atual no movimento
    unsigned int   segmentStartMs; // timestamp do início do segmento atual
                                   //    -> é uma variavel importante para controlar a velocidade do movimento, ou seja, quando é que deve avançar para o próximo ponto do movimento
    bool           active;      // se este movimento está ativo
    bool           Individual_start_comand;
    bool           Executing;      // se este movimento está em pausa (entre ciclos)
    short int      state;
    float Dt_pause;
    unsigned long elapsed_1;
    unsigned long elapsed_2;
    unsigned long elapsed_3;

    int iIndex;  // novo: índice i para a interpolação
    int jIndex;  // novo: índice j para a interpolação
    int I_Index;
    int  End_Point; 
    int Direction; // novo: direção do movimento (1 ou -1)
    char Situacion;
    float Z_amp_mult;
};



// ------------------Temporario---------------------------------
MotionStorage Bat_move; //Declaração de objetos para utilizar com as structures de movimento
MotionInstance Bat_inst;
MotionStorage Resp_move; //Declaração de objetos para utilizar com as structures de movimento
MotionInstance Resp_inst;
MotionStorage Tosse_move; //Declaração de objetos para utilizar com as structures de movimento
MotionInstance Tosse_inst;
MotionStorage* currentMove = nullptr;
MotionInstance* currentInst = nullptr;

void Iniciate_Bat_Move() { //Serve para copiar os valores definidos mais acima para o objeto Bat_move, que é do tipo MotionStorage. 
                        // ->    Isto é só para facilitar a criação de movimentos de teste, ou seja, para não ter que copiar os valores manualmente para o Bat_move cada vez que quiseres testar algo.
                        // ->    No futuro será utilizado para copiar os movimentos pré definidos da EPPROM ou do PI
    Bat_move.n_Points = N_test;
    Bat_move.period  = T_period;
    Bat_move.T_pause_End = T_pausedt;
    for (int i = 0; i < N_test; i++) {
        Bat_move.X[i] = X_test[i];
        Bat_move.Y[i] = -1 * Y_test[i];
        Bat_move.Z[i] = Z_test[i];

    }
    Bat_move.start_Index = 0;
    Bat_move.bidirectional = false; // Define o movimento como vai-e-vem 
    Bat_move.updateDtMs();
    Bat_move.updateDt_PauseMs();
    //Serial.print("BAT N_point ->global: ");
    //Serial.println(Bat_move.N_points);
}






void Iniciate_Resp_Move() { //Serve para copiar os valores definidos mais acima para o objeto Resp2_move, que é do tipo MotionStorage. 
                        // ->    Isto é só para facilitar a criação de movimentos de teste, ou seja, para não ter que copiar os valores manualmente para o Resp2_move cada vez que quiseres testar algo.
                        // ->    No futuro será utilizado para copiar os movimentos pré definidos da EPPROM ou do PI
    Resp_move.n_Points = N_test2;
    Resp_move.period  = T_period2;
    Resp_move.T_pause_End = T_pausedt2_End;
    Resp_move.T_pause_Ini = T_pausedt2_Ini;
    for (int i = 0; i < N_test2; i++) {
        Resp_move.X[i] = X_test2[i];
        Resp_move.Y[i] = -1 * Y_test2[i];
        Resp_move.Z[i] = Z_test2[i];
    }
    Resp_move.start_Index = 5;
    Resp_move.bidirectional = true; // Define o movimento como vai-e-vem
    Resp_move.updateDtMs();
    Resp_move.updateDt_PauseMs();
    //Serial.print("RESP N_point ->global: ");
    //Serial.println(Resp_move.N_points);
}


void Iniciate_Tosse_Move() { //Serve para copiar os valores definidos mais acima para o objeto Resp2_move, que é do tipo MotionStorage. 
                        // ->    Isto é só para facilitar a criação de movimentos de teste, ou seja, para não ter que copiar os valores manualmente para o Resp2_move cada vez que quiseres testar algo.
                        // ->    No futuro será utilizado para copiar os movimentos pré definidos da EPPROM ou do PI
    //Tosse_move.n_Points = N_test2;
    //Tosse_move.period  = T_period2;
    //Tosse_move.T_pause_End = T_pausedt2_End;
    //Tosse_move.T_pause_Ini = T_pausedt2_Ini;
    //for (int i = 0; i < N_test2; i++) {
   //     Tosse_move.X[i] = X_test2[i];
   //     Tosse_move.Y[i] = -1 * Y_test2[i];
   //     Tosse_move.Z[i] = Z_test2[i];
   // }
   // Tosse_move.start_Index = 0;
    //Tosse_move.bidirectional = false; // Define o movimento como vai-e-vem
    //Tosse_move.updateDtMs();
    //Tosse_move.updateDt_PauseMs();
   // Serial.print("RESP N_point ->global: ");
   // Serial.println(Resp_move.N_points);
}
    
void Iniciate_Bat_Instance() { //Serve para arrancar a structure de temporaria de runtime
    Bat_inst.def           = &Bat_move;
    Bat_inst.currentIndex  = 0;
    Bat_inst.segmentStartMs = 0; //millis(); // ou 0, e tratamos no update
    Bat_inst.active        = true;
    Bat_inst.Individual_start_comand = 1;
    Bat_inst.Executing       = 0; // novo: inicia o movimento em pausa, ou seja, não começa imediatamente
    Bat_inst.state         = 0;
    Bat_inst.elapsed_1     = 0;
    Bat_inst.elapsed_2     = 0;
    Bat_inst.elapsed_3     = 0;
    Bat_inst.I_Index       = 0;
    Bat_inst.iIndex        = 0;
    Bat_inst.jIndex        = 0;
    Bat_inst.Direction     = 1; // novo: inicializa a direção como positiva
    Bat_inst.Dt_pause      = 0;
    Bat_inst.End_Point     = 0;
    Bat_inst.Situacion    = 'F';
}

void Iniciate_Resp_Instance() { //Serve para arrancar a structure de temporaria de runtime
    Resp_inst.def           = &Resp_move;
    Resp_inst.currentIndex  = 0;
    Resp_inst.segmentStartMs = 0; //millis(); // ou 0, e tratamos no update
    Resp_inst.active        = true;
    Resp_inst.Individual_start_comand = 1;
    Resp_inst.Executing       = false; // novo: inicia o movimento em pausa, ou seja, não começa imediatamente
    Resp_inst.state         = 0;
    Resp_inst.elapsed_1     = 0;
    Resp_inst.elapsed_2     = 0;
    Resp_inst.elapsed_3     = 0;
    Resp_inst.I_Index       = 0;
    Resp_inst.iIndex        = 0;
    Resp_inst.jIndex        = 0;
    Resp_inst.Direction     = 1; // novo: inicializa a direção como positiva
    Resp_inst.Dt_pause      = 0;
    Resp_inst.End_Point     = 0;
    Resp_inst.Situacion     = 'F';
    Resp_inst.Z_amp_mult = 1.0f;
}

void Iniciate_Tosse_Instance() { //Serve para arrancar a structure de temporaria de runtime
    Tosse_inst.def           = &Resp_move;
    Tosse_inst.currentIndex  = 0;
    Tosse_inst.segmentStartMs = 0; //millis(); // ou 0, e tratamos no update
    Tosse_inst.active        = true;
    Tosse_inst.Individual_start_comand = 0;
    Tosse_inst.Executing       = false; // novo: inicia o movimento em pausa, ou seja, não começa imediatamente
    Tosse_inst.state         = 0;
    Tosse_inst.elapsed_1     = 0;
    Tosse_inst.elapsed_2     = 0;
    Tosse_inst.elapsed_3     = 0;
    Tosse_inst.I_Index       = 0;
    Tosse_inst.iIndex        = 0;
    Tosse_inst.jIndex        = 0;
    Tosse_inst.Direction     = 1; // novo: inicializa a direção como positiva
    Tosse_inst.Dt_pause      = 0;
    Tosse_inst.End_Point     = 0;
    Tosse_inst.Situacion     = 'F';
}



//-------------------Fim Temporario---------------------------------

struct Coordinate {
    float X;
    float Y;
    float Z;
};




int Clock_loop = 0;
const float pi = 3.141592653;


// ---------Declaração das variaveis------------------
void LED_LOOP(); //Declaração de funções
void Servo_test();
//-------Novas-------------
float boundFloat(float, float, float);
bool attach_servos(void);
bool inverse_kinematics_1(float, float, float, float, float&);
bool inverse_kinematics_2(float, float, float, float, float&);
bool inverse_kinematics_3(float, float, float, float, float&);
bool inverse_kinematics(Manipulador_Config& cfg, float, float, float);
void move_servos(const Manipulador_Config& d1, const Manipulador_Config& d2);
double mapNumber(double x, double in_min, double in_max, double out_min, double out_max);
int roundMapNumber(double x, double in_min, double in_max, double out_min, double out_max);
double degToRads(double deg);
double radsToDeg(double rads);
void debugPrintMove(const MotionStorage& move, const char* name);
void debugPrintInstance(const MotionInstance& inst, const char* name);
float lerp(float a, float b, float t);
float applyEasing(float alpha, char mode);
float spline3(float p0, float p1, float p2, float t);
void Intermed_position(boolean start, MotionInstance& inst, unsigned long nowMs, Coordinate& xyz, boolean trig_serial);
bool Fusao_plus_IK_D1(const Coordinate& respi, const Coordinate& batim);
bool Fusao_plus_IK_D2(const Coordinate& respi, const Coordinate& batim);
//------------------FSM-------------------------------
void FSM_Motion_Update(boolean start, MotionInstance& inst, unsigned long nowMs);
void FSM_Serial_reader(unsigned long nowMs);
void FSM_Serial_Command(unsigned long nowMs);
void FSM_Main(boolean start, MotionInstance& inst, unsigned long nowMs);
void Preparacao_Tosse();
void Calcular_Index0(MotionStorage& m, Coordinate& idx0);
//----------------------------------------------------

//--Declaração dos comandos de leitura
//char commands
#define Scomand_start_Program 's'
#define Scomand_stop_Program 'p'
#define Scomand_GRIPPER 3
#define Scomand_ABSOLUTE_CARTESIAN_LINEAR 4
#define Scomand_SET_PROGRAM_ARRAY 5
#define Scomand_REQUEST_READY_FLAG 6

//String commands
#define Mcomand_JOG_X 'x'
#define Mcomand_JOG_Y 'y'
#define COMMAND_JOG_Z 'z'
#define COMMAND_GRIPPER_ASCII 'g'
#define COMMAND_STATUS 's' 
#define COMMAND_REPORT_COMMANDS 'r'
#define COMMAND_ADD_POSITION 'p'
#define COMMAND_SET_STEP_DELAY 'd'
#define COMMAND_SET_STEP_INCREMENT 'i'
#define COMMAND_CLEAR_ARRAY 'c'
#define COMMAND_EXECUTE 'e'
#define COMMAND_EXECUTE_JOINT 'j'
#define COMMAND_EXECUTE_TIME 't'
#define COMMAND_SET_US_INCREMENT_LINEAR 'u'
#define COMMAND_SET_US_INCREMENT_JOINT 'U'
#define COMMAND_STEP_FORWARD '>'
#define COMMAND_STEP_BACKWARD '<'
#define COMMAND_JUMP_TO_START '['
#define COMMAND_JUMP_TO_END ']'
#define COMMAND_EDIT_ARRAY 'P'
#define COMMAND_ADD_DELAY 'D'
#define COMMAND_MOVE_HOME 'h'
#define COMMAND_PRINT_FILE 'f'
#define COMMAND_PING_PONG 'o'
#define COMMAND_SERVO_CALIBRATION 'C'
#define COMMAND_SET_SERVO 'S'

//EEPROM commands
#define Ecomand_SET_LINK_2 'L'
#define Ecomand_SET_END_EFFECTOR_TYPE 'E'
#define Ecomand_SET_AXIS_DIRECTION 'A'
#define Ecomand_SET_HOME_X 'X'
#define Ecomand_SET_HOME_Y 'Y'
#define Ecomand_SET_HOME_Z 'Z'
#define Ecomand_SET_HOME_GRIPPER 'G'
#define Ecomand_SET_GRIPPER_MIN_MAX 'M'

//EEPROM addresses for the delta robots configuration
#define EEPROM_ADDRESS_LINK_2 0
#define EEPROM_ADDRESS_END_EFFECTOR_TYPE 1
#define EEPROM_ADDRESS_AXIS_DIRECTION 2
#define EEPROM_ADDRESS_Z_OFFSET 3
#define EEPROM_ADDRESS_HOME_X 7
#define EEPROM_ADDRESS_HOME_Y 11
#define EEPROM_ADDRESS_HOME_Z 15
#define EEPROM_ADDRESS_HOME_GRIPPER 19
#define EEPROM_ADDRESS_GRIPPER_ROTATION_MIN 21
#define EEPROM_ADDRESS_GRIPPER_ROTATION_MAX 23
#define EEPROM_ADDRESS_GRIPPER_CLAW_MIN 25
#define EEPROM_ADDRESS_GRIPPER_CLAW_MAX 27
#define EEPROM_ADDRESS_GRIPPER_VACUUM_MIN 29
#define EEPROM_ADDRESS_GRIPPER_VACUUM_MAX 31




//Delta1 --------------------------------
Servo mg90s_1;  // cria objeto servo
Servo mg90s_2;  // cria objeto servo
Servo mg90s_3;  // cria objeto servo
  

//Delta2 --------------------------------
Servo mg90s_4;  // cria objeto servo
Servo mg90s_5;  // cria objeto servo        
Servo mg90s_6;  // cria objeto servo


volatile boolean start_comand = 0;  


Coordinate Lerp_Respi_OUT;
Coordinate Lerp_Batim_OUT;
Coordinate IK2_IN;
Coordinate home_position;
Coordinate Index0;


float servo_offset_z = SERVO_OFFSET_Z;

//-----------------Declaração da localização dos pinos para cada objeto ------------------
//-------------------Servos-----------------
#define SERVO_PIN_1 12  //Servo_1 -> thetta3
#define SERVO_PIN_2 11  //Servo_2 -> thetta1
#define SERVO_PIN_3 10  //Servo_3 -> thetta2

#define SERVO_PIN_4 36  // Servo_4 -> thetta3
#define SERVO_PIN_5 37  // Servo_5 -> thetta1
#define SERVO_PIN_6 38  // Servo_6 -> thetta2

//----------------Led_Informação------------
#define LED_PIN 48
#define LED_COUNT 1


//------------------------------------------------------------------------------------------
bool stepDirection = false;
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

char Mode = 'S'; // 'S' = Single, 'M' = Manual
char CH_Command_Out = '\0';
char CH_Command_IN = '\0';
char ST_Command_Out[100];
char ST_Command_IN[100];
int command_counter = 0;
bool LineReady_IN = false;
bool LineReady_Out = false;



boolean trig_display_fsm(float at, short int Td){
static short int state; //state internal to the state machine
static float pt; //var. internal to the state machine
boolean trig; // output

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

int setup_counter = 0;






int state_test = 0;
int state_test2 = 0;
float Training_Exame_Starttime = 0;
float Training_Exame_Finishingtime = 0;
float Training_Exame_Durantiontime = 0;
int Safe_comand = 0;


void FSM_Serial_reader(unsigned long nowMs){
static short int state;
static int counter = 0;
static int n = 0;
static char CH_Bus;
static char ST_BUS[100];
state_test = state;
/*
Têm em atenção constituição de cada caso:
case x: 
  - Ações a executar
  - Condições de transição por hierarquia
*/
//Serial.print("Estado atual: ");
//Serial.println(inst.state);

switch (state) {

    case 0:
      counter = 0;
      CH_Command_Out = '\0'; 

      if(Serial.available() > 0 && Mode == 'S'){state = 1;} 
      if(Serial.available() > 0 && Mode == 'M'){state = 6;} 
    break;


    case 1:
      CH_Bus = Serial.read();
      counter++;

      if(CH_Bus == ' '){state = 1;}
      if(CH_Bus != '\n' && CH_Bus != ' '){state = 2;} 
      if(CH_Bus == '\n'){state = 4;} 
    break;


    case 2:
      CH_Command_Out= CH_Bus;
      command_counter++;
      
      if (Serial.available() > 0) { state = 3;}
       else {state = 5;}
    break;


    case 3: 
      CH_Bus = '\0';

      if(LineReady_IN == true && Mode == 'S'){state = 1;}
      if(LineReady_IN == true && Mode == 'M'){state = 6;} 
    break;


    case 4: 
      CH_Bus = '\0';

      if(1){state = 0;}
    break;

    case 5: 
      CH_Bus = '\0';

      if(LineReady_IN == true){state = 0;}
    break;
/*
    case 6: 
      ST_BUS[counter] = Serial.read();
      Serial.print("ST_Bus: ");
      Serial.println(ST_BUS[counter]);
      counter++;

      if(ST_BUS[counter-1] == ' '){state = 6;}
      //if(Serial.available() > 0 && ST_BUS[counter-1] != '\n'){state = 6;}
      if(ST_BUS[counter-1] == '\n'){state = 8;}
    break;
*/
    case 6: {
    int c = Serial.read();
    ST_BUS[n] = (char)c;

    Serial.print("ST_Bus char = '");
    Serial.print(ST_BUS[n]);
    Serial.print("'  code = ");
    Serial.println(c);


      if(ST_BUS[n] == ' '){state = 6;}
      if(ST_BUS[n] != '\n' && ST_BUS[n] != ' '){state = 7;} 
      if(ST_BUS[n] == '\n'){state = 8;} 
    break;
}

    case 7:
    n++;

    if(Serial.available() > 0){state = 6;}
    if(Serial.available() < 1){state = 8;}
    break;

    case 8: {
  Serial.print("ST_Bus completo: ");
  Serial.println(ST_BUS);
  int maxCopy = n;
  if (maxCopy >= (int)sizeof(ST_Command_Out)) {
      maxCopy = sizeof(ST_Command_Out) - 1;
  }
  strncpy(ST_Command_Out, ST_BUS, maxCopy);

  if(1){state = 9;}
  break;
}


    case 9:
    ST_BUS[n] = '\0';   // substitui o '\n'
    //ST_Command_Out[maxCopy] = '\0';
    n = 0;

    if(LineReady_IN == true){state = 0;}
    break;


    }
    return;
}

void FSM_Serial_Command(unsigned long nowMs){
static short int state;
state_test2 = state;
static int n = 0;
static int m = 0;
static char ST_copy[100];
/*
Têm em atenção constituição de cada caso:
case x: 
  - Ações a executar
  - Condições de transição por hierarquia
*/
//Serial.print("Estado atual: ");
//Serial.println(inst.state);

switch (state) {

    case 0:
      Serial.println("Escolha um comando:");

      if(1){state = 1;}

    break;

    case 1:
      LineReady_Out = false;

      if(CH_Command_IN == 'S' || CH_Command_IN == 's'){state = 2;}
      if(CH_Command_IN == 'P' || CH_Command_IN == 'p'){state = 3;}
      if(CH_Command_IN == 'L' || CH_Command_IN == 'l'){state = 5;}
      if(CH_Command_IN == 'M' || CH_Command_IN == 'm'){state = 14;}
      if(CH_Command_IN == 'W' || CH_Command_IN == 'w'){state = 23;}
    break;


    case 2:
      Serial.println("Ativo");
            start_comand = 1;
            Training_Exame_Starttime = nowMs;
      LineReady_Out = true;
      CH_Command_Out = '\0';
      pixel.setPixelColor(0, pixel.Color(255, 0, 0));
      pixel.show(); 

      if (1) { state = 4;} 
    break;


    case 3:
      Serial.println("Desativo");
            start_comand = 0;
            Training_Exame_Finishingtime = nowMs;
            Training_Exame_Durantiontime = Training_Exame_Finishingtime - Training_Exame_Starttime; 
      if(Training_Exame_Durantiontime > 0){
        Serial.print("Duração do treino: "); 
        Serial.print(Training_Exame_Durantiontime/1000.0f); 
        Serial.println(" Segundos");
        }    
      Training_Exame_Finishingtime = 0;
      Training_Exame_Starttime = 0; 
      LineReady_Out = true;
      CH_Command_Out = '\0';
      pixel.setPixelColor(0, pixel.Color(0, 0, 0));
      pixel.show();
      
      if (1) { state = 4;}
    break;


    case 4: 
      Serial.println("Escolha um comando:");
      LineReady_Out = false;

      if(1){state = 1;}
    break;

    case 5: 
      Serial.println("ler");
      LineReady_Out = true;
      CH_Command_Out = '\0';
      pixel.setPixelColor(0, pixel.Color(255, 255, 0));
      pixel.show(); 

      if(1){state = 6;}
    break;

    case 6: 
      LineReady_Out = false;

      if(CH_Command_IN == '1'){state = 7;}
      if(CH_Command_IN == '2'){state = 8;}
      if(CH_Command_IN == '3'){state = 9;}
      if(CH_Command_IN == 'r'){state = 10;}
      if(CH_Command_IN == 'b'){state = 11;}
      if(CH_Command_IN == 't'){state = 12;}
      if(CH_Command_IN == 'a'){state = 13;}
    break;


    case 7: 
      Serial.println("Comando L1");
      LineReady_Out = true;
      CH_Command_Out = '\0';
      pixel.setPixelColor(0, pixel.Color(255, 255, 255));
      pixel.show();
      Safe_comand = 1;

      if(1){state = 4;}
    break;

    case 8: 
      Serial.println("Comando L2");
      LineReady_Out = true;
      CH_Command_Out = '\0';
      pixel.setPixelColor(0, pixel.Color(0, 255, 255));
      pixel.show();
      Safe_comand = 0;

      if(1){state = 4;}
    break;

    case 9:
      Serial.println("Comando L3");
      LineReady_Out = true;
      CH_Command_Out = '\0';
      sinal_tossir = 1;
      //Serial.println("Sem nada");  //Retirar quando estiver implementado  
        pixel.setPixelColor(0, pixel.Color(255, 0, 255));
      if(1){state = 4;}
    break;

    case 10:
      Serial.println("Comando Lr");
      LineReady_Out = true;
      CH_Command_Out = '\0';
      debugPrintMove(Resp_move, "Resp_move"); 
      debugPrintInstance(Resp_inst, "Resp_inst");
      
      
      if(1){state = 4;}
    break;

    case 11:
      Serial.println("Comando Lb");
      LineReady_Out = true;
      CH_Command_Out = '\0';
      debugPrintMove(Bat_move,  "Bat_move");
      debugPrintInstance(Bat_inst,  "Bat_inst");
      
      if(1){state = 4;}
    break;


    case 12:
      Serial.println("Comando Lt");
      LineReady_Out = true;
      CH_Command_Out = '\0';
      //debugPrintMove(Bat_move,  "Tosse_move");   //ainda é preciso implementar
      //debugPrintInstance(Bat_inst,  "Tosse_inst");  //ainda é preciso implementar
      Serial.println("Sem nada");  //Retirar quando estiver implementado 
      
      if(1){state = 4;}
    break;


    case 13:
      Serial.println("Comando LA");
      LineReady_Out = true;
      CH_Command_Out = '\0';
      debugPrintMove(Bat_move,  "Bat_move");
      debugPrintMove(Resp_move, "Resp_move"); 
      debugPrintInstance(Bat_inst,  "Bat_inst");
      debugPrintInstance(Resp_inst, "Resp_inst");
      Serial.println("Sem Tosse");  //Retirar quando estiver implementado a tosse
      
      if(1){state = 4;}
    break;


    case 14: 
      Serial.println("Comando Mode"); 
            start_comand = 0;
            Training_Exame_Finishingtime = nowMs;
            Training_Exame_Durantiontime = Training_Exame_Finishingtime - Training_Exame_Starttime; 
      if(Training_Exame_Durantiontime > 0){
        Serial.print("Duração do treino: "); 
        Serial.print(Training_Exame_Durantiontime/1000.0f); 
        Serial.println(" Segundos");
        }    
      Training_Exame_Finishingtime = 0;
      Training_Exame_Starttime = 0; 
      LineReady_Out = true;

      if(1){state = 15;}
    break;


    case 15: 
      LineReady_Out = false;

      if(CH_Command_IN == 'a'){state = 16;}
      if(CH_Command_IN == 'h'){state = 17;}
      if(CH_Command_IN == 'r'){state = 18;}
      if(CH_Command_IN == 'b'){state = 19;}
      if(CH_Command_IN == 's'){state = 20;}
    break;


    case 16: 
      Serial.println("Comando Mode ALL"); 
      LineReady_Out = true;
      CH_Command_Out = '\0';
            Bat_inst.active = true;
            Resp_inst.active = true;
          //Tosse_inst.active = true;

      if(1){state = 21;}
    break;


    case 17: 
      Serial.println("Comando Mode Human (Bat+Resp)"); 
      LineReady_Out = true;
      CH_Command_Out = '\0';
            Bat_inst.active = true;
            Resp_inst.active = true;
            Tosse_inst.active = false;

      if(1){state = 21;}
    break;


    case 18: 
      Serial.println("Comando Mode Respiração"); 
      LineReady_Out = true;
      CH_Command_Out = '\0';
            Bat_inst.active = false;
            Resp_inst.active = true;
            Tosse_inst.active = false;

      if(1){state = 21;}
    break;


    case 19: 
      Serial.println("Comando Mode Batimento"); 
      LineReady_Out = true;
      CH_Command_Out = '\0';
            Bat_inst.active = true;
            Resp_inst.active = false;
            Tosse_inst.active = false;

      if(1){state = 21;}
    break;


    case 20: 
      Serial.println("Comando Mode Static"); 
      LineReady_Out = true;
      CH_Command_Out = '\0';
            Bat_inst.active = false;
            Resp_inst.active = false;
            Tosse_inst.active = false;

      if(1){state = 21;}
    break;


    case 21: 
      //Serial.println("want to start de cicle? y/n"); 
      //LineReady_Out = false;

      if(1){state = 4;}
    break;

    case 22: 
      if(CH_Command_IN == 'y'){state = 2;}
      if(CH_Command_IN == 'n'){state = 3;}
    break;

    case 23: 
      Mode = 'M';
      Serial.println("Comando Mode multiple characters");
      LineReady_Out = true;

      if(1){state = 24;}
    break;

    case 24: 
      LineReady_Out = false;
      n = 0;

      if(ST_Command_IN[n] == 'D'){state = 25;}
      if(ST_Command_IN[n] == 'd'){state = 26;}
    break;

    case 25: 
      start_comand = 0;
      n++;

      if(1){state = 27;}
    break;

    case 26: 
      n++;

      if(1){state = 28;}
    break;

    case 27: 
      if(ST_Command_IN[n] == 'R' || ST_Command_IN[n] == 'r'){state = 28;}
      if(ST_Command_IN[n] == 'B' || ST_Command_IN[n] == 'b'){state = 29;}
      if(ST_Command_IN[n] == 'T' || ST_Command_IN[n] == 't'){state = 30;}
    break;

    case 28: 
      currentMove = &Resp_move;
      currentInst = &Resp_inst;

      if(1){state = 31;}
    break;


    case 29: 
      currentMove = &Bat_move;
      currentInst = &Bat_inst;

      if(1){state = 31;}
    break;


    case 30: 
      currentMove = &Tosse_move;
      currentInst = &Tosse_inst;

      if(1){state = 31;}
    break;


    case 31:
      n++;

      if(ST_Command_IN[n] == 'T' || ST_Command_IN[n] == 't'){state = 32;} //Periodo
      if(ST_Command_IN[n] == 'P' || ST_Command_IN[n] == 'p'){state = 33;} //Pausa
      if(ST_Command_IN[n] == 'M' || ST_Command_IN[n] == 'm'){state = 34;} //Movimento
    break;


    case 32: 
      n++;
      m = 0;

      if(ST_Command_IN[n] == '['){state = 32;}
      if(ST_Command_IN[n] != '['){state = 35;}
    break;


    case 33: 
      //L
    break;


    case 34: 
      //L
    break;


    case 35: 
      ST_copy[m]=ST_Command_IN[n];
      n++;
      m++;

      if(ST_Command_IN[n] != ']'){state = 35;}
      if(ST_Command_IN[n] == ']'){state = 36;}
    break;


    case 36: 
      //L
      currentMove->period=atof(ST_copy);
      memset(ST_copy,0,sizeof(ST_copy));
      LineReady_Out = true;

      if(1){state = 37;}
    break;

    case 37:
    Mode = 'S';

    if(1){state = 4;}
    break;
    }
    
    return;
}






void setup() {

  Serial.begin(115200);

  setup_counter++;
  Serial.print("setup chamado: ");
  Serial.println(setup_counter);
  
  Serial.println("Teste LED RGB");

  pixel.begin();
  pixel.clear();
  pixel.show();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  bool servo_ok = attach_servos();

  delay(1000);
  if (servo_ok) {
    Serial.println("Servos attached successfully");
  } else {
    Serial.println("Error attaching servos");
  }
  
  //----------------Inicialização do movimento de teste-----------------
    Iniciate_Bat_Move();
    Iniciate_Resp_Move();
    Iniciate_Bat_Instance();
    Iniciate_Resp_Instance();
    Calcular_Index0(Resp_move, Index0);
    //Serial.println("Setup completo, pronto para correr Sequence.");
    
}



void loop() {
//Serial.println("beep");
//delay(1000);
    
static boolean trig_serial_IN; static boolean trig_serial_OUT;
float actual_time = millis();
float tempo_inicial = millis();

  
    unsigned long now1 = millis();
    trig_serial_OUT = trig_display_fsm(actual_time,50);
    FSM_Serial_reader(now1);
    FSM_Serial_Command(now1);

    Preparacao_Tosse();

    unsigned long now2 = millis();
    // --- Apenas um comentario temporario, -> tenho que ativar denovo isto mais tarde!!
    FSM_Motion_Update(start_comand , Bat_inst, now2);
    unsigned long now3 = millis();
    Intermed_position(start_comand ,Bat_inst, now3, Lerp_Batim_OUT, trig_serial_IN);
    FSM_Motion_Update(start_comand , Resp_inst, now2);
    Intermed_position(start_comand ,Resp_inst, now3, Lerp_Respi_OUT, trig_serial_IN);


    bool ik1 = Fusao_plus_IK_D1(Lerp_Respi_OUT, Lerp_Batim_OUT);
    bool ik2 = Fusao_plus_IK_D2(Lerp_Respi_OUT, Lerp_Batim_OUT);

    if (ik1 && ik2) {
      move_servos(delta_1_Cfg, delta_2_Cfg);
    }





float tempo_final = millis();
float demora = tempo_final - tempo_inicial;
Clock_loop++;
//Serial.print("Clock loop: ");
   // Serial.println(Clock_loop);
if(trig_serial_IN == 1 && Safe_comand == 1){
  
    //Serial.print("Estado da leitura:");
    //Serial.println(state_test);
    //Serial.print("Estado da Interpertação:");
    //Serial.println(state_test2);
    Serial.print("X:");
    Serial.println(Lerp_Respi_OUT.X);
    Serial.print("          Y:");
    Serial.println(Lerp_Respi_OUT.Y);
    Serial.print("                    Z:");
    Serial.println(Lerp_Respi_OUT.Z);
 

  }
trig_serial_IN = trig_serial_OUT;
LineReady_IN=LineReady_Out;
CH_Command_IN=CH_Command_Out;
strncpy(ST_Command_IN, ST_Command_Out, sizeof(ST_Command_IN));
ST_Command_IN[sizeof(ST_Command_IN)-1] = '\0';
}




/*------------------------------------------------------------------------------------------------------------------------------------------------------*/

bool attach_servos(void){ //Serve apenas para definir os valores de PWM maixos e minimos, estes foram calibrados mais em cima no codigo 
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

  return true;
}

bool inverse_kinematics(Manipulador_Config& cfg, float xt, float yt, float zt){    
    float servo_Theta1_angle; 
    float servo_Theta2_angle;
    float servo_Theta3_angle;

  
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
void move_servos(const Manipulador_Config& d1, const Manipulador_Config& d2){
    mg90s_1.writeMicroseconds(d1.servo1Pulse);
    mg90s_2.writeMicroseconds(d1.servo2Pulse);
    mg90s_3.writeMicroseconds(d1.servo3Pulse);

    mg90s_4.writeMicroseconds(d2.servo1Pulse);
    mg90s_5.writeMicroseconds(d2.servo2Pulse);
    mg90s_6.writeMicroseconds(d2.servo3Pulse);
    //mg90s_6.writeMicroseconds(420);
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



void Intermed_position(boolean start, MotionInstance& inst, unsigned long nowMs, Coordinate& xyz, boolean trig_serial) {
    MotionStorage& m = *(inst.def);

    if (!inst.active || m.n_Points <= 1 || m.period <= 0.0f || (start == 0 && inst.Executing == 0)) {
        xyz.X = 0.0f;
        xyz.Y = 0.0f;
        xyz.Z = 0.0f;
        return;
    }

    int i = inst.iIndex;
    int j = inst.jIndex;
    float elapsed = (float)inst.elapsed_1;

    float alpha = elapsed / m.dtMs;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    // posição bruta entre os pontos i e j
    float X_lerp = lerp(m.X[i], m.X[j], alpha);
    float Y_lerp = lerp(m.Y[i], m.Y[j], alpha);
    float Z_lerp = lerp(m.Z[i], m.Z[j], alpha);

    // coordenadas relativas ao ponto inicial (Index0 pré-calculado no setup)
    xyz.X = X_lerp - Index0.X;
    xyz.Y = Y_lerp - Index0.Y;
    xyz.Z = Z_lerp - Index0.Z;

    return;
}

void Calcular_Index0(MotionStorage& m, Coordinate& idx0) {
    // usa o start_Index já existente no movimento
    int startIdx = m.start_Index;

    if (startIdx >= 0 && startIdx < m.n_Points) {
        idx0.X = m.X[startIdx];
        idx0.Y = m.Y[startIdx];
        idx0.Z = m.Z[startIdx];
    } else {
        // fallback seguro se start_Index estiver fora do intervalo
        idx0.X = 0.0f;
        idx0.Y = 0.0f;
        idx0.Z = 0.0f;
    }
}



/*
void FSM_Motion_Update(boolean start, MotionInstance& inst, unsigned long nowMs){
MotionStorage& m = *(inst.def);

Têm em atenção constituição de cada caso:
case x: 
  - Ações a executar
  - Condições de transição por hierarquia
//
//Serial.print("Estado atual: ");
//Serial.println(inst.state);

switch (inst.state) {

    case 0:
      inst.Executing = 0;
      inst.iIndex = 0;
      inst.jIndex = 0;

      if (start==0) {inst.state = 0;}
      if (!inst.active || inst.def == nullptr) {inst.state = 0;}
      if(start==1 && inst.active && inst.def != nullptr){inst.state = 1;}
    break;

    case 1:
      if (m.n_Points <= 1 || m.period <= 0.0f) {inst.state = 1;}
      else {inst.state = 2;}
    break;

    case 2:
      inst.segmentStartMs = nowMs;
      inst.jIndex = (inst.iIndex + 1) % m.n_Points;
      inst.Executing = 1;

      if (inst.Direction == 1) {inst.state = 3;}
    break;

    case 3: 
      inst.elapsed_1 = nowMs - inst.segmentStartMs;

      if(inst.elapsed_1 >= m.dtMs) {inst.state = 4;}
    break;

    case 4: 
      inst.iIndex++;
      inst.jIndex = (inst.iIndex + 1) % m.n_Points;
      inst.elapsed_1 = 0;
      inst.segmentStartMs = nowMs;

      if(inst.iIndex < m.n_Points -1) {inst.state = 3;}
      if(inst.iIndex >= m.n_Points -1 ) {inst.state = 5;}
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
*/


void FSM_Motion_Update(boolean start, MotionInstance& inst, unsigned long nowMs){
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
      inst.Executing = 0;
      inst.I_Index=0;
      inst.iIndex = 0;
      inst.jIndex = 0;
      inst.Situacion='F';

      if (start==0 || inst.Individual_start_comand == 0) {inst.state = 0;}
      if (!inst.active || inst.def == nullptr) {inst.state = 0;}
      if(start==1 && inst.Individual_start_comand == 1 && inst.active && inst.def != nullptr){inst.state = 1;}
    break;

    case 1:
      inst.I_Index=0;
      inst.Situacion='P';
      inst.Direction=1;
      //Serial.println("-----------------------------------------------intermed state");
      if (m.n_Points <= 1 || m.period <= 0.0f) {inst.state = 1;}
      if(inst.Direction==1 && start==1 && inst.Individual_start_comand == 1){inst.state = 2;}
      if(start==0 || inst.Individual_start_comand == 0){inst.state = 0;}
    break;

    case 2:
      inst.Executing = 1;
      inst.Situacion='M';
      inst.segmentStartMs = nowMs;
      inst.iIndex = m.start_Index;
      inst.End_Point = m.n_Points-1;
      inst.jIndex = (inst.iIndex + inst.Direction) % m.n_Points;
      

      if (1) {inst.state = 3;}
    break;

    case 3: 
      inst.elapsed_1 = nowMs - inst.segmentStartMs;

      if(inst.elapsed_1 >= m.dtMs) {inst.state = 4;}
    break;

    case 4: 
      inst.I_Index++;
      inst.iIndex = (inst.iIndex + inst.Direction);
      inst.jIndex = (inst.iIndex + inst.Direction);
      inst.elapsed_1 = 0;
      inst.segmentStartMs = nowMs;

      if((inst.iIndex < inst.End_Point && inst.Direction == 1) || (inst.iIndex > inst.End_Point && inst.Direction == -1)) {inst.state = 3;}
      //if(inst.iIndex > inst.End_Point && inst.Direction == -1) {inst.state = 3;}
      if((inst.iIndex >= inst.End_Point && inst.Direction == 1) || (inst.iIndex <= inst.End_Point && inst.Direction == -1)) {inst.state = 5;}
      //if(inst.iIndex <= inst.End_Point && inst.Direction == -1) {inst.state = 5;}
      //if(inst.I_Index == m.N_points && m.bidirectional == true){inst.state = 1;}
      if(m.bidirectional == true && inst.iIndex == m.start_Index && inst.jIndex == m.start_Index + 1){inst.state = 1;}
      if(Tosse_signal == 1 && m.bidirectional == true){inst.state = 14;}
    break;

    case 5: //função trás
      inst.segmentStartMs = nowMs;
      inst.jIndex = inst.iIndex;

      if (inst.Direction == -1 && m.Dt_pauseMs_Ini != 0) {inst.state = 7;}
      else if (inst.Direction == 1 && m.Dt_pauseMs_End != 0) {inst.state = 8;}
      else {
        if (m.bidirectional == true) { inst.state = 6; }
        else if (m.bidirectional != true && m.Dt_pauseMs_Ini != 0 && m.Dt_pauseMs_End != 0) { inst.state = 1; }
      }
    break;


    case 6: 
      inst.Direction = inst.Direction * (-1);

      if(inst.Direction == -1) {inst.state = 11;}
      if(inst.Direction == 1) {inst.state = 12;}
    break;

    case 7: 
      inst.Dt_pause = m.Dt_pauseMs_Ini;

  
      if(1) {inst.state = 9;}
    break;


    case 8: //função trás
      inst.Dt_pause = m.Dt_pauseMs_End;

      if(1) {inst.state = 9;}
    break;

    case 9: //função trás
      inst.elapsed_2 = nowMs - inst.segmentStartMs;
   
      if(inst.elapsed_2 >= inst.Dt_pause) {inst.state = 10;}

    break;

    case 10: //função trás
      inst.segmentStartMs = nowMs;
      inst.elapsed_2 = 0;

      if(start==0 || inst.active != true || inst.Individual_start_comand == 0) {inst.state = 0;}
      if(m.bidirectional == true) {inst.state = 6;}
      if(m.bidirectional == false){inst.state = 1;}
    break;

    case 11:
      inst.iIndex = m.n_Points-1;
      inst.jIndex = inst.iIndex + inst.Direction;
      inst.End_Point = 0;
      //Serial.println("Descida");
      if(1){inst.state = 13;}
    break;


    case 12:
      inst.iIndex = 0;
      inst.jIndex = inst.iIndex + inst.Direction;
      inst.End_Point = m.n_Points-1;
      //Serial.println("Subida");
      if(1){inst.state = 13;}
    break;


    case 13:
      inst.segmentStartMs = nowMs;

      if(1){inst.state=3;}
    break;

    case 14:
      Tosse_signal=0;
      Serial.println("Tosse executada");
      inst.Direction = (-1);
      inst.jIndex = inst.iIndex + inst.Direction;
      inst.End_Point = 0;

      if(1){inst.state=13;}
    }
    return;
}


bool Fusao_plus_IK_D1(const Coordinate& respi, const Coordinate& batim) {
    float X_total = 1.0f * respi.X + 0.2f * batim.X + -15.0f + Index0.X;
    float Y_total = 1.0f * respi.Y + 0.2f * batim.Y + -10.0f + Index0.Y;
    float Z_total = 1.0f * respi.Z * Resp_inst.Z_amp_mult + 0.2f * batim.Z +  50.0f + Index0.Z;

    bool ik_ok = inverse_kinematics(delta_1_Cfg, X_total, Y_total, Z_total);
    return ik_ok;
}

bool Fusao_plus_IK_D2(const Coordinate& respi, const Coordinate& batim) {
    float X_total = 1.0f * respi.X + 0.0f * batim.X + -15.0f + Index0.X;
    float Y_total = 1.0f * respi.Y + 0.0f * batim.Y + -10.0f + Index0.Y;
    float Z_total = 1.0f * respi.Z * Resp_inst.Z_amp_mult + 0.0f * batim.Z +  50.0f + Index0.Z;

    bool ik_ok = inverse_kinematics(delta_2_Cfg, -1.0f * X_total, Y_total, Z_total);
    return ik_ok;
}


void Preparacao_Tosse() {
    if (start_comand == 0) {
        sinal_tossir         = false;
        Tosse_em_preparacao  = false;
        Tosse_signal         = 0;
        Resp_inst.Z_amp_mult = 1.0f;   // garantir reset se programa parar
        return;
    }

    short int estadoResp = Resp_inst.state;

    // 1) novo sinal de tosse, ainda não preparado
    if (sinal_tossir && !Tosse_em_preparacao) {
        if (!Resp_inst.active || Resp_inst.def == nullptr) {
            sinal_tossir = false;
            return;
        }

        // copiar valores padrão
        Resp_period_padrao      = Resp_move.period;
        Resp_T_pause_Ini_padrao = Resp_move.T_pause_Ini;

        // alterar para tosse
        Resp_move.period      = 0.50f;   // segundos
        Resp_move.T_pause_Ini = 0.1f;   // segundos
        Resp_inst.Z_amp_mult  = 1.50f;  // aumentar amplitude em Z

        Resp_move.updateDtMs();
        Resp_move.updateDt_PauseMs();

        Tosse_signal        = 1;
        Tosse_em_preparacao = true;

        return;
    }

    // 2) fim da tosse: FSM voltou ao state 1
    if (Tosse_em_preparacao && estadoResp == 1) {
        Resp_move.period      = Resp_period_padrao;
        Resp_move.T_pause_Ini = Resp_T_pause_Ini_padrao;
        Resp_inst.Z_amp_mult  = 1.0f;   // volta a 1.0

        Resp_move.updateDtMs();
        Resp_move.updateDt_PauseMs();

        Tosse_signal        = 0;
        Tosse_em_preparacao = false;
        sinal_tossir        = false;
    }
}


void debugPrintMove(const MotionStorage& move, const char* name) {
    Serial.print("=== ");
    Serial.print(name);
    Serial.println(" contents ===");

    Serial.print("nPoints: ");
    Serial.println(move.n_Points);
    Serial.print("period (sec): ");
    Serial.println(move.period);
    Serial.print("dtMs: ");
    Serial.println(move.dtMs);
    Serial.print("T_pause (sec): ");
    //Serial.println(move.T_pause);
    Serial.print("dt_PauseMs: ");
    //Serial.println(move.dt_pauseMs);

    for (int i = 0; i < move.n_Points; i++) {
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

