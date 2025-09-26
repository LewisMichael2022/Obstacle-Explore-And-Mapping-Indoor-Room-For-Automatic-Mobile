#include <QMC5883LCompass.h>
#include <ArduinoSTL.h>
#include <SoftwareSerial.h>
/*
arduino => 馬達控制、六個超聲波 
wifi => 四個碰撞
*/

/*
1.馬達控制 2.超聲波偵測 3.碰撞感測器控制 4.羅盤 5.互相傳輸的資料
*/
//卡住時，先reset wifi再reset arduino

//超聲波相關
#define detect_limit_dis 20
//超聲波左前:12、A3
#define trig_FL 13
#define echo_FL A0

//超聲波中前:4、3
#define trig_FM 12
#define echo_FM A1

//超聲波右前:2、11 
#define trig_FR 11
#define echo_FR A2

//車子馬達相關 ENA:6 ENB:5  IN4(RB):7 IN3(RF):8 IN2(LF):9 IN1(LB):10
#define ENA 6//Rpwm_pin 棕色
#define ENB 5//Lpwm_pin 藍
#define IN4 7 //綠
#define IN3 8 //黃
#define IN2 9 //橘
#define IN1 10 //紅
float dis_unit_time=650;//前進一格需多少毫秒
float left_unit_time=800;//ms
float right_unit_time=745;//ms

//碰撞感測器相關

#define crash_FL 4
#define crash_FR 3

/*馬達控制*/
byte Lpwm_val = 120;//初始化前進左轮速度
byte Rpwm_val = 120;//初始化前進右轮速度
byte LTLpwm_val=120;//初始化左轉左轮速度
byte LTRpwm_val=120;
byte RTLpwm_val=120;
byte RTRpwm_val=120;

byte pwm_max_variation=50;
byte pwm_max=Lpwm_val+pwm_max_variation;
byte pwm_min=Lpwm_val-pwm_max_variation;

//羅盤相關
QMC5883LCompass compass;
int dir_error_baseline=0;
int dir_error_limit=10;
int axis_dir;
int dir;

//循線相關
#define trace_pinL A3
#define trace_pinR 2
int trace_resL;
int trace_resR;
int forward_delay_time=3;

void M_Control_IO_config(void)//电机驱动板的IO初始化函数
{
  pinMode(IN4,OUTPUT); 
  pinMode(IN3,OUTPUT); 
  pinMode(IN2,OUTPUT); 
  pinMode(IN1,OUTPUT); 
  pinMode(ENB,OUTPUT); 
  pinMode(ENA,OUTPUT);   
}

void set_speed(unsigned char Left,unsigned char Right)//车速设定函数
{
  analogWrite(ENB,Left);   
  analogWrite(ENA,Right);
}
void set_stop(){
   digitalWrite(IN4,HIGH);  
   digitalWrite(IN3,HIGH);
   digitalWrite(IN1,HIGH);
   digitalWrite(IN2,HIGH);
}
void set_back(){
     digitalWrite(IN4,LOW);  
     digitalWrite(IN3,HIGH);
     digitalWrite(IN1,LOW); 
     digitalWrite(IN2,HIGH);
}
void set_front(){
   digitalWrite(IN4,HIGH);  
   digitalWrite(IN3,LOW);
   digitalWrite(IN1,HIGH);  
   digitalWrite(IN2,LOW);
}

void set_right(){
   digitalWrite(IN4,LOW);  
   digitalWrite(IN3,HIGH);
   digitalWrite(IN1,HIGH);
   digitalWrite(IN2,LOW); 
}

void set_left(){
   digitalWrite(IN4,HIGH);  
   digitalWrite(IN3,LOW);   
   digitalWrite(IN1,LOW);    
   digitalWrite(IN2,HIGH);
}

String forward(){//前進t秒時間，並記錄碰撞感測試否有偵測到物體
  set_speed(Lpwm_val,Rpwm_val);
  unsigned long now=millis();
  trace_resL=digitalRead(trace_pinL);
  trace_resR=digitalRead(trace_pinR);
  set_front();
  if(trace_resL==1&&trace_resR==1){
      //delay(500);
      while(millis()-now<500){
        if(crash_detect()){
            int back_time=millis()-now;
            set_back();
            now=millis();
            while(millis()-now<back_time);
            set_stop();
            return "2";
        }  
      }
  }
  trace_resL=digitalRead(trace_pinL);
  trace_resR=digitalRead(trace_pinR);
  int fix_count=0;
  int state=2;//froward
  while(!(trace_resL==1&&trace_resR==1)){
    trace_resL=digitalRead(trace_pinL);
    trace_resR=digitalRead(trace_pinR);
    if(crash_detect()){
      unsigned long back_time=millis()-now;
      set_back();
      now=millis();
      while(millis()-now<back_time){
          trace_resL=digitalRead(trace_pinL);
          trace_resR=digitalRead(trace_pinR);
          if(trace_resL==0&&trace_resR==1){
            set_left();  
          }
          else if(trace_resL==1&&trace_resR==0){
            set_right();
          }
          else{
            set_back();  
          }
      }
      set_stop();
      return "2";
    } 
    if(trace_resL==0&&trace_resR==1){
      set_right();
      if(state!=0)fix_count++; 
      state=0;   
    }
    else if(trace_resL==1&&trace_resR==0){
      set_left();
      if(state!=1)fix_count++;
      state=1;
    }
    else{
      set_front();
      state=2;  
    }
  }
  
  //時間計數修正
  set_front();
  unsigned long start_time=millis();
  now=start_time;
  while(now-start_time<fix_count*forward_delay_time){
    now=millis();
    if(trace_resL==0&&trace_resR==1){
      set_right();
    }
    else if(trace_resL==1&&trace_resR==0){
      set_left();
    }
    else{
      set_front();
    }
  }
  set_stop();
  return ultrasound_detect();
}

String turn_right(){//28左轉90 理想值:298 測量值316~318
  set_speed(RTLpwm_val,RTRpwm_val);
  trace_resL=digitalRead(trace_pinL);
  trace_resR=digitalRead(trace_pinR);
  if(trace_resR==1){
      set_right();
      /*while(trace_resR==1){
        trace_resL=digitalRead(trace_pinL);
        trace_resR=digitalRead(trace_pinR);
      }*/
      delay(500);
      trace_resL=digitalRead(trace_pinL);
      trace_resR=digitalRead(trace_pinR);
      while(trace_resR!=1){
        trace_resL=digitalRead(trace_pinL);
        trace_resR=digitalRead(trace_pinR);
      }
      set_stop();  
  }
  
  else{
    set_right();
    while(trace_resR!=1){
            trace_resL=digitalRead(trace_pinL);
            trace_resR=digitalRead(trace_pinR);
    }
    set_stop();  
  }
  
  /*if(trace_resR==1){
      set_right();
      while(trace_resR==1){
        trace_resL=digitalRead(trace_pinL);
        trace_resR=digitalRead(trace_pinR);
      }
      while(trace_resR!=1){
          trace_resL=digitalRead(trace_pinL);
          trace_resR=digitalRead(trace_pinR);
      }
      set_stop();
  }
  else{
    set_right();
    while(trace_resR!=1){
      trace_resL=digitalRead(trace_pinL);
      trace_resR=digitalRead(trace_pinR);
    }
    set_stop();
  }*/
  
  return ultrasound_detect(); 
}

String turn_left(){
  set_speed(LTLpwm_val,LTRpwm_val);
  trace_resL=digitalRead(trace_pinL);
  trace_resR=digitalRead(trace_pinR);
  if(trace_resL==1){
    set_left();
    /*while(trace_resL==1){
        trace_resL=digitalRead(trace_pinL);
        trace_resR=digitalRead(trace_pinR);
    }*/
    delay(500);
    trace_resL=digitalRead(trace_pinL);
    trace_resR=digitalRead(trace_pinR);
    while(trace_resL!=1){
        trace_resL=digitalRead(trace_pinL);
        trace_resR=digitalRead(trace_pinR);
    }
    set_stop();
  }
  else{
    set_left();
    while(trace_resL!=1){
        trace_resL=digitalRead(trace_pinL);
        trace_resR=digitalRead(trace_pinR);
    }
    set_stop();    
  }
  /*if(trace_resL==1){
      while(trace_resL==1){
        trace_resL=digitalRead(trace_pinL);
        trace_resR=digitalRead(trace_pinR);
      }
      while(trace_resL!=1){
        trace_resL=digitalRead(trace_pinL);
        trace_resR=digitalRead(trace_pinR);
      }
  }
  else{
    while(trace_resL!=1){
      trace_resL=digitalRead(trace_pinL);
      trace_resR=digitalRead(trace_pinR);
    }    
  }
  set_stop();*/
  return ultrasound_detect();
}
void backward(){
  
}

void check(){
    
}


//test
String test_forward(){//回傳直線前進30cm需要多久
  set_speed(Lpwm_val,Rpwm_val);
  set_front();
  float time_now=millis();
  while(1){
    if(crash_detect()){
       break;
      //return ;//代表碰撞有偵測到物體  
    }  
  }
  set_stop();
  float res=millis()-time_now;
  return String(res);  
}


String test_turn_right(){//回傳轉到目前角度需要多久(右轉90度)
  set_speed(RTLpwm_val,RTRpwm_val);
  axis_dir=get_direction();
  axis_dir=(axis_dir+90)%360;
  float time_now=millis();
  dir=get_direction();
  set_right();
  while(abs(compare_dir())>=dir_error_limit){
      dir=get_direction();
  }
  //axis_dir=dir;
  set_stop(); 
  return String(millis()-time_now);
}


String test_turn_left(){//回傳左轉90度需要的時間
  set_speed(LTLpwm_val,LTRpwm_val);
  axis_dir=get_direction();
  axis_dir=(axis_dir-90);
  float time_now=millis();
  dir=get_direction();
  if(axis_dir<0)axis_dir+=360;
  set_left();
  while(abs(compare_dir())>=dir_error_limit){
    dir=get_direction();
  }
  set_stop();
  return String(millis()-time_now);
}

String test_ultrasound(){//回傳在此位置超聲波感測器讀取的值
  String res="";
  int FL_dis,FM_dis,FR_dis;
  FL_dis=FM_dis=FR_dis=0;
  FL_dis=get_object_dis(trig_FL,echo_FL);
  delay(200);
  FM_dis=get_object_dis(trig_FM,echo_FM);
  delay(200);
  FR_dis=get_object_dis(trig_FR,echo_FR);
  delay(200);
  return String(FL_dis)+","+String(FM_dis)+","+String(FR_dis);  
}



/*超聲波偵測*/
long get_object_dis(int trigPin,int echoPin){//用超聲波偵測物體距離
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);
  long duration = pulseIn(echoPin, HIGH);
  long cm = (duration/2) / 29.1;
  return cm;
}

String ultrasound_detect(){//用超聲波偵測有無物體在我們定義之距離內
  /*超聲波前*/
  int FL_dis,FM_dis,FR_dis,BL_dis,BM_dis,BR_dis;
  FL_dis=FM_dis=FR_dis=BL_dis=BM_dis=BR_dis=0;
  FL_dis=get_object_dis(trig_FL,echo_FL);
  delay(200);
  FM_dis=get_object_dis(trig_FM,echo_FM);
  delay(200);
  FR_dis=get_object_dis(trig_FR,echo_FR);
  delay(200);
  if(FL_dis<detect_limit_dis||FM_dis<detect_limit_dis||FR_dis<detect_limit_dis){
      return "1";
  }
  else return "0";
}


/*碰撞感測器偵測*/
bool crash_detect(){
    if(!(digitalRead(crash_FL))||!(digitalRead(crash_FR))){
        return 1;
    }
    else{
        return 0;
    }
}

String decode_string(String&data){
    int len=0;
    int i=0;
    while(data[i]!='$'){
        i++;
    }
    len=data.substring(0,i).toInt();
    i++;
    int count=0;
    String res="";
    while(count<len){
      res.concat(data[i]);
      count++;
      i++;
    }
    return res;
}
String encode_string(String data){
    String res="";
    int len=data.length();
    res=String(len)+"$"+data;
    return res;
}

void inter_transmit(String data){//傳data給arduino
    String res=encode_string(data);
   //Serial.println(res);
    for(int i=0;i<res.length();i++){
        Serial.print(res[i]);
        delay(100);
    }
    Serial.print('\n');
}

String inter_receive(){//從arduino接收一串資料
  String data="";
  bool finished=0;
  while(!finished){
    while(Serial.available()){
        char c=Serial.read();
        if(c=='\n'){
          finished=1;
          break;  
        }
        else data.concat(c);  
    }
  }
  
  String res=decode_string(data);
  return res;
}


int get_direction(){//羅盤在同一直線個位置給出的數值不一樣:大(170~180)->小(140~160)->大(180~190)
  compass.read();
  int a;
  a = compass.getAzimuth();
  return a;
}


int compare_dir(){
  if(dir==axis_dir)return 0;
  else if(dir<=180&&axis_dir<=180||dir>=180&&axis_dir>=180){
      return dir-axis_dir;
  }
  else if(dir<=180&&axis_dir>=180){
      int diff=axis_dir-dir;
      if(diff<360-diff)return -diff;
      else return 360-diff;
  }
  else{
      int diff=dir-axis_dir;
      if(diff<360-diff)return diff;
      else return -(360-diff);
  }
}

void setting(String&set){//格式: %(0->dis_unit_time/1->left_unit_time/2->right_unit_time)(val)
  char target=set[1];
  if(target=='0'){//直走一格的時間
    dis_unit_time=set.substring(2).toFloat();    
  }
  else if(target=='1'){//左轉的時間
    left_unit_time=set.substring(2).toFloat();
  }
  else if(target=='2'){//右轉的時間
    right_unit_time=set.substring(2).toFloat();
  }
  else if(target=='3'){//直走左側pwm
    Lpwm_val=set.substring(2).toInt();
  }
  else if(target=='4'){//直走右側pwm
    Rpwm_val=set.substring(2).toInt();
  }
  else if(target=='5'){//左轉左側pwm
    LTLpwm_val=set.substring(2).toInt();
  }
  else if(target=='6'){//左轉右側pwm
    LTRpwm_val= set.substring(2).toInt();
  }
  else if(target=='7'){//右轉左側pwm
    RTLpwm_val= set.substring(2).toInt();
  }
  else if(target=='8'){//右轉右側pwm
    RTRpwm_val= set.substring(2).toInt();
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  M_Control_IO_config(); 
  set_speed(Lpwm_val,Rpwm_val);
  //wifi_Serial.begin(115200);
  //Serial.println("begin");
  //Serial.flush();
  set_stop();
  compass.init();
  pinMode(trig_FL, OUTPUT);      //Arduino 對外啟動距離感測器Trig腳，射出超音波 
  pinMode(echo_FL, INPUT);       //超音波被障礙物反射後，Arduino讀取感測器Echo腳的時間差
  pinMode(trig_FM, OUTPUT);      //Arduino 對外啟動距離感測器Trig腳，射出超音波 
  pinMode(echo_FM, INPUT);
  pinMode(trig_FR, OUTPUT);      //Arduino 對外啟動距離感測器Trig腳，射出超音波 
  pinMode(echo_FR, INPUT);
  pinMode(trace_pinL,INPUT);
  pinMode(trace_pinR,INPUT);
}



void loop() {
  // put your main code here, to run repeatedly:
  String instruction=inter_receive();
  String return_s="";
  if(instruction=="f"){
    return_s=forward();
    inter_transmit(return_s);
  }
  else if(instruction=="b"){
    //backward();
    return_s="Bfinish";
    inter_transmit(return_s);
  }
  else if(instruction=="r"){
    return_s=turn_right();;//test_turn_right();
    inter_transmit(return_s);
  }
  else if(instruction=="l"){
    return_s=turn_left();;//test_turn_left();;
    inter_transmit(return_s);
  }
  else if(instruction=="c"){
    //check();
    return_s="Cfinish";
    inter_transmit(return_s);
  }
  else if(instruction.length()>0&&instruction[0]=='%'){//setting 
    setting(instruction);
    if(instruction[1]=='0'){
      return_s="dis="+String(dis_unit_time);       
    }
    else if(instruction[1]=='1'){
      return_s="left="+String(left_unit_time);  
    }
    else if(instruction[1]=='2'){
      return_s="right="+String(right_unit_time);
    }
    else if(instruction[1]=='3'){
      return_s="Lpwm="+String(Lpwm_val);
    }
    else if(instruction[1]=='4'){
      return_s="Rpwm="+String(Rpwm_val);
    }
    else if(instruction[1]=='5'){
      return_s="LTLpwm="+String(LTLpwm_val);
    }
    else if(instruction[1]=='6'){
      return_s="LTRpwm="+String(LTRpwm_val);
    }
    else if(instruction[1]=='7'){
      return_s="RTLpwm="+String(RTLpwm_val);
    }
    else if(instruction[1]=='8'){
      return_s="RTRpwm="+String(RTRpwm_val);
    }
    else{
      return_s="setting_error";  
    }
    inter_transmit(return_s);
  }
  else if(instruction.length()>0&&instruction[0]=='@'){//test
      if(instruction[1]=='0'){
        return_s=test_forward();    
      }
      else if(instruction[1]=='1'){
        return_s=test_turn_left();  
      }
      else if(instruction[1]=='2'){
        return_s=test_turn_right();
      }
      else if(instruction[1]=='3'){
        return_s=test_ultrasound();
      }
      else{
        return_s="test_error";  
      }
      inter_transmit(return_s);
  }
  else{
    return_s="error_string";
    inter_transmit(return_s);
  }
  Serial.flush();
  
  /*String arr[4]={"Ffinish","Bfinish","Rfinish","Lfinish"};
  inter_transmit(arr[i%4]);
  i+=1;*/
  //delay(1000);//要delay，因為軟串口速度會跟不上
}
