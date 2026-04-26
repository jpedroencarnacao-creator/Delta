#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <math.h>


//Link lengths (mm)
#define L1 50
#define L2 90
#define L3 15

#define END_EFFECTOR_Z_OFFSET 0
#define SERVO_OFFSET_X 32
#define SERVO_OFFSET_Y 0
#define SERVO_OFFSET_Z (0.0 + END_EFFECTOR_Z_OFFSET)
//#define SERVO_OFFSET_Z_INVERTED -293

#define SERVO_ANGLE_MIN 0.78539816339744830961566084581988f //45 degrees
#define SERVO_ANGLE_MAX 3.9269908169872415480783042290994f //225 degrees

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
//-----------------------------------------------------------------------------------------

#define INVERTED -1


struct Coordinate_f {
    float x;
    float y;
    float z;
};

//float posX = 0.0F;
//float posY = 0.0F;
//float posZ = 62.0F;

float posX[] = {0.0,  15.0,   15.0,    0.0,   -15.0,  -15.0,     -15.0,     0.0,   15.0,   15.0,    15.0,    0.0,   -15.0,  -15.0,   -15.0,    0.0,    15.0,    15.0,    15.0,    0.0,   -15.0,   -15.0,     -15.0,     0.0,   15.0,  15.0,   15.0,    0.0,   -15.0,  -15.0};
float posY[] = {0.0,  0.0,   15.0,    15.0,   15.0,    0.0,     -15.0,    -15.0,  -15.0,   0.0,    15.0,    15.0,   15.0,    0.0,   -15.0,   -15.0,   -15.0,    0.0,    15.0,    15.0,    15.0,    0.0,     -15.0,    -15.0,  -15.0,  0.0,   15.0,    15.0,   15.0,    0.0, };
float posZ[] = {70.0, 72.0,  74.0,  76.0,   78.0,   80.0,    85.0,    90.0,  100.0, 105.0,  110.0, 112.0, 114.0,  116.0,  118.0,  120.0,  118.0,  116.0,  114.0,  112.0,  110.0,  105.0,    100.0,    90.0,  85.0,  80.0, 78.0,   76.0,   74.0,   72.0};
int Step_delay_t[] = {0, 0,  0,  0,   0,   0,    0,    0,  0};

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


//---------Declaração das variaveis------------------
int delta_calcForward(float theta1, float theta2, float theta3, float &x0, float &y0, float &z0);
int delta_calcAngleYZ(float x0, float y0, float z0, float &theta);
int delta_calcInverse(float x0, float y0, float z0, float &theta1, float &theta2, float &theta3);
float deltakinematic(float posX, float posY, float posZ, char servo);
void readJoystickButtons();
void LED_LOOP(); //Declaração de funções
void Servo_test();
//-------Novas-------------
float boundFloat(float, float, float);
void attach_servos(void);
bool inverse_kinematics_1(float, float, float);
bool inverse_kinematics_2(float, float, float);
bool inverse_kinematics_3(float, float, float);
bool inverse_kinematics(float, float, float);
void linear_move(float, float, float, float, int);
void joint_move(float, float, float, int, int);
void move_servos(void);
double mapNumber(double x, double in_min, double in_max, double out_min, double out_max);
int roundMapNumber(double x, double in_min, double in_max, double out_min, double out_max);
double degToRads(double deg);
double radsToDeg(double rads);
//----------------------------------------------------

Servo mg90s_1;  // cria objeto servo
Servo mg90s_2;  // cria objeto servo
Servo mg90s_3;  // cria objeto servo

Coordinate_f end_effector;
Coordinate_f home_position;

float servo_1_angle;
float servo_2_angle;
float servo_3_angle;


int servo_1_pulse_count = 0;
int servo_2_pulse_count = 0;
int servo_3_pulse_count = 0;
int servo_4_pulse_count = 0;

int step_delay_linear = 2; //0ms 
float step_increment = 0.5; //0.4mm
int step_pulses = 4; //1us increments
int step_delay_joint = 1; //3ms 
byte axis_direction = 0; 
float servo_offset_z = SERVO_OFFSET_Z;

//-----------------Declaração da localização dos pinos para cada objeto ------------------
//-------------------Servos-----------------
#define SERVO_PIN_1 19
#define SERVO_PIN_2 20 
#define SERVO_PIN_3 21  

//----------------Led_Informação------------
#define LED_PIN 38
#define LED_COUNT 1

// joytsick
#define BTN_UP     4
#define BTN_DOWN   5
#define BTN_LEFT   6
#define BTN_RIGHT  7
#define BTN_MIDLE  8
#define BTN_SET    9
#define BTN_RESET  10

//----------STEPPER SIMPLES------------------
//#define STEP_PIN 19
//#define DIR_PIN  21
//#define ENABLE_PIN 22

//------------------------------------------------------------------------------------------
bool stepDirection = false;
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);



void setup() {
  Serial.begin(921600);
  

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

    end_effector.x=0.0;
    end_effector.y=0.0;
    end_effector.z=60.1;

  Serial.println("Teste MG90S Servo");
  // Configurações específicas para servo MG90S (50Hz, pulsos 500-2400us)
  mg90s_1.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_1.attach(SERVO_PIN_1, SERVO_1_MIN, SERVO_1_MAX);  

  mg90s_2.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_2.attach(SERVO_PIN_2, SERVO_2_MIN, SERVO_2_MAX);  

  mg90s_3.setPeriodHertz(50);      // Frequência PWM 50 Hz
  mg90s_3.attach(SERVO_PIN_3, SERVO_3_MIN, SERVO_3_MAX); 

  delay(1000);
  Serial.println("Delta robot kinematics ready");
}


void loop() {

  float theta1, theta2, theta3;
    float x, y, z;
  x = 0.0;    //minimo 0.0
  y = 30.0;   //minimo 0.0
  z = 80.1;   //minimo 61.2

  //printf("\n dentro do loop"); 
  //readJoystickButtons();
  //LED_LOOP();
  //Servo_test();
  //mg90s_1.writeMicroseconds(SERVO_1_MIN);  //thetta3 min 550, max 2450
  //mg90s_2.writeMicroseconds(SERVO_2_MIN);  //thetta1 min 545, max 2410
  //mg90s_3.writeMicroseconds(SERVO_3_MIN);  //thetta2 min 520, max 2450
   
  /* //------------test de cinematica só com inverse_kinematics--------------
  for (int i = 0; i < 9; i++) {
    bool verification = inverse_kinematics(posX[i], posY[i], posZ[i]);
  if(verification == 1){
    printf("\n Success Inverse Kinematics");
    printf("\n Angulo do servo 1: %f", servo_1_angle);
    printf("\n Angulo do servo 2: %f", servo_2_angle);
    printf("\n Angulo do servo 3: %f", servo_3_angle);
  }
  if(verification == 0){
    printf("\n Erro Inverse Kinematics");
  }
  mg90s_1.writeMicroseconds(servo_1_pulse_count); 
  mg90s_2.writeMicroseconds(servo_2_pulse_count);
  mg90s_3.writeMicroseconds(servo_3_pulse_count);
  delay(5000);
  }
  */

  for(int i = 0; i < 30; i++){
            joint_move(posX[i], posY[i], posZ[i], step_pulses, step_delay_joint);
            //linear_move(posX[i], posY[i], posZ[i], step_increment, step_delay_linear );
            //delay(Step_delay_t[i]);
            end_effector.x=posX[i];
            end_effector.y=posY[i];
            end_effector.z=posZ[i];
        }


/* //--------------old_kinematics-----------------------
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
  }*/
 delay(1);
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

void attach_servos(void){
    mg90s_1.attach(SERVO_PIN_1, SERVO_1_MIN, SERVO_1_MAX); // TODO: set correct min/max values
    mg90s_2.attach(SERVO_PIN_2, SERVO_2_MIN, SERVO_2_MAX);
    mg90s_3.attach(SERVO_PIN_1, SERVO_3_MIN, SERVO_3_MAX);
}

/*------------------------------------------------------------------------------------------------------------------------------------------------------*/

bool inverse_kinematics_1(float xt, float yt, float zt){
    //printf("\n x= %f, y=%f, z=%f", xt, yt, zt);
    zt -= servo_offset_z; //Remove the differance in height from ground level to the centre of rotation of the servos
    
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
//printf("\n zt= %f", zt);
//printf("\n SERVO_OFFSET_X= %f", SERVO_OFFSET_X);
//printf("\n arm_end_x= %f", arm_end_x);
    float ext = sqrt(pow (zt, 2) + pow(SERVO_OFFSET_X - arm_end_x, 2)); //Extension of the arm from the centre of the servo rotation to the end ball joint of link2
//printf("\n ext= %f", ext);
//printf("\n l2p - L1= %f", l2p - L1);
//printf("\n L1 + l2p= %f", L1 + l2p);
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

/*------------------------------------------------------------------------------------------------------------------------------------------------------*/

bool inverse_kinematics_2(float xt, float yt, float zt){
    zt -= servo_offset_z;
    float x = xt;
    float y = yt;
    xt = x * cos(2.0943951023931954923084289221863f) - y * sin(2.0943951023931954923084289221863f); //Rotate coordinate frame 120 degrees
    yt = x * sin(2.0943951023931954923084289221863f) + y * cos(2.0943951023931954923084289221863f);
    
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

/*------------------------------------------------------------------------------------------------------------------------------------------------------*/

bool inverse_kinematics_3(float xt, float yt, float zt){
    zt -= servo_offset_z;

    float x = xt;
    float y = yt;
    xt = x * cos(4.1887902047863909846168578443727f) - y * sin(4.1887902047863909846168578443727f); //Rotate coordinate frame 240 degrees
    yt = x * sin(4.1887902047863909846168578443727f) + y * cos(4.1887902047863909846168578443727f);

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

/*------------------------------------------------------------------------------------------------------------------------------------------------------*/

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

void linear_move(float x1, float y1, float z1, float stepDist, int stepDelay){//interpolates between two points to move in a stright line (beware of physical and kinematic limits)
    //Sets the initial position variables
    float x0 = end_effector.x;
    float y0 = end_effector.y;
    float z0 = end_effector.z;
    
    //Distance change in each axis
    float xDist = x1 - x0;
    float yDist = y1 - y0;
    float zDist = z1 - z0;
    
    double totalDist = sqrt(sq(xDist) + sq(yDist) + sq(zDist));//Absolute magnitute of the distance
    int numberOfSteps = round(totalDist / stepDist);//Number of steps required for the desired step distance

    //Step size of each axis
    if(numberOfSteps == 0){
        //printf("ERROR: No change in position: numberOfSteps = ", numberOfSteps);
        return;
    }
    
    float xStep = xDist / (float)numberOfSteps;
    float yStep = yDist / (float)numberOfSteps;
    float zStep = zDist / (float)numberOfSteps;

    //Interpolation variables
    float xInterpolation;
    float yInterpolation;
    float zInterpolation;

    for(int i = 1; i <= numberOfSteps; i++){//Interpolate the points
        xInterpolation = x0 + i * xStep;
        yInterpolation = y0 + i * yStep;
        zInterpolation = z0 + i * zStep;

        inverse_kinematics(xInterpolation, yInterpolation, zInterpolation);//calculates the inverse kinematics for the interpolated values
        //printf("\n thetta3 Angulo do servo 1: %f", radsToDeg(servo_1_angle));
    //printf("\n thetta1 Angulo do servo 2: %f", radsToDeg(servo_2_angle));
    //printf("\n thetta2 Angulo do servo 3: %f", radsToDeg(servo_3_angle));
        move_servos();
        delay(stepDelay);
    }
}

void joint_move(float xt, float yt, float zt, int stepPulses, int stepDelay){//interpolates between the current and tartget joint pule counts. All joints will reach the target position at the same time.
    if(stepPulses <= 0) return;//Checks that the step size if valid
    
    //Sets variables to store the current pulse counts
    int servo1PulseCount0 = servo_1_pulse_count;
    int servo2PulseCount0 = servo_2_pulse_count;
    int servo3PulseCount0 = servo_3_pulse_count;

    if(inverse_kinematics(xt, yt, zt) == false) return;//Exit the function if the position is out of the workspace
   
    //Sets variables to store the differences in pulse counts
    int servo1PulseDiff = servo_1_pulse_count - servo1PulseCount0;
    int servo2PulseDiff = servo_2_pulse_count - servo2PulseCount0;
    int servo3PulseDiff = servo_3_pulse_count - servo3PulseCount0;
    
    int maxDiff;

    //Gets the biggest difference in pulse count
    if(abs(servo1PulseDiff) >= abs(servo2PulseDiff) && abs(servo1PulseDiff) >= abs(servo3PulseDiff)){
        maxDiff = abs(servo1PulseDiff);
    }
    else if(abs(servo2PulseDiff) >= abs(servo1PulseDiff) && abs(servo2PulseDiff) >= abs(servo3PulseDiff)){
        maxDiff = abs(servo2PulseDiff);
    }
    else if(abs(servo3PulseDiff) >= abs(servo1PulseDiff) && abs(servo3PulseDiff) >= abs(servo2PulseDiff)){
        maxDiff = abs(servo3PulseDiff);
    }
    else{
//        printi("ERROR: No difference in pulse counts. ");
        return;
    }

    float servo1Step = (float)servo1PulseDiff / (float)maxDiff;
    float servo2Step = (float)servo2PulseDiff / (float)maxDiff;
    float servo3Step = (float)servo3PulseDiff / (float)maxDiff;

    for(int i = 1; i <= maxDiff; i += stepPulses){
        servo_1_pulse_count = round(servo1PulseCount0 + i * servo1Step);
        servo_2_pulse_count = round(servo2PulseCount0 + i * servo2Step);
        servo_3_pulse_count = round(servo3PulseCount0 + i * servo3Step);
        move_servos();
        if(stepDelay > 0) delay(stepDelay);//If there is a delay then delay
    }

    //Sets the correct final position
    servo_1_pulse_count = servo1PulseCount0 + servo1PulseDiff;
    servo_2_pulse_count = servo2PulseCount0 + servo2PulseDiff;
    servo_3_pulse_count = servo3PulseCount0 + servo3PulseDiff;
    move_servos();
    delay(stepDelay);
}

void move_servos(void){
    mg90s_1.writeMicroseconds(servo_1_pulse_count);
    mg90s_2.writeMicroseconds(servo_2_pulse_count);
    mg90s_3.writeMicroseconds(servo_3_pulse_count);

}


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

//------------------------obselet------------------------------------
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

