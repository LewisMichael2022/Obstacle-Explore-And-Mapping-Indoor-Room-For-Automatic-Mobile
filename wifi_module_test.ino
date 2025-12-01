#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
const char* ssid = "LAPTOP-NUE8DFUI 7287"; // 请将此处更改为您的Wi-Fi SSID
const char* password = "56&5Tu22"; // 请将此处更改为您的Wi-Fi密码
const char* host = "192.168.137.1"; // 请将此处更改为您要连接的主机的IP地址
const int port = 12346; // 请将此处更改为您要连接的主机的端口号
const int myport = 12345;
int debug_count=0;

float dis_unit_time=1.5;//前進一格需多少秒

//碰撞感測器相關
/*
#define crash_FL 13
#define crash_FR
#define crash_BL
#define crash_BR
*/

WiFiServer server(myport);
//UNO傳輸給wifi
//SoftwareSerial wifi_Serial(12,14); // TX=d6, RX=d5

void wifi_init(){// 连接到Wi-Fi网络
  //Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000); //wifi模組不能閒置太久，因此要加上delay()
    //Serial.print(".");
  }  
  //Serial.println("Wi-Fi connected");
  //
  server.begin();
}

String get_information(){//一直監聽直到主機有傳資訊過來
  WiFiClient client;//arduino每次連上wifi的IP都會不一樣
  while(1){//listen
    if(WiFi.status() != WL_CONNECTED)break;
    client = server.available();
    if(client!=NULL)break;
  }
  //Serial.println("listen finished");
  while(!client.connected()){//accept
    if(WiFi.status() != WL_CONNECTED)break;
    delay(100);
  } 
  //Serial.println("accept finished");
  String data="";
  char c='e';
  while(WiFi.status()== WL_CONNECTED){//recv
    while (client.available()) {
      if(WiFi.status() != WL_CONNECTED)break;
      c=(char)client.read();
      if(c=='\n')break;
      data+=c;
    }
    if(c=='\n')
      break;
  }
  client.stop();
  String res=decode_string(data);
  return res;
}

void transmit(String data){//傳一串data給主機
  WiFiClient client;//arduino每次連上wifi的IP都會不一樣 
  while(1){//listen
    if(WiFi.status() != WL_CONNECTED)break;
    client = server.available();
    if(client!=NULL)break;
  }
  //Serial.println("listen finished");
  while(!client.connected()){//accept
    if(WiFi.status() != WL_CONNECTED){
      
      return; 
    }
    delay(100);
  } 
  if(WiFi.status()==WL_CONNECTED){
    client.println(encode_string(data));
    client.stop();      
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


String crash_detect(){
    /*String res="";
    char res_FL='0'+!(digitalRead(crash_FL));
    char res_FR='0'+!(digitalRead(crash_FR));
    char res_BL='0'+!(digitalRead(crash_BL));
    char res_BR='0'+!(digitalRead(crash_BR));*/
    /*
    if(!(digitalRead(crash_FL))||!(digitalRead(crash_FR))||!(digitalRead(crash_BL))||!(digitalRead(crash_BR))){
        return "1";
    }
    else{
        return "0";
    } */
    return "0";
}



void setup() {
  Serial.begin(9600);
  wifi_init();
  //Serial.flush();
  // put your setup code here, to run once:
}
int i=0;
void loop(){
  if(WiFi.status() != WL_CONNECTED){
    wifi_init();
  }
  //digitalWrite(LED_BUILTIN, HIGH); 
  // put your main code here, to run repeatedly:
  
  //wifi從srver收到指令
  String instruction=get_information();
  //wifi把指令傳給UNO
  inter_transmit(instruction);
  Serial.flush();
  //wifi從UNO接收執行結果
  
  String res_message=inter_receive();
  
  if(WiFi.status() != WL_CONNECTED){
    wifi_init();
  }
  //wifi把執行結果傳給server
  transmit(res_message);
}
