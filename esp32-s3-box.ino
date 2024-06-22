#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <esp_now.h>
#include <WiFi.h>

#include <Adafruit_BMP085.h> 

int diffEnergy(int **array, int x, int y);
double energy_f(int **array);

#define GRID_SIZE 2
#define STEP 500

static LGFX lcd;

static int GRID_COLS;
static int GRID_ROWS;

static int energy;
static int magnetic = 0;

// Define the Ising model variables
static int **grid;
static double  temperature = 10.0; // Adjust the temperature as needed
static double B = 0;

Adafruit_BMP085 a_bmp;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
    char a[32];
    int b;
    float c;
    bool d;
} struct_message;

// Create a struct_message called myData
struct_message myData;
int flag = 0;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  flag = 1;
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Char: ");
  Serial.println(myData.a);
  Serial.print("Int: ");
  Serial.println(myData.b);
  Serial.print("Float: ");
  Serial.println(myData.c);
  Serial.print("Bool: ");
  Serial.println(myData.d);
  Serial.println();
}

void setup() {
  lcd.init();

  if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);

  GRID_COLS = lcd.width() / GRID_SIZE;
  GRID_ROWS = (lcd.height()-10) / GRID_SIZE;

  grid = (int **)malloc(GRID_COLS*sizeof(int *));
  grid[0] = (int *)malloc(GRID_COLS*GRID_ROWS*sizeof(int));
  for(int i=1;i<GRID_COLS;i++) grid[i] = grid[i-1] + GRID_ROWS;

  // Initialize the Ising model grid with random spins
  randomSeed(analogRead(0));
  for (int x = 0; x < GRID_COLS; x++) {
    for (int y = 0; y < GRID_ROWS; y++) {
      grid[x][y] = random(2) * 2 - 1; // Random spin: +1 or -1
      magnetic += grid[x][y];
    }
  }
  energy = energy_f(grid);

  // Display the Ising model on the screen
  for (int x = 0; x < GRID_COLS; x++) {
    for (int y = 0; y < GRID_ROWS; y++) {
      int color = (grid[x][y] == 1) ? TFT_MAROON : TFT_NAVY;
      lcd.fillRect(x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE, color);
    }
  }

  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  int array[2][STEP];
  for(int i=0;i<STEP;i++){
    // 乱数を振る処理
    int x = rand() % GRID_COLS;
    int y = rand() % GRID_ROWS;
    array[0][i] = x;
    array[1][i] = y;
    
    // Calculate the change in energy if we flip this spin
    int diffen = diffEnergy(grid, x, y);

    // Perform the Metropolis algorithm to decide whether to flip the spin
    if (diffen <= 0 || random(1000) < exp(-diffen / temperature) * 1000) {
      grid[x][y] = -grid[x][y]; // Flip the spin
      magnetic += 2*grid[x][y];
      energy += diffen;
    }
  }

  // 描画処理
  for(int i=0;i<STEP;i++){
    int x = array[0][i];
    int y = array[1][i];
    int color = (grid[x][y] == 1) ? TFT_MAROON : TFT_NAVY;
    lcd.fillRect(x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE, color);
  }
  lcd.setCursor(0, GRID_ROWS*GRID_SIZE);
  lcd.setFont(&fonts::Font0);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  
  temperature = myData.c;
  B = myData.b;

  energy = energy_f(grid);
  
  lcd.printf("B=%2.0f, T=%.2f, E=%6.3f, M=%6.3f, %10s", B, temperature, (double)energy/(GRID_COLS*GRID_ROWS), (double)magnetic/(GRID_COLS*GRID_ROWS), flag==1 ? "connect" : "disconnect");
  flag = 0;
}

// エネルギーを計算する関数
double energy_f(int **array){
  int i, j;
  double sum = 0.0;

  for(i=0;i<GRID_COLS;i++){
    for(j=0;j<GRID_ROWS;j++){
      sum += array[i][j]*(array[((i==GRID_COLS-1)?0:i+1)][j] + array[i][((j==GRID_ROWS-1)?0:j+1)]) + B*array[i][j];
    }
  }
  return (-sum);
}

// スピンを反転した際のエネルギー差を計算する関数
int diffEnergy(int **array, int x, int y){
  int sum;
  sum = array[((x==GRID_COLS-1)?0:x+1)][y] + array[((x==0)?GRID_COLS-1:x-1)][y] + array[x][((y==GRID_ROWS-1)?0:y+1)] + array[x][((y==0)?GRID_ROWS-1:y-1)];

  return 2*array[x][y]*sum + 2*B*array[x][y];
}