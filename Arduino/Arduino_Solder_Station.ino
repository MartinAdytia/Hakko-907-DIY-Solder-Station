#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdio.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//oled
char buf[30];
char float_val[7];

bool enc_a;
uint16_t ADC_readings[3];
float A, V, T;
//pid
uint8_t pwm_val;
float sum_error;
int16_t Kp = 10, Ki = 0.8;
bool changing_T = 0;
uint16_t target_T = 25;
uint16_t target_T_sel = 25;

//rtos
uint32_t oled_task_time = 0;
uint32_t pid_task_time = 0;
uint32_t esp_send_task_time = 0;
uint32_t esp_rec_task_time = 0;
void oled_task();
void pid_task();
void sw_task();
void esp_send_task();
void esp_rec_task();

//encoder
uint32_t sw_time = 4294967295;
void a_change();
void sw_change();

//esp
String inputString = "";
char tempstr[4] = "000";
bool stringComplete = false;
int temp_esp;
int temp_esp_old;
int P = 0;


void setup() {
  digitalWrite(9, 1);
  pinMode(9, OUTPUT);  //solder pwm

  pinMode(2, INPUT);  //encoder a
  pinMode(3, INPUT_PULLUP);  //encoder sw
  pinMode(5, INPUT);  //encoder b

  Serial.begin(9600);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    //Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);

  attachInterrupt(digitalPinToInterrupt(2), a_change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), sw_change, CHANGE);
}

void loop() {
  //Serial.print("Real target temp= ");
  //Serial.println(target_T);
  if (millis() > oled_task_time) oled_task();
  if (millis() > pid_task_time) pid_task();
  if (stringComplete & millis() > esp_rec_task_time) esp_rec_task();    
  if (millis() > esp_send_task_time) esp_send_task();
  if (millis() > sw_time) sw_task();
}

void oled_task() {
  oled.clearDisplay();

  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);

  oled.setCursor(2, 5);
  if(changing_T){
    sprintf(buf, "Ts %dc", target_T_sel);
  }
  else {
    sprintf(buf, "T %dc", (int16_t)T);
  }

  oled.print(buf);

  oled.setCursor(2, 25);
  // dtostrf(A, 0, 1, float_val);
  // sprintf(buf, "A %sA", float_val);
  dtostrf(A * V, 0, 1, float_val);
  P = A * V;
  sprintf(buf, "P %sw", float_val);
  oled.print(buf);

  oled.setCursor(2, 45);
  // dtostrf(V, 0, 1, float_val);
  dtostrf(pwm_val/2.55, 0, 1, float_val);
  sprintf(buf, "PWM %s%%", float_val);
  oled.print(buf);

  oled.display();
  oled_task_time = millis() + 100;
}

void pid_task() {
  ADC_readings[0] = analogRead(A0);  //T
  ADC_readings[1] = analogRead(A1);  //A
  ADC_readings[2] = analogRead(A2);  //V
  // delay(2);

  A = (ADC_readings[1] * 0.00602) - 0.01411;
  V = (ADC_readings[2] * 0.02741) - 0.03237;
  T = (ADC_readings[0] * 1.28189) - 212.751;

  int error = (target_T - (uint16_t) T);

  if(abs(error) < 25) sum_error += error * Ki;

  if(sum_error > 255) sum_error = 255;
  if(sum_error < 0) sum_error = 0;

  int out = error * Kp + sum_error;

  if(out < 0) out = 0;
  if(out > 255) out = 255;

  pwm_val = out;
  analogWrite(9, pwm_val);

  pid_task_time = millis() + 20;
}

void a_change() {
  changing_T = 1;
  enc_a = digitalRead(2);

  if (enc_a == 0) {
    if (target_T_sel > 0 && digitalRead(5) == 1) target_T_sel -= 5;
    else if (target_T_sel < 450 && digitalRead(5) == 0) target_T_sel += 5;
  } 
  else if (enc_a == 1) {
    if (target_T_sel > 0 && digitalRead(5) == 0) target_T_sel -= 5;
    else if (target_T_sel < 450 && digitalRead(5) == 1) target_T_sel += 5;
  }
}

void sw_change(){
  detachInterrupt(digitalPinToInterrupt(3));
  sw_time = millis() + 100;

  if(digitalRead(3) == 0){
    target_T = target_T_sel;
    changing_T = 0;
  }
}

void sw_task(){
  sw_time = 4294967295;
  attachInterrupt(digitalPinToInterrupt(3), sw_change, CHANGE);
}

void esp_send_task(){
  Serial.print("~");
  Serial.print(int(T));
  Serial.print("  #");
  Serial.print(target_T);
  Serial.print("  ^");
  Serial.print(int(pwm_val/2.55));
  Serial.print("  @");
  Serial.print(P);
  Serial.println(" ");
  esp_send_task_time = millis() + 200;
}

void esp_rec_task(){
    temp_esp_old = temp_esp;
    for(int i=0; i<=19; i++){
      if(inputString[i] == '~'){
        for(int n=1; n<=4; n++){
          if(inputString[i+n]!='|'){
            if(inputString[i+n]>47 & inputString[i+n]<58){
              tempstr[n-1]=inputString[i+n];
           }
            else{tempstr[n-1]='\0';}
          }
          else{tempstr[n-1]='\0';}
        }
        sscanf(tempstr, "%d", &temp_esp);
      }
    }
    inputString = "";
    stringComplete = false;
  if (temp_esp_old != temp_esp) target_T = temp_esp;
  esp_send_task_time = millis() + 100;
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
