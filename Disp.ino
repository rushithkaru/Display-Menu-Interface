#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define debounceTimeout 300
#define CLOCK 14 
#define DATA 13 
#define CS 15  
#define DC 27 
#define RESET 26 
#define PushButton1  12
#define PushButton2  4
#define PushButton3  19
#define PushButton4  18
#define LED1 33
#define LED2 32
#define LED3 25
#define RTCAD 0x51
#define EEPROM_ADR 0x56

//Constuctor call for the u8g2 library and ssd1309 display
U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(U8G2_R0, CLOCK, DATA, CS, DC, RESET);
//Maximum depth of a menu
//Can have different variables fpor different menus
const int maxMenu = 6;
//Depth 1 no interaction code
const int stable[6] = {0,2,3,4,5,6};
//Current depth
const int maxDepth = 1;
//FreeRtos queue type
QueueHandle_t queue;
uint8_t _seconds;
uint8_t _minutes;
uint8_t _hours;
uint8_t _days;
uint8_t months;
uint16_t years;
byte tempStore = 1;
long eeAddress = 0;

//Bit map for solar panel
static unsigned char panel[] PROGMEM 
  {
  0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 
  0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x84, 0x01, 
  0x00, 0x00, 0x00, 0x90, 0x3F, 0x01, 0x00, 0x00, 0x00, 0xC0, 0x71, 0x00, 
  0x00, 0x00, 0x00, 0x60, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x60, 0xC0, 0x00, 
  0x00, 0x00, 0x00, 0x20, 0x80, 0x04, 0x00, 0x00, 0x00, 0x36, 0x80, 0x0C, 
  0xFE, 0xFF, 0x3F, 0x24, 0x80, 0x01, 0xFF, 0xFF, 0x3F, 0x60, 0xC0, 0x00, 
  0x03, 0x83, 0x60, 0x60, 0xC0, 0x00, 0x07, 0x82, 0xC1, 0xC0, 0x71, 0x00, 
  0x07, 0x06, 0xC1, 0x90, 0x3F, 0x01, 0x0F, 0x0C, 0x83, 0x19, 0x84, 0x01, 
  0x1B, 0x0C, 0x86, 0x11, 0x00, 0x01, 0x1A, 0x08, 0x06, 0x03, 0x06, 0x00, 
  0xF6, 0xFF, 0xFF, 0x03, 0x04, 0x00, 0xEC, 0xFF, 0xFF, 0x07, 0x00, 0x00, 
  0x6C, 0x30, 0x18, 0x06, 0x00, 0x00, 0x58, 0x60, 0x10, 0x0C, 0x00, 0x00, 
  0xD8, 0x60, 0x30, 0x08, 0x00, 0x00, 0xB0, 0xC0, 0x30, 0x18, 0x00, 0x00, 
  0xB0, 0xC1, 0x60, 0x30, 0x00, 0x00, 0x60, 0x81, 0xC1, 0x30, 0x00, 0x00, 
  0x60, 0xBF, 0xF7, 0x77, 0x00, 0x00, 0xC0, 0xFE, 0xFF, 0x7F, 0x00, 0x00, 
  0x80, 0x06, 0x86, 0xC1, 0x00, 0x00, 0x80, 0x0D, 0x06, 0xC3, 0x00, 0x00, 
  0x00, 0x0D, 0x0C, 0x83, 0x01, 0x00, 0x00, 0x1B, 0x0C, 0x06, 0x01, 0x00, 
  0x00, 0x12, 0x18, 0x04, 0x03, 0x00, 0x00, 0x36, 0x10, 0x0C, 0x02, 0x00, 
  0x00, 0xEC, 0xFB, 0xDF, 0x07, 0x00, 0x00, 0xEC, 0xFF, 0xFF, 0x0F, 0x00, 
  0x00, 0xD8, 0x60, 0x30, 0x0C, 0x00, 0x00, 0xD8, 0x40, 0x30, 0x18, 0x00, 
  0x00, 0xB0, 0xC1, 0x20, 0x18, 0x00, 0x00, 0xA0, 0x81, 0x60, 0x30, 0x00, 
  0x00, 0x60, 0x83, 0xC1, 0x30, 0x00, 0x00, 0x40, 0x02, 0xC3, 0x60, 0x00, 
  0x00, 0xC0, 0xDF, 0xE7, 0xE7, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0x7F, 0x00, 
  };
  
//Bitmap for grid
static unsigned char tower[] PROGMEM {
  0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 
  0x00, 0x38, 0x00, 0xC0, 0x01, 0x00, 0x00, 0x78, 0x00, 0xE0, 0x01, 0x00, 
  0x00, 0xD8, 0x00, 0xB0, 0x01, 0x00, 0x00, 0x98, 0x01, 0x98, 0x01, 0x00, 
  0x00, 0xF8, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x1E, 0xFE, 0x8F, 0x03, 0x00, 
  0x00, 0x8F, 0x67, 0x8F, 0x0F, 0x00, 0x80, 0xD9, 0x61, 0xB8, 0x19, 0x00, 
  0xC0, 0xFF, 0xFF, 0xF7, 0x3F, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0x3F, 0x00, 
  0x00, 0xD8, 0x00, 0x30, 0x01, 0x00, 0x00, 0xE8, 0x00, 0xB0, 0x01, 0x00, 
  0x00, 0x38, 0x00, 0xC0, 0x01, 0x00, 0x00, 0x38, 0x00, 0xC0, 0x01, 0x00, 
  0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0x00, 0x38, 0x00, 0xC0, 0x01, 0x00, 
  0x00, 0x70, 0x00, 0xE0, 0x00, 0x00, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0x00, 
  0x00, 0xF0, 0x01, 0xD8, 0x00, 0x00, 0x00, 0x20, 0x01, 0x68, 0x00, 0x00, 
  0x00, 0x60, 0x03, 0x6C, 0x00, 0x00, 0x00, 0xE0, 0x07, 0x7E, 0x00, 0x00, 
  0x00, 0xC0, 0x0C, 0x33, 0x00, 0x00, 0x00, 0xC0, 0x98, 0x31, 0x00, 0x00, 
  0x00, 0x80, 0xF1, 0x30, 0x00, 0x00, 0x00, 0x80, 0xFF, 0x1F, 0x00, 0x00, 
  0x00, 0x80, 0xFF, 0x1E, 0x00, 0x00, 0x00, 0x00, 0xF1, 0x08, 0x00, 0x00, 
  0x00, 0x00, 0x9B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 
  0x00, 0x00, 0xFE, 0x07, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x0F, 0x00, 0x00, 
  0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFB, 0x0D, 0x00, 0x00, 
  0x00, 0x00, 0xE1, 0x08, 0x00, 0x00, 0x00, 0x80, 0xF1, 0x18, 0x00, 0x00, 
  0x00, 0x80, 0x9D, 0x19, 0x00, 0x00, 0x00, 0x80, 0x0C, 0x17, 0x00, 0x00, 
  0x00, 0xC0, 0x07, 0x3C, 0x00, 0x00, 0x00, 0xC0, 0x01, 0x3C, 0x00, 0x00, 
  0x00, 0xC0, 0x00, 0x30, 0x00, 0x00, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 
};


void TaskBlink( void *pvParameters );
double scaleFind(int data[]);
void Display( void *pvParameters);
double scaleFind(int data[]);
int maxFind(int data[]);
int upCheck(long int currentTime);
int downCheck(long int currentTime);
int select(long int currentTime);
int goBack(long int currentTime);
void stateChoose(int currentState, int *next, int *back, int *depth,int *menuState2, int *newMenu);
int stateCheck(int currentState,long int currentTime, int *next, int *back, int *menuState2, int newMenu);
void menuSystem();
void graph();
void firstMenu(int currentState);
void secondMenu(int currentState);
void menuSystem2();
void energyShow();
int checkStable(int state);
void empty();
void voltage();
void bitmapD();
byte readEeprom();
void uploadEeprom();
void setDate(uint16_t years, uint8_t months, uint8_t _days, uint8_t _hours, uint8_t _minutes, uint8_t _seconds);
void now(void);
uint8_t bcdToDec(uint8_t val);
uint8_t decToBcd(int val);
uint8_t decToBcd(int val);


void setup() {
  pinMode(PushButton1, INPUT);
  pinMode(PushButton2, INPUT);
  pinMode(PushButton3, INPUT);
  pinMode(PushButton4, INPUT);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  pinMode(LED3, OUTPUT);
  Serial.begin(115200);
  Wire.begin();
  setDate(2021,3,2,12,1,0);
  now();
  Serial.println(readEeprom());
  
  u8g2.begin();
  u8g2.clear();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  do {
    u8g2.setFont(u8g2_font_helvR14_tf); // 14 px height
    u8g2.drawStr(3, 32, "LOGO HERE");
    } while ( u8g2.nextPage() );
    digitalWrite(LED1,HIGH);
    digitalWrite(LED2,HIGH);
    digitalWrite(LED3,HIGH);
    delay(4000);
    u8g2.clear();
    digitalWrite(LED1,LOW);
    digitalWrite(LED2,LOW);
    digitalWrite(LED3,LOW);
    
    xTaskCreate(
    Display
    ,  "Display"   // A name just for humans
    ,  12000  // Stack size
    ,  NULL
    ,  1  // priority
    ,  NULL );

    /*xTaskCreate(
    TaskBlink
    ,  "Blink"   // A name just for humans
    ,  1000  // Stack size
    ,  NULL
    ,  0  // priority
    ,  NULL );*/
   
}

int state = 0;
int menuState2 = 0;
int newMenu = 0;
int prevState = 0;
long int lastDebounceTime;
int newPage = 0;
int depth = 0;
int arrowPos = 39;

void loop() {
  
  now();
  delay(1000);
         
}

//Function to find scale of the grid
double scaleFind(int data[]){
  int max = maxFind(data);
  return (double) max/50;
}


int maxFind(int data[]){
  int max = 0;
  for (int i = 0; i < 10 ; i++){
    if (data[i] >= max){
      max = data[i];   
    }
  }
  return max;
}

//Check for up press
int upCheck(long int currentTime){
  if (digitalRead(PushButton1)){
    currentTime = lastDebounceTime;
  }
  if (digitalRead(PushButton1 == 0) && (currentTime - lastDebounceTime) > debounceTimeout){
    return 1;
  }
  else {
    return 0;
  }
}

//Check for down press
int downCheck(long int currentTime){
  if (digitalRead(PushButton2)){
    currentTime = lastDebounceTime;
  }
  if (digitalRead(PushButton2) == 0 && (currentTime - lastDebounceTime) > debounceTimeout){
    return 1;
  }
  else {
    return 0;
  }
}

//Function to check for select button
int select(long int currentTime){
  if (digitalRead(PushButton3)){
    currentTime = lastDebounceTime;
  }
  if (digitalRead(PushButton3) == 0 && (currentTime - lastDebounceTime) > debounceTimeout){
    return 1;
  }
  else {
    return 0;
  }
}



//Function to check back button
int goBack(long int currentTime){
  if (digitalRead(PushButton4)){
    currentTime = lastDebounceTime;
  }
  if (digitalRead(PushButton4) == 0 && (currentTime - lastDebounceTime) > debounceTimeout){
    return 1;
  }
  else {
    return 0;
  }
}

//Function that calls the check functions and updates necessary variables
int stateCheck(int currentState,long int currentTime, int *next, int *back, int *menuState2, int newMenu){
  
  if (newMenu == 0){  
    if (select(currentTime)){
      *next = 1;
       if (checkStable(currentState) == 0 && depth > 0){
          *next = 0;
        }
      
      delay(200);
      return currentState;
    }
    else if(goBack(currentTime)){
      *back = 1;
      delay(200);
      return 0;
    }
    else if (upCheck(currentTime)){
      if (currentState < maxMenu){
        currentState += 1;
        if (checkStable(currentState) == 0 && depth > 0){
          currentState -= 1;
        }
      }
      else{
        if (depth < 1 || checkStable(currentState) ){
          currentState = 0;
        }
      }
      delay(200);
      return currentState;
    }
    else if (downCheck(currentTime)){
      if (currentState > 0 ){
        currentState -= 1;
        if (checkStable(currentState) == 0 && depth > 0){
          currentState += 1;
        }
      }
      else{
        if (depth < 1 || checkStable(currentState)){
          currentState = maxMenu ;
        }
      }
      delay(200);
      return currentState;
    }
  }
  //Second Menu
  //Add checks here for future new menus
  else{
    if (select(currentTime)){
      *next = 1;
      
      delay(200);
      return currentState;
    }
    else if(goBack(currentTime)){
      *back = 1;
      delay(200);
      return 0;
    }
    else if (upCheck(currentTime)){
      if (*menuState2 < maxMenu ){
        *menuState2 += 1;
      }
      else{
        *menuState2 = 0;
      }
      delay(100);
      return currentState;
    }
    else if (downCheck(currentTime)){
      if (*menuState2 > 0 ){
        *menuState2 += -1;
      }
      else{
        *menuState2 = maxMenu;
      }
      
      delay(200);
      return currentState;
    }
  }
  //Add else if here for any new menus 
  return currentState;
}

//Function to choose what page to show on the display
void stateChoose(int currentState, int *next, int *back, int *depth,int *menuState2, int *newMenu){
  //depth = the current stage of the menu
  //*next and *back are 1 when the buttons are pushed and are used to decide which page to display
  //When adding menus need to create global variable like menuState2 to 
  
  
       if (*next){
        if (*depth < maxDepth){
          *depth += 1;
        }
          *next = 0;         
       }
       if (*back){
          if (*depth > 0){
            *depth += -1; 
          }
          *back = 0; 
          *newMenu = 0;
          *menuState2 = 0;
       }       
       if (*depth == 0){
          firstMenu(currentState);         
       }
       else if(*depth == 1){
          if (currentState == 0){
            graph();
          }
          else if (currentState == 1){
            *newMenu = 1;
            secondMenu(*menuState2);
          }
          else if (currentState == 2){
            energyShow();
          }
          else if (currentState == 3){
            voltage();
          }
          else if (currentState == 4){
            bitmapD();
          }
          //Add here to increase menu size but need to add to the menu page as well
          else if (currentState == 5){
            showTime();
          }
          else {
            empty();
          }
       }
       //else if depth == 2 for further extension
}

//Main menu Stings
void menuSystem(){
  if (state <= 5){
    u8g2.drawStr(2,10,"Graph");
    u8g2.drawStr(2,20,"New Menu");
    u8g2.drawStr(2,30,"State");
    u8g2.drawStr(2,40,"Data");
    u8g2.drawStr(2,50,"Bitmaps");
    u8g2.drawStr(2,60,"Time");
  }
  else{
    u8g2.drawStr(2,10,"Empty");
  }
}


//Graph function
//Change array or get data to arrayof size 10 to create the graph
//Need to add max value for the graph for scale
void graph(){
          int xChange = 15;
          //data stream can be changed here
          int data[10] = {2, 4, 8, 3, 6, 2, 3, 2, 7, 3};
          //find the scale based on biggest value
          double scale = scaleFind(data);
          //get height of first data point
          int height = (int) data[0]/scale;
          u8g2.firstPage();
          do {
              u8g2.drawHLine(10,55,100);
              u8g2.drawVLine(10,5,50);
              u8g2.setFont(u8g2_font_ncenB08_tr);
              u8g2.setCursor(0, 10);
              u8g2.print("Y");
              u8g2.setCursor(112,55);
              u8g2.print("X");
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[1]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[2]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[3]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[4]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[5]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[6]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[7]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[8]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);
              height = (int) data[9]/scale;
              xChange += 10;
              u8g2.drawBox(xChange, 55 - height , 5 , height);           
             } while ( u8g2.nextPage() );
}

//Pages for the first menu
void firstMenu(int currentState){
        if (currentState == 0){
           u8g2.firstPage();
           do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem();
               u8g2.drawFrame(0,1,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();             
              } while ( u8g2.nextPage() );
           
          }
           if (currentState == 1){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem();
               u8g2.drawFrame(0,11,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           if (currentState == 2){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem();
               u8g2.drawFrame(0,21,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           if (currentState == 3){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem();
               u8g2.drawFrame(0,31,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           if (currentState == 4){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem();
               u8g2.drawFrame(0,41,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           if (currentState == 5){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem();
               u8g2.drawFrame(0,51,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           if (currentState == 6){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem();
               u8g2.drawFrame(0,1,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           
}

// Pages for the second menu
// add neceessary menus for the next menus
void secondMenu(int currentState){
        if (currentState == 0){
           u8g2.firstPage();
           do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem2();
               u8g2.drawFrame(0,0,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
              } while ( u8g2.nextPage() );          
          }
           if (currentState == 1){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem2();
               u8g2.drawFrame(0,11,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           if (currentState == 2){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem2();
               u8g2.drawFrame(0,21,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() )
                ;
           }
           if (currentState == 3){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem2();
               u8g2.drawFrame(0,31,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           if (currentState == 4){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem2();
               u8g2.drawFrame(0,41,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
           if (currentState == 5){
            u8g2.firstPage();
            do {
               u8g2.setFont(u8g2_font_ncenB08_tr);
               menuSystem2();
               u8g2.drawFrame(0,51,70,10);
               u8g2.drawRFrame(115,0,9,64,4);
               scroll();
                } while ( u8g2.nextPage() );
           }
}

//Strings for the second menu
void menuSystem2(){
  u8g2.drawStr(2,10,"Empty");
  u8g2.drawStr(2,20,"Empty");
  u8g2.drawStr(2,30,"Empty");
  u8g2.drawStr(2,40,"Empty");
  u8g2.drawStr(2,50,"Empty");
  u8g2.drawStr(2,60,"Empty");
}


//Function that has the animation, needs edit
void energyShow(){
           u8g2.firstPage();
           do {
              u8g2.drawRFrame(10,10,30,22,7);
              u8g2.drawRFrame(90,10,30,22,7);
              u8g2.drawTriangle(90,21, 75,35, 75,7);
              u8g2.setFont(u8g2_font_unifont_t_symbols);
              u8g2.drawGlyph(arrowPos, 26, 0x23E9);
              u8g2.drawLine(39, 27, 74, 27);
              u8g2.drawLine(39,15, 74, 15);
              u8g2.drawStr(11,25,"Inv");
              delay(100);
              arrowPos += 11;
              if (arrowPos > 61){
                arrowPos = 39;
              }
              } while ( u8g2.nextPage() );
}

//Blinking multi task
void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;


  for (;;) // A Task shall never return or exit.
  {    
    //Serial.println(xPortGetCoreID());
    digitalWrite(LED1, HIGH);   // turn the LED on (HIGH is the voltage level)
    vTaskDelay(100);  // one tick delay (15ms) in between reads for stability
    digitalWrite(LED1, LOW);    // turn the LED off by making the voltage LOW
    vTaskDelay(100);  // one tick delay (15ms) in between reads for stability
    
  }
}

//freertos task for the display functionality
void Display( void *pvParameters){
  (void) pvParameters;
  for(;;){
    
  // put your main code here, to run repeatedly:
    int next = 0;
    int back = 0;
    
    long int currentTime = millis();
    state = stateCheck(state, currentTime, &next, &back, &menuState2, newMenu);   
    stateChoose(state, &next, &back,&depth,&menuState2,&newMenu);
    prevState = state;
    
    vTaskDelay( 10 ); 
  
  }
}//Check for static pages (pages that do not have any interaction)
int checkStable(int state){
  for (int i = 0; i < maxMenu; i++){
    if (state == stable[i]){
      return 0;
    }
  }
  return 1;
}

//Scroll function needs to be edited if the size of menu changes
void scroll(){
  if (depth == 0){
   if(state == 0){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 15, 0x2195);
   }
   if(state == 1){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 22, 0x2195);
   }
   if(state == 2){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 30, 0x2195);
   }
   if(state == 3){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 40, 0x2195);
   }
   if(state == 4){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 50, 0x2195);
   }
   if(state == 5){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 55, 0x2195);
   }
   if(state == 6){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 62, 0x2195);
   }
  }
  else if(depth == 1){
    if(menuState2 == 0){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 15, 0x2195);
   }
   if(menuState2 == 1){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 22, 0x2195);
   }
   if(menuState2 == 2){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 30, 0x2195);
   }
   if(menuState2 == 3){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 40, 0x2195);
   }
   if(menuState2 == 4){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 50, 0x2195);
   }
   if(menuState2 == 5){
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      u8g2.drawGlyph(115, 62, 0x2195);
   }
   
  }
}

//Page for empty data
void empty(){
  u8g2.firstPage();
  do {   
    u8g2.setFont(u8g2_font_helvR14_tf); // 14 px height
    u8g2.drawStr(45, 32, "Empty");
    } while ( u8g2.nextPage() );
}

//Page
void voltage(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvR10_tr);  // choose a suitable font
  u8g2.setCursor(0,12);  
  u8g2.print("Power");  
  String power = String(millis());
  u8g2.setCursor(50, 14);
  u8g2.print(power);
  u8g2.setCursor(100, 14);
  u8g2.print("kW");
  u8g2.drawStr(0,35,"Day - ");
  String volts = String(millis());
  u8g2.setCursor(50, 35);
  u8g2.print(volts);  
  u8g2.setCursor(100, 35);
  u8g2.print("kWh");
  u8g2.setCursor(5, 55);
  u8g2.print("EVENT - NONE");
  u8g2.sendBuffer();
}
//Display time on screen
void showTime(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvR10_tr);  // choose a suitable font
  u8g2.setCursor(0,12);  
  u8g2.print("Time");
  String hourDraw = String(_hours);
  String minDraw = String(_minutes);
  String secDraw = String(_seconds);
  u8g2.setCursor(50, 12);
  u8g2.print(hourDraw);
  u8g2.setCursor(70, 12);
  u8g2.print(":");
  u8g2.setCursor(75, 12);
  u8g2.print(minDraw);
  u8g2.setCursor(85, 12);
  u8g2.print(":");
  u8g2.setCursor(90, 12);
  u8g2.print(secDraw);
  
  //Date
  u8g2.setCursor(0,32);  
  u8g2.print("Date");
  String yearDraw = String(years);
  String monthDraw = String(months);
  String dateDraw = String(_days);
  u8g2.setCursor(50, 32);
  u8g2.print(yearDraw);
  u8g2.setCursor(85, 32);
  u8g2.print("/");
  u8g2.setCursor(95, 32);
  u8g2.print(monthDraw);
  u8g2.setCursor(105, 32);
  u8g2.print("/");
  u8g2.setCursor(115, 32);
  u8g2.print(dateDraw);
  u8g2.sendBuffer();
  
}

//Bit map display function
void bitmapD(){
  u8g2.firstPage();
  do {   
    u8g2.drawXBM(0, 10, 44 , 44 , panel);
    u8g2.drawXBM(90, 10, 44 , 44 , tower);   
    } while ( u8g2.nextPage() );
  delay(100);
}

//RTC CODE

//Set time on rtc
void setDate(uint16_t years, uint8_t months, uint8_t _days,
                              uint8_t _hours, uint8_t _minutes, uint8_t _seconds) {
  Wire.beginTransmission(RTCAD);
  Wire.write(0x03);
  Wire.write(decToBcd(_seconds) + 0x80);
  Wire.write(decToBcd(_minutes));
  Wire.write(decToBcd(_hours));
  Wire.write(decToBcd(_days));
  Wire.write(0x00);
  Wire.write(decToBcd(months));
  Wire.write(decToBcd(years-2000));
  Wire.endTransmission();
}

//decimal to binary coded decimal
uint8_t decToBcd( int val )
{
   return (uint8_t) ((val / 10 * 16) + (val % 10));
}

//binary coded decimal to decimal
uint8_t bcdToDec( uint8_t val )
{
   return (uint8_t) ((val / 16 * 10) + (val % 16));
}

//get time set on RTC
void now(void) {
  Wire.beginTransmission(RTCAD);
  Wire.write(0x03);
  Wire.endTransmission();
  Wire.requestFrom(RTCAD, 7);
  while(!Wire.available());
  _seconds = bcdToDec(Wire.read());                    
  _minutes = bcdToDec(Wire.read());
  _hours = bcdToDec(Wire.read());
  _days = bcdToDec(Wire.read());
  Wire.read(); // blank read weekdays
  months = bcdToDec(Wire.read());
  years = bcdToDec(Wire.read()) + 2000;
}

//EEPROM code

//Write to eeprom adress
void uploadEeprom(){
  Wire.beginTransmission(EEPROM_ADR);
  Wire.write((int)(eeAddress >> 8)); // MSB
  Wire.write((int)(eeAddress & 0xFF)); // LSB
  Wire.write(tempStore); 
  Wire.endTransmission();
} 

//Read from eeprom address
byte readEeprom(){
  Wire.beginTransmission(EEPROM_ADR);
  Wire.write((int)(eeAddress >> 8)); // MSB
  Wire.write((int)(eeAddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_ADR, 1);
  byte rdata = 0xFF;
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}
