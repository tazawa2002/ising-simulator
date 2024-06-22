// -----------------------------
// KY-003 Hall magnetic switch
// -----------------------------
#include <Adafruit_BMP085.h> 
#include <esp_now.h>
#include <WiFi.h>

// REPLACE WITH YOUR RECEIVER MAC Address
// F4:12:FA:D9:8F:38
uint8_t broadcastAddress[] = {0xF4, 0x12, 0xFA, 0xD9, 0x8F, 0x38};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  char a[32];
  int b;
  float c;
  bool d;
} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

int SENSOR1 = D1; // define the Hal magnetic sensor interface
int SENSOR2 = D2; // define the Hal magnetic sensor interface
double t_data_init;
double t_data;
int val1;
int val2;
int Led_status;

Adafruit_BMP085 a_bmp;

void setup() {
  pinMode(SENSOR1, INPUT); // define the Hall magnetic sensor line as input
  pinMode(SENSOR2, INPUT); // define the Hall magnetic sensor line as input
  Serial.begin( 115200 );
  Serial.println("シリアルモニタ");
  // 気圧センサ起動
  if ( !a_bmp.begin() ) {
    Serial.println( "温度センサ起動エラー" );
    while(1);   // エラー時の強制ループ
  }
  Serial.println("温度センサー起動");
  t_data_init = a_bmp.readTemperature();
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

}

void loop() {  
  val1 = digitalRead(SENSOR1); // read sensor line
  val2 = digitalRead(SENSOR2); // read sensor line
  t_data = a_bmp.readTemperature();
  if(t_data < t_data_init) t_data_init=t_data;
  if(val1==0 & val2==1){
    myData.b = 1;
  }else if(val1==1 & val2==0){
    myData.b = -1;
  }else{
    myData.b = 0;
  }
  myData.c = t_data - t_data_init;
  
  Serial.print( val1 );
  Serial.print(", ");
  Serial.print( val2 );
  Serial.print(", ");
  Serial.print( t_data );
  Serial.print(", ");
  Serial.print( t_data_init );
  Serial.print(", ");
  Serial.print( myData.b );
  Serial.print(", ");
  Serial.println( myData.c );

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  
  delay( 20 );

}
