/***********************************************************************
  Programmer:
    Andreas Spielhofer
    Date: 20.10.2020
    
    Version: IAKGBSYLA, V2.0, Nov 2020
    Following Libraries are needed for Adafruit 2.2" TFT Breakout Card:
      - Adafruit_GFX
      - Adafruit_ILI9340
************************************************************************/
String versionNumber = "IAKGBSYLA, V2.0, Nov 2020";


#include <math.h>

/******************
 For Debugging:
*****************/
boolean debug = false;


/******************
 Display Settings:
*****************/
#include <Adafruit_GFX.h>    // Core graphics library
#include "Adafruit_ILI9340.h" // Hardware-specific library
#include "Fonts/bgothm12pt7b.h"
#include <SPI.h>
#include <SD.h>

/******************
 Pinout Settings:
*****************/
//Display and SD Card
#define TFT_RST 29 //Andreas 48
#define TFT_DC 28 //Andreas49
#define TFT_CS 30 //Andreas 53
#define SD_CS 31 //Andreas 52 
boolean tftBoolean = true;
#define PinDisplayBL 2

//Analog Pins - Temperature
#define PinTemperature1 A0
#define PinTemperature2 A1
#define PinTemperature3 A2
int PinTemperatureArray[3] = {PinTemperature1, PinTemperature2, PinTemperature3};

//Analog Pins - LN2 Sensor
#define PinLN2 A4
int PinLN2Array[1] = {PinLN2};


//Analog Pins - Pressure
#define PinPressure1 A3
#define PinPressure2 A7
int PinPressureArray[2] = {PinPressure1,PinPressure2};

//Analog Pins - Not defined yet (can be used for Pressure, LN2 or Temperature Readout)
#define AnalogInput2 A5
#define AnalogInput3 A6

// Button Pins on Front Panel (Digital Pins)
#define PinButton1 24 // Button Left, Top
#define PinButton2 23 // Button Left, Middle
#define PinButton3 22 // Button Left, Bottom
#define PinButton4 25 // Button Right, Top
#define PinButton5 26 // Button Right, Middle
#define PinButton6 27 // Button Right, Bottom

int PinButtonArray[6]= {PinButton1,PinButton2,PinButton3,PinButton4,PinButton5,PinButton6};
int lastButtonStates[6] = {0,0,0,0,0,0}; // Needed to compare if button is pressed or not
int presentButtonStates[6];
int buttonPressTimer;

// Pins UART USB:
#define USBUARTRXD 0 // USB UART RX
#define USBUARTTXD 1 // USB UART TX

// Pins PowerSwitch ("Leistungsschalter"), 24V 0.25A max:
#define PinPowerSwitch1 37 //D37, Vent Valve Dock OPEN
#define PinPowerSwitch2 38 //D38, Angle Valve Dock OPEN
#define PinPowerSwitch3 39 //D39, Gate Valve Dock OPEN
#define PinPowerSwitch4 40 //D40, Gate Valve Dock CLOSE

// Pins Open Collector Output 40V, ca. 30mA
#define PinOpenCollectorOutput1 41 //D41 //LN2 Dewar
#define PinOpenCollectorOutput2 42 //D42
#define PinOpenCollectorOutput3 43 //D43 //Angle Valve Turbo Pump
#define PinOpenCollectorOutput4 44 //D44
#define PinOpenCollectorOutput5 45 //D45
#define PinOpenCollectorOutput6 46 //D46

// Digital IN isolated Pins, Group 1, 3 - 24V, Rin = 1kOhm, inverted, pullup
#define PinDigitalInIsolated1 53 //D53, Group 1
#define PinDigitalInIsolated2 51 //D51, Gate Valve Allowed to be Opened, From SEM
#define PinDigitalInIsolated3 50 //D50, Group 1

// Digital IN isolated Pins, Group 2, 3 - 24V, Rin = 1kOhm, inverted, pullup
#define PinDigitalInIsolated4 49 //D49, Group 2
#define PinDigitalInIsolated5 48 //D48, Group 2
#define PinDigitalInIsolated6 47 //D47, Group 2 , Signal Gate Valve Closed, inverted, pullup

// Ditital Pins MOS-Contact, 40V 0.3A max
#define PinMOS1 34 //D34 "0"=Open, Scroll Pump ON
#define PinMOS2 35 //D35 "0"=Open
#define PinMOS3 36 //D36 "0"=Open, Turbo Pump ON

// Interaction With SEM:
#define PrepChamberPressureReturn DAC0 //This Pin Returns the Pressure Readout to the SEM.


// Beep
#define PinBeep 52 //D52, Beeper

//Menu Toggle and Switch Parameters:
const int menuSize = 4;
String menuItems[menuSize]={"Welcome Screen","Main Menu 1","Main Menu 2","Settings Menu"};
int currentMenu =0;
int previousMenu =0;



/******************
  SD card Data and Settings Storage:
 *****************/
File myFile;
int settingsMemory[8] ={0,0,0,0,0,0,0,0};
  // The non-volatile data is stored as following:
  // Storage 1 (settingsMemory[0]) [Pressure Unit]: 0=mbar, 1=Torr, 2=Pascal
  // Storage 2 (settingsMemory[1]) [Temperature Unit]: 0=°C, 1=K, 2=°F
  // Storage 3 (settingsMemory[2]) [Background Color]: 0= Blue, 1= Black, 2= Red, 3= Green, 4= Cyan, 5= Magenta, 6= Yellow, 7= White 
  // Storage 4 (settingsMemory[3]) [Font Color]: 0= White, 1= Black
  // Storage 5 (settingsMemory[4]) [Heater Timer]: 0=60, 1=120, 2=300, 3=600, 4=900, 5=1200, 6=1500, 7=1800, 8=3600
  // Storage 6 (settingsMemory[5]) [Safety Mode]: 0= SEM Interact ON, 1= SEM Interact OFF, 2= Debug Mode
  // Storage 7 (settingsMemory[6]) [LN2 Dewar Filling]: 0= Auto 100%, 1= Auto 50%, 2 = Manual 
int indx =0;
signed int SD_string_value;
char writetomemory;

/******************
  Color Definitions:
*****************/
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

int colorBackground[8] = {BLUE, BLACK, RED, GREEN, CYAN, MAGENTA, YELLOW, WHITE};
String colorDisplayed[8] = {" Blue"," Black"," Red","Green"," Cyan","Magenta","Yellow","White"};
int colorFont[2] = {WHITE, BLACK};
// Use hardware SPI
Adafruit_ILI9340 tft = Adafruit_ILI9340(TFT_CS, TFT_DC, TFT_RST);

int switchColor;

/*****************
 Define Variables: Temperature and Pressure
****************/
float pressure1 = 2E-8;
float pressure2 = 1E-10;
String temperatureMeasured[3] = {"--","--","--"};
int Pressure;
int ADCLN2;
String pressureMeasured[3] = {"--","--","--"};
String LN2Measured[1] = {"--"};

float pInMbar;
float pInTorr;
float pInPascal;


String pressureUnit[3] = {"mbar", "Torr", "Pa"};
String temperatureUnit[3] = {String(char(167)) +"C", "K", String(char(167)) + "F"};
float value;
double randNumber1;
double randNumber2;
double randNumber3;
double randNumber4;
double logValue;
signed int counter;
String displayedNumber;
boolean heaterBoolean = false;
boolean heaterIsOnCheck = false;
boolean modeBooleanLN2Dewar = false;
boolean scrollPumpBoolean = false;
boolean turboPumpBoolean = false;
boolean gateValveBoolean = false;
boolean modeBooleanScroll = false;
boolean loadLockBoolean = true;
boolean cryoDockBoolean = true;
boolean settingsBoolean =false;
boolean nextMenuBoolean = false;
boolean previousMenuBoolean =false;
String message = "";

//Heater: Predefined Values (in Seconds):
int heaterTimingValues[16] = {60,120,300,600,900,1200,1500,1800,3600};
String heaterTimingValuesDisplayed[16] = {"1 min","2 min","5 min","10min","15min","20min","25min","30min", "60min"};

//Safety Mode: Predefined Values (Displayed):
String safetyModeValuesDisplayed[3] = {"ON", "OFF", "Debug"};

// LN2 Dewar Filling: Predefinde Values (Displayed):
String LN2DewarValuesDisplayed[3] = {"Auto 50%", "Auto 100%", "Manually"};

/*****************
  Timer Variables:
****************/
unsigned long previousTime = millis();
unsigned long afterTime = 0;
unsigned long deltaTime = 0;
unsigned long seconds ;
unsigned long minutes ;
unsigned long hours ;
unsigned long timeSeconds=0 ;
unsigned long delayedTime =0;
int refreshingTimer =1000;
unsigned long clockTime;




void setup() {
  Serial.begin(9600);
  pinMode(PinButton1, INPUT);
  pinMode(PinButton2, INPUT);
  pinMode(PinButton3, INPUT);
  pinMode(PinButton4, INPUT);
  pinMode(PinButton5, INPUT);
  pinMode(PinButton6, INPUT);
  pinMode(PinBeep, OUTPUT);
  pinMode(PinDisplayBL,OUTPUT);
  digitalWrite(PinDisplayBL, HIGH);
  
  pinMode(PinPressure1, INPUT); //Pfeiffer Pressure Gauge 1
  pinMode(PinPressure2, INPUT); //Pfeiffer Pressure Gauge 2
  pinMode(PinTemperature1,INPUT); 
  pinMode(PinTemperature2,INPUT); 
  pinMode(PinTemperature3,INPUT); 
  pinMode(PinLN2, INPUT); //LN2 Level Sensor
  pinMode(PinPowerSwitch1, OUTPUT); //Vent Valve Dock OPEN
  pinMode(PinPowerSwitch2, OUTPUT); // Angle Valve Dock OPEN
  pinMode(PinPowerSwitch3, OUTPUT); //Gate Valve OPEN
  pinMode(PinPowerSwitch4, OUTPUT); //Gate Valve CLOSE
  pinMode(PinMOS1,OUTPUT); // Scroll Pump ON
  pinMode(PinMOS3,OUTPUT); // Turbo Pump On
  
  pinMode(PinDigitalInIsolated2, INPUT_PULLUP); // Gate Valve allowed to be opened from SEM
  pinMode(PinDigitalInIsolated6,INPUT_PULLUP); //Signal Gate Valve Closed
  pinMode(PinOpenCollectorOutput1,OUTPUT); // LN2 Dewar
  pinMode(PinOpenCollectorOutput3,OUTPUT); // Angle Valve Turbo Pump
  
  analogReadResolution(12);  //set ADC resolution to 12 bits. If this is not declared, the Arduino Due Board sets the ADC to a standard resolution of 10bits
  analogWriteResolution(12); //set ADC resolution to 12 bits. IF this is not declared, the Arduino Due Board sets the ADC to a standard resolution of 8 bits
  tft.begin();
  tft.setRotation(3);

  //Vent the Load Lock at the Startup to quickly reset the Pressure to Atmospheric Pressure
  
  SDCardInitialization();
  settingsMemory[5]=0;
  // 
  if(digitalRead(PinDigitalInIsolated6)==false){// When booting, check if the Gate Valve Sensor says if it is open or closed. 
    gateValveBoolean = true;
  } else{
    gateValveBoolean = false;
  }

  welcomeScreen();
}


void loop(void) {
  serialCommands();
  
  menuChanged();
  // the internal clock of an Arduino resets after about 50 days. To prevent a crash, the following code is implemented:
  if (afterTime < previousTime){
    afterTime = millis();
    previousTime = millis();
  }
  // Instead of doing a delay() function and block the whole loop, one can implement a delay this way:    
  if (millis() - previousTime >refreshingTimer){
    previousTime = millis();
    MeasureTemperature();
    MeasurePressure();
    MeasureLN2();

    //LN2 Level check and adjust
    if (modeBooleanLN2Dewar == true) {
      ADCLN2 = ADC12bit(PinLN2Array[0]);
      if(settingsMemory[6] == 0){ //Auto 50%
        if(ADCLN2 <= 1650){ // ~25%
          digitalWrite(PinOpenCollectorOutput1,HIGH);
        }if(ADCLN2 >=2500){ // 50%
          digitalWrite(PinOpenCollectorOutput1,LOW);
        }         
      } else if(settingsMemory[6] == 1){ //Auto 100%
        if(ADCLN2 <= 2500){ // 50%
          digitalWrite(PinOpenCollectorOutput1,HIGH);
         }if(ADCLN2 >=3650){ // 100%
          digitalWrite(PinOpenCollectorOutput1,LOW);
         }         
      } else if(settingsMemory[6] == 2){ //Manually
        if(ADCLN2 <= 2500){ // 50%
          digitalWrite(PinOpenCollectorOutput1,HIGH);
        }if(ADCLN2 >=3650){ // 100%
          digitalWrite(PinOpenCollectorOutput1,LOW);
          modeBooleanLN2Dewar = false;
          if(currentMenu ==1){
            menuLN2Dewar();
          }
         }         
        }
                
    }else{
      digitalWrite(PinOpenCollectorOutput1,LOW); 
    }
    if(currentMenu ==1){ //Main Menu 1
          displayLN2();
          displayTemperature2();
          displayPressure1();
          displayPressure2();
          displayTimer();
          buttonPressedLong();
    }
    if(currentMenu ==2){ //Main Menu 2
          displayLN2();
          displayTemperature2();
          displayPressure1();
          displayPressure2();
          displayTimer();
          buttonPressedLong();
    }       
  }
  if(currentMenu ==3){
    digitalWrite(PinOpenCollectorOutput1,LOW); //prevent that LN2 is uncontrollably filled
    buttonPressedSimpleClick();
  }
  afterTime = millis();
  deltaTime = afterTime-previousTime;
}

void serialCommands() {
  if(Serial.available() > 0) {
    message = Serial.readStringUntil('\n');
    currentMenu = 1;
    //else if (message == "LSCHeaterOff"){heaterBoolean = false; menuHeater(); Serial.println(0);}
    //else if (message == "LSCHeaterOn"){heaterBoolean = true; menuHeater(); Serial.println(1);}
    //else if (message == "LSCScrollPumpOff"){scrollPumpBoolean = false; menuScrollPump(); Serial.println(0);}
    //else if (message == "LSCScrollPumpOn"){scrollPumpBoolean = true; menuScrollPump(); Serial.println(1);}
    //if (message == "LSCTurboPumpOff"){turboPumpBoolean = false; menuTurboPump(); Serial.println(0);}
    //else if (message == "LSCTurboPumpOn"){turboPumpBoolean = true; menuTurboPump(); Serial.println(1);}
    if (message == "LSCGateValveClose"){gateValveBoolean = false; menuGateValve(); Serial.println(0);}
    else if (message == "LSCGateValveOpen"){gateValveBoolean = true; menuGateValve(); Serial.println(1);}
    //else if (message == "LSCModePumpLL"){modeBooleanVent = false; menuModeLoadLock(); Serial.println(1);}
    //else if (message == "LSCModeVentLL"){modeBooleanVent = true; menuModeLoadLock(); Serial.println(0);}
    else if (message == "LSCColorBlue"){colorSettings(0); saveToSDCard(); Serial.println(1);}
    else if (message == "LSCColorBlack"){colorSettings(1); saveToSDCard(); Serial.println(1);}
    else if (message == "LSCColorRed"){colorSettings(2); saveToSDCard(); Serial.println(1);}
    else if (message == "LSCColorGreen"){colorSettings(3); saveToSDCard(); Serial.println(1);}
    else if (message == "LSCColorCyan"){colorSettings(4); saveToSDCard(), Serial.println(1);}
    else if (message == "LSCColorMagenta"){colorSettings(5); saveToSDCard(); Serial.println(1);}
    else if (message == "LSCColorYellow"){colorSettings(6); saveToSDCard(); Serial.println(1);}
    else if (message == "LSCColorWhite"){colorSettings(7); saveToSDCard(); Serial.println(1);}  
    else if (message.startsWith("LSCSetTunit")> 0){
      message.remove(0,11);      
      if(message == "C"){
        settingsMemory[1]=0;
      }
      else if(message == "K"){
        settingsMemory[1]=1;
      }
      else if(message == "F"){
        settingsMemory[1]=2;
      }
      displayMainMenu();
      //menuHeater();
      menuLN2Dewar();
      menuGateValve();
      menuSettings();
      menuModeCryoDock();
      saveToSDCard();
    }
  
    else if (message.startsWith("LSCSetPunit")> 0){
      message.remove(0,11);      
      if(message == "M"){
        settingsMemory[0]=0;
      }
      else if(message == "T"){
        settingsMemory[0]=1;
      }
      else if(message == "P"){
        settingsMemory[0]=2;
      }
      displayMainMenu();
      menuLN2Dewar();
      //menuHeater();
      menuGateValve();
      menuSettings();
      menuModeCryoDock();
      saveToSDCard();
    }
  }
}

void SDCardInitialization(){
  if (!SD.begin(SD_CS)){
    Serial.println("No SD Card");
  } 
  else{
    Serial.println("SD Card OK");
    if (SD.exists("original.txt")){
      myFile = SD.open("original.txt", FILE_READ);
  
      while (myFile.available()){
        SD_string_value = myFile.readStringUntil('\n').toInt();
        settingsMemory[indx] = SD_string_value;
        indx ++;
      } 
      
      myFile.close();
      indx = 0;
    } 
    else{
      myFile = SD.open("original.txt", FILE_WRITE);
      myFile.write("0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n");
      myFile.close();
    }
  }
}

void saveToSDCard(){
  // Saves Settings to SD Card
  if (!SD.begin(SD_CS)){
    Serial.println("No SD Card");
  } 
  else{
    Serial.println("SD Card OK");
    SD.remove("original.txt");
    myFile = SD.open("original.txt", FILE_WRITE);
    myFile.println(settingsMemory[0]);
    myFile.println(settingsMemory[1]);
    myFile.println(settingsMemory[2]);
    myFile.println(settingsMemory[3]);
    myFile.println(settingsMemory[4]);
    myFile.println(settingsMemory[5]);
    myFile.println(settingsMemory[6]);
    myFile.println(settingsMemory[7]);
    myFile.close();
  }
}

void welcomeScreen() {
  Serial.println("LSC1 Controller Unit"); 
  Serial.println("Ferrovac GmbH, Thurgauerstrasse 72, 8052 Zürich");
  resetBackground();
  tft.setFont(&bgothm12pt7b);
  tft.setCursor(40, 40);
  tft.setTextSize(2);
  tft.println("Ferrovac");
  tft.setTextSize(1);
  tft.setCursor(53, 60);
  tft.println("UHV Technology");
  tft.setCursor(130, 130);
  tft.println("LSC1");
  
  tft.setCursor(66, 200);
  tft.println("...Please Wait...");
  tft.setFont();
  
  //Print Version Number of the Program
  tft.setCursor(90,210);
  tft.println(versionNumber);

  //Vent the Dock at Startup
  digitalWrite(PinPowerSwitch1,HIGH); //Vent Valve OPEN
  delay(2000);
  digitalWrite(PinPowerSwitch1,LOW); //Vent Valve OPEN 
  delay(2000);
  
  currentMenu = 1;
}

void displayLN2() {
  // Define rectangle where to display the temperature
  int x1 = 40; //rectangle x starting point
  int x2 = 127; //rectangle x end point
  int dx = x2-x1; //width
  int y1 = 15; //rectangle y starting point
  int y2 = 40; //rectangle y end point
  int dy = y2-y1; //height
  
  tft.fillRect(x1, y1, dx, dy, colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setCursor(x1, y1);
  tft.setTextSize(2);
  tft.println(LN2Measured[0]);
}

void displayTemperature1() {
  // Define rectangle where to display the temperature
  int x1 = 40; //rectangle x starting point
  int x2 = 127; //rectangle x end point
  int dx = x2-x1; //width
  int y1 = 15; //rectangle y starting point
  int y2 = 40; //rectangle y end point
  int dy = y2-y1; //height
  
  tft.fillRect(x1, y1, dx, dy, colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setCursor(x1, y1);
  tft.setTextSize(2);
  tft.println(temperatureMeasured[0]);
}

void displayTemperature2() {
  // Define rectangle where to display the temperature
  int x1 = 40; //rectangle x starting point
  int x2 = 127; //rectangle x end point
  int dx = x2-x1; //width
  int y1 = 45; //rectangle y starting point
  int y2 = 70; //rectangle y end point
  int dy = y2-y1; //height
  
  tft.fillRect(x1, y1, dx, dy, colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setCursor(x1, y1);
  tft.setTextSize(2);
  tft.println(temperatureMeasured[1]);
}

void displayPressure1() {
  // Define rectangle where to display the temperature
  int x1 = 201; //rectangle x starting point
  int x2 = 288; //rectangle x end point
  int dx = x2-x1; //width
  int y1 = 15; //rectangle y starting point
  int y2 = 40; //rectangle y end point
  int dy = y2-y1; //height
  
  tft.fillRect(x1, y1, dx, dy, colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setCursor(x1, y1);
  tft.setTextSize(2);
  tft.println(pressureMeasured[0]);
}

void displayPressure2() {
  // Define rectangle where to display the temperature
  int x1 = 201; //rectangle x starting point
  int x2 = 288; //rectangle x end point
  int dx = x2-x1; //width
  int y1 = 45; //rectangle y starting point
  int y2 = 70; //rectangle y end point
  int dy = y2-y1; //height

  //counter =0;
  //randNumber1 = float(random(1,1000));
  //randNumber2 = float(random(1,100000000000));
  //value = float(randNumber1/randNumber2);

  //if (value>=1) {
    //value = int(value);
    //String valueString = String(value);
    //tft.fillRect(x1, y1, dx, dy, colorBackground[settingsMemory[2]]);
    //tft.setCursor(x1, y1);
    //tft.setTextSize(2);
    //tft.println(value,0);
  //}
  //else {
    //while (value <1){
      //value = value*10;
      //counter = counter -1;
    //}
    //value = int(value*10);
    //value = float(value/10);
    tft.fillRect(x1, y1, dx, dy, colorBackground[settingsMemory[2]]);
    tft.setCursor(x1, y1);
    tft.setTextSize(2);
    //tft.print(value,1); tft.print("E"); tft.print(counter);
    tft.println(pressureMeasured[1]);
  //}
}



void displayTimer(){
  if((heaterBoolean==true)&& (delayedTime<=heaterTimingValues[settingsMemory[4]])){
    heaterIsOnCheck =true;
    clockTime = heaterTimingValues[settingsMemory[4]]-delayedTime;    
    hours =0;
    minutes = 0;
    seconds = 0;       
    
    while (clockTime >=3600){
      clockTime = clockTime -3600;
      hours = hours +1;
    }
    
    while (clockTime >=60){
      clockTime = clockTime -60;
      minutes = minutes +1;
    }
   
    seconds = int(clockTime);
    tft.fillRect(30, 120, 82, 15, colorFont[settingsMemory[3]]);
    tft.setTextColor(colorBackground[settingsMemory[2]]);
    tft.setCursor(30, 120);
    tft.print (hours, DEC);

    if (minutes <10){
      tft.print (":0");
    }
    else{
      tft.print (":");
    }
    tft.print (minutes,DEC);
    
    if (seconds <10){
      tft.print (":0");
    }
    else{
      tft.print (":");
    }
    tft.println(seconds,DEC);   
    tft.setTextColor(colorFont[settingsMemory[3]]);
    delayedTime = delayedTime + deltaTime/1000;
    
  }
  else{
    heaterBoolean =false;
    if(heaterIsOnCheck ==true){
      heaterIsOnCheck = false;
      menuHeater();
    }
  }
}

void resetBackground() {
  //Resets the  Plain Background
  tft.fillScreen(colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
}

void displayMainMenu() {
  //Main Menu: Displays Background and Rectangles for Main Menu
  resetBackground();
  settingsBoolean=false;
  nextMenuBoolean = false;
  previousMenuBoolean = false;
  tft.fillRect(0, 79, 320, 3, colorFont[settingsMemory[3]]); //1st horizontal line
  tft.fillRect(0, 159, 320, 3, colorFont[settingsMemory[3]]); //2nd horizontal line
  tft.fillRect(159, 0, 3, 240, colorFont[settingsMemory[3]]); //1st vertical line
  tft.setTextSize(2);

  //Temperature:
  tft.setTextSize(1);
  tft.setCursor(10, 20);
  tft.println("LN2:");
  tft.setCursor(140, 20);
  tft.println("%");
  tft.setCursor(10, 50);
  tft.println("T1:");
  tft.setCursor(140, 50);
  tft.println(temperatureUnit[settingsMemory[1]]);

  //Pressure:
  tft.setTextSize(1);
  tft.setCursor(171, 20);
  tft.println("P1:");
  tft.setCursor(290, 20);
  tft.println(pressureUnit[settingsMemory[0]]);
  tft.setCursor(171, 50);
  tft.println("P2:");
  tft.setCursor(290, 50);
  tft.println(pressureUnit[settingsMemory[0]]);
}

void displaySettingsMenu(int currentSettingsMenu){
  if(currentSettingsMenu==3){
    resetBackground();
    tft.fillRect(0, 79, 320, 3, colorFont[settingsMemory[3]]); //1st horizontal line
    tft.fillRect(0, 159, 320, 3, colorFont[settingsMemory[3]]); //2nd horizontal line
    tft.fillRect(159, 80, 3, 160, colorFont[settingsMemory[3]]); //1st vertical line

    //Exit Symbol:
    tft.fillRect(10,22,36,36,colorFont[settingsMemory[3]]);
    tft.fillRect(13,25,30,30,colorBackground[settingsMemory[2]]);
    tft.setTextSize(3);
    tft.setTextColor(colorFont[settingsMemory[3]]);
    tft.setCursor(20, 30);
    tft.println("X");
    
    tft.setTextSize(2);
    tft.setCursor(93, 30);
    tft.println("Settings Menu");
    //tft.println(menuItems[currentSettingsMenu]);
  }

// In case you want to add more Settings Menues, uncomment this part
//  if(currentSettingsMenu==3){
//    resetBackground();
//    tft.fillRect(0, 79, 320, 3, colorFont[settingsMemory[3]]); //1st horizontal line
//    tft.fillRect(0, 159, 320, 3, colorFont[settingsMemory[3]]); //2nd horizontal line
//    tft.fillRect(159, 80, 3, 160, colorFont[settingsMemory[3]]); //1st vertical line
//    tft.setTextSize(2);
//    tft.setCursor(65, 20);
//    tft.println(menuItems[currentSettingsMenu]);
//  }
}

void scrollPumpON(){
  digitalWrite(PinMOS1, HIGH); // Scroll Pump ON
  scrollPumpBoolean == true;
}
void scrollPumpOFF(){
  digitalWrite(PinMOS1, LOW); // Scroll Pump ON
  scrollPumpBoolean == false;  
}
void turboPumpON(){
  digitalWrite(PinMOS3, HIGH); // Turbo Pump ON
  turboPumpBoolean ==true;  
}
void turboPumpOFF(){
  digitalWrite(PinMOS3, LOW); // Turbo Pump OFF
  turboPumpBoolean = false;
}

void menuLoadLock(){
  Serial.println(loadLockBoolean);
  digitalWrite(PinOpenCollectorOutput1,LOW); //Turn off LN2 Dewar in case it is open (Safety Feature)
  Beeper(20);
  if (loadLockBoolean == true) { //VENT
    if (gateValveBoolean == true){
        tft.setTextColor(colorFont[settingsMemory[3]]);
        tft.setTextSize(1);
        tft.setCursor(30, 118);
        tft.println("Close Gate Valve");
        Beeper(2000);
        tft.fillRect(25, 112, 100, 15, colorBackground[settingsMemory[2]]);
        loadLockBoolean = false;
        return;          
    }

    tft.fillRect(2, 84, 155, 73, colorFont[settingsMemory[3]]);
    tft.setTextColor(colorBackground[settingsMemory[2]]);
    tft.setTextSize(2);
    tft.setCursor(25, 87);
    tft.println("LOAD LOCK");

    digitalWrite(PinPowerSwitch2, LOW); //Angle Valve Dock Close
    if(cryoDockBoolean == false){// if cryo dock is pumped
      digitalWrite(PinOpenCollectorOutput3,HIGH);//Angle Valve Turbo OPEN
      turboPumpON();
      scrollPumpON();
    }else{
      digitalWrite(PinOpenCollectorOutput3,LOW);//Angle Valve Turbo CLOSE
      turboPumpOFF();
      scrollPumpOFF();
    }

    int  ReadOut12bit = ADC12bit(PinPressureArray[0]);
    if(ReadOut12bit <= 3800){
      tft.setCursor(55, 140);
      tft.println("WAIT");
      tft.fillRect(25, 112, 120, 15, colorFont[settingsMemory[3]]);
      tft.setTextSize(1);
      tft.setCursor(30, 118);
      tft.println("Close Angle Valve"); 
      digitalWrite(PinPowerSwitch2, LOW); //Angle Valve Dock Close
      delay(2000);
  
      tft.fillRect(25, 112, 120, 15, colorFont[settingsMemory[3]]);
      tft.setTextSize(1);
      tft.setCursor(35, 118);
      tft.println("Open Vent Valve");
       
      int  ReadOut12bit = ADC12bit(PinPressureArray[0]);
      int countLoop = 0;    
      while (ReadOut12bit <= 3950){
        //Update Temperature and Pressure Measurements
        digitalWrite(PinPowerSwitch1,HIGH); //Vent Valve OPEN
        MeasureTemperature();
        MeasurePressure();
        displayTemperature1();
        displayTemperature2();
        displayPressure1();
        displayPressure2();
        delay(400);
        ReadOut12bit = ADC12bit(PinPressureArray[0]);
        ++ countLoop ;
        if(countLoop >=25){  //STOP the while loop if it has been vented for longer than 10 seconds
          ReadOut12bit = 4000;
        }
      } 
      tft.fillRect(25, 112, 100, 15, colorFont[settingsMemory[3]]);
      tft.setTextSize(1);
      tft.setCursor(30, 118);
      tft.println("Close Vent Valve");
      digitalWrite(PinPowerSwitch1,HIGH); //Vent Valve OPEN
      delay(2000);
      digitalWrite(PinPowerSwitch1,LOW); //Vent Valve CLOSE
    }
    digitalWrite(PinPowerSwitch1,LOW); //Vent Valve CLOSE
    tft.fillRect(25, 112, 100, 15, colorFont[settingsMemory[3]]);
    tft.fillRect(40, 135, 100, 20, colorFont[settingsMemory[3]]);
    tft.setTextColor(colorBackground[settingsMemory[2]]);
    tft.setCursor(45, 140);
    tft.setTextSize(2);
    tft.println("VENTED");  
  } else{ //loadLockBoolean = false; PUMP
    tft.fillRect(2, 84, 155, 73, colorBackground[settingsMemory[2]]);
    tft.setTextColor(colorFont[settingsMemory[3]]);
    tft.setTextSize(2);
    tft.setCursor(25, 87);
    tft.println("LOAD LOCK");

    tft.setCursor(55, 140);
    tft.println("WAIT");

    int  ReadOut12bit = ADC12bit(PinPressureArray[0]);
    if(ReadOut12bit >= 2800 || ReadOut12bit <=256){
      if(cryoDockBoolean == false){//if cryo Dock is pumped
        digitalWrite(PinOpenCollectorOutput3,LOW);//Angle Valve Turbo CLOSE
        turboPumpON();
        scrollPumpON();
      }else{
        digitalWrite(PinOpenCollectorOutput3, LOW); // Angle Valve Turbo Pump CLOSE
        turboPumpOFF();
        scrollPumpON();
        tft.fillRect(25, 112, 120, 15, colorBackground[settingsMemory[2]]);
        tft.setTextColor(colorFont[settingsMemory[3]]);
        tft.setTextSize(1);
        tft.setCursor(30, 118);
        tft.println("Turn On Scroll"); 
        delay(1000);        
      }
      tft.fillRect(25, 112, 120, 15, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setTextSize(1);
      tft.setCursor(40, 118);
      tft.println("Pumping Down"); 

      delay(1000);
      digitalWrite(PinPowerSwitch2, HIGH); //Angle Valve Dock OPEN

      int countLoop =0;
      ReadOut12bit = ADC12bit(PinPressureArray[0]); // Read the pressure from the pressure gauge
      while (ReadOut12bit >= 2800 || ReadOut12bit <= 256){
            //Update Temperature and Pressure Measurements
            MeasureTemperature();
            MeasurePressure();
            displayTemperature1();
            displayTemperature2();
            displayPressure1();
            displayPressure2();
            delay(400);
            ReadOut12bit = ADC12bit(PinPressureArray[0]);
            ++ countLoop ;
            if(countLoop >=40){  //STOP the while loop if it has been vented for longer than 10 seconds
              ReadOut12bit = 1000;
            }
      }

    } 
    // pressure in range
    if(cryoDockBoolean == false){ //if cryo dock is pumped        
      turboPumpON();
      scrollPumpON();
      digitalWrite(PinOpenCollectorOutput3,HIGH);//Angle Valve Turbo OPEN
      digitalWrite(PinPowerSwitch2, HIGH); //Angle Valve Dock OPEN

    }else{
      digitalWrite(PinOpenCollectorOutput3, LOW); // Angle Valve Turbo Pump CLOSE
      turboPumpOFF();
      scrollPumpON();
      digitalWrite(PinPowerSwitch2, HIGH); //Angle Valve Dock OPEN    
    }
    
    tft.fillRect(25, 112, 120, 15, colorBackground[settingsMemory[2]]);
    tft.fillRect(40, 135, 100, 20, colorBackground[settingsMemory[2]]);
    tft.setCursor(45, 140);
    tft.println("PUMPED");
  }
}

void menuLN2Dewar(){
  Beeper(20);
  if (modeBooleanLN2Dewar == true) {
      //Filling ON
      tft.fillRect(2, 84, 155, 73, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(25, 87);
      tft.println("LN2 DEWAR");
      
      tft.fillRect(25, 112, 100, 15, colorFont[settingsMemory[3]]);
      tft.setTextSize(1);
      tft.setCursor(50, 118);
      tft.println(LN2DewarValuesDisplayed[settingsMemory[6]]);

      tft.setTextSize(2);
      tft.setCursor(63, 140);
      tft.println("ON");  
      tft.setTextColor(colorFont[settingsMemory[3]]);
  } else{
      //Filling OFF:
      tft.fillRect(2, 84, 155, 73, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setTextSize(2);
      tft.setCursor(25, 87);
      tft.println("LN2 DEWAR"); 
      tft.fillRect(25, 112, 100, 15, colorBackground[settingsMemory[2]]);
      tft.setTextSize(1);
      tft.setCursor(50, 118);
      tft.println(LN2DewarValuesDisplayed[settingsMemory[6]]);
      tft.setTextSize(2);
      tft.setCursor(55, 140);
      tft.println("OFF");
      
  }
}

void menuHeater() {
  //Heater Menu:
  digitalWrite(PinOpenCollectorOutput1,LOW); //Turn off LN2 Dewar in case it is open (Safety Feature)
  Beeper(20);
  if (heaterBoolean == true) {
      tft.fillRect(2, 84, 155, 73, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(35, 87);
      tft.println("HEATER");
      tft.setCursor(58, 140);
      tft.println("ON");  
      tft.setTextColor(colorFont[settingsMemory[3]]);
  }
  else{
      //Heater Menu:         
      tft.fillRect(2, 84, 155, 73, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setTextSize(2);
      tft.setCursor(35, 87);
      tft.println("HEATER"); 
      tft.setCursor(55, 140);
      tft.println("OFF");
      delayedTime =0;
      clockTime = heaterTimingValues[settingsMemory[4]];
      hours =0;
      minutes = 0;
      seconds = 0;
               
      while (clockTime >=3600){
        clockTime = clockTime -3600;
        hours = hours +1;
      }
      while (clockTime >=60){
        clockTime = clockTime -60;
        minutes = minutes +1;
      }
      seconds = int(clockTime);
      
      tft.fillRect(30, 120, 82, 15, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setCursor(30, 120);
      tft.print (hours, DEC);

      if (minutes <10){
        tft.print (":0");
      }
      else{
        tft.print (":");
      }
      tft.print (minutes,DEC);
      if (seconds <10){
        tft.print (":0");
      }
      else{
        tft.print (":");
      }
      tft.println(seconds,DEC);        
  }  
}

//void menuModeLoadLock() {
//  //Mode Menu: Switch Display Values between Vent and Pump
//  digitalWrite(PinOpenCollectorOutput1,LOW); //Turn off LN2 Dewar in case it is open (Safety Feature)
//  Beeper(20);
//  if (modeBooleanVent == true) {//Vent
//      if (gateValveBoolean == true){
//        tft.setTextColor(colorFont[settingsMemory[3]]);
//        tft.setTextSize(1);
//        tft.setCursor(30, 195);
//        tft.println("Close Gate Valve");
//        Beeper(2000);
//        tft.fillRect(25, 195, 100, 20, colorBackground[settingsMemory[2]]);
//        modeBooleanVent = false;
//        return;          
//      }
//      tft.fillRect(2, 164, 155, 73, colorFont[settingsMemory[3]]);
//      tft.setTextColor(colorBackground[settingsMemory[2]]);
//      tft.setTextSize(2);
//      tft.setCursor(25, 167);
//      tft.println("LOAD LOCK");
//      tft.setTextSize(2);
//      tft.setCursor(50, 220);
//      tft.println("WAIT");
//      //Close the Angle Valve
//      tft.fillRect(2, 190, 155, 15, colorFont[settingsMemory[3]]);
//      tft.setTextSize(1);
//      tft.setCursor(25, 195);
//      tft.println("Angle Valve Closed");
//      digitalWrite(PinPowerSwitch2,LOW); //Angle Valve Dock CLOSE
//      digitalWrite(PinOpenCollectorOutput3,LOW); //Angle Valve Turbo CLOSE
//      delay(500);
//      digitalWrite(PinMOS1, LOW); //Scroll Pump OFF
//      scrollPumpBoolean = false;
//      digitalWrite(PinMOS3, LOW); //Turbo Pump OFF
//      turboPumpBoolean = false;
//      
//      //Update Temperature and Pressure Measurements
//      MeasureTemperature();
//      MeasurePressure();
//      displayTemperature1();
//      displayTemperature2();
//      displayPressure1();
//      displayPressure2();
//
//      // Open the Vent Valve
//      tft.fillRect(2, 190, 155, 15, colorFont[settingsMemory[3]]);
//      tft.setTextColor(colorBackground[settingsMemory[2]]);
//      tft.setTextSize(1);
//      tft.setCursor(25, 195);
//      tft.println("Vent Valve Opened");
//      // Open the Vent Valve for <4 seconds or until the pressure is about atmospheric pressure
//      int countLoop = 0;
//      float ReadOut12bit;
//      ReadOut12bit = ADC12bit(PinPressureArray[0]); // Read the pressure from the pressure gauge
//      while (ReadOut12bit < 4000){
//        //Update Temperature and Pressure Measurements
//        digitalWrite(PinPowerSwitch1,HIGH); //Vent Valve OPEN
//        MeasureTemperature();
//        MeasurePressure();
//        displayTemperature1();
//        displayTemperature2();
//        displayPressure1();
//        displayPressure2();
//        delay(400);
//        ReadOut12bit = ADC12bit(PinPressureArray[0]);
//        ++ countLoop ;
//        if(countLoop >=12){  //STOP the while loop if it has been vented for longer than 4 seconds
//          ReadOut12bit = 4096;
//        }
//      }
//      //Close the Vent Valve
//      tft.fillRect(2, 190, 155, 15, colorFont[settingsMemory[3]]);
//      tft.setTextColor(colorBackground[settingsMemory[2]]);
//      tft.setTextSize(1);
//      tft.setCursor(22, 195);
//      tft.println("Closing Vent Valve");
//      delay(3000);
//      digitalWrite(PinPowerSwitch1,LOW); // Vent Valve CLOSE
//      tft.fillRect(2, 190, 155, 15, colorFont[settingsMemory[3]]);
//      tft.fillRect(2,220,155,15, colorFont[settingsMemory[3]]);
//      tft.setTextColor(colorBackground[settingsMemory[2]]);
//      tft.setTextSize(2);
//      tft.setCursor(40, 220);
//      tft.println("VENTED");
//      tft.setTextColor(colorFont[settingsMemory[3]]);
//  }
//  else { //Pump
//      tft.fillRect(2, 164, 155, 73, colorBackground[settingsMemory[2]]);
//      tft.setTextColor(colorFont[settingsMemory[3]]);
//      tft.setTextSize(2);
//      tft.setCursor(25, 167);
//      tft.println("LOAD LOCK"); 
//      tft.setCursor(50, 220);
//      tft.println("WAIT");
//      float ReadOut12bit;
//      ReadOut12bit = ADC12bit(PinPressureArray[0]); // Read the pressure from the pressure gauge at the dock
//      //Close Angle Valves
//      digitalWrite(PinPowerSwitch2,LOW); //Angle Valve Dock CLOSE
//      digitalWrite(PinOpenCollectorOutput3,LOW); //Angle Valve Turbo CLOSE
//      delay(500);
//
//      tft.fillRect(2, 190, 155, 15, colorBackground[settingsMemory[2]]);
//      tft.setTextColor(colorFont[settingsMemory[3]]);
//      tft.setTextSize(1);
//      tft.setCursor(40, 195);
//      tft.println("Pumping Down");      
//      digitalWrite(PinMOS1, HIGH); //Scroll Pump ON
//      scrollPumpBoolean = true;
//      digitalWrite(PinPowerSwitch2,HIGH); //Angle Valve Dock OPEN
//
//      boolean requestGranted =false;
//      int waitTimer =0;
//      while(waitTimer <=20 && requestGranted ==false){
//        ReadOut12bit = ADC12bit(PinPressureArray[0]); 
//        if(ReadOut12bit >= 2800 || ReadOut12bit <= 256){//Pressure in Dock >=1mbar
//          requestGranted = false;
//        } else{ //Pressure in Dock <=1mbar
//          requestGranted =true;
//        }
//        tft.setTextColor(colorFont[settingsMemory[3]]);
//        MeasureTemperature();
//        MeasurePressure();
//        displayTemperature1();
//        displayTemperature2();
//        displayPressure1();
//        displayPressure2();
//        tft.setTextColor(colorBackground[settingsMemory[2]]);
//        delay(900);
//        waitTimer ++;
//     }
//     // Open Angle valve towards Chamber
//     digitalWrite(PinMOS3, HIGH); //Turbo Pump ON
//     turboPumpBoolean = true;
//     delay(1000);
//     digitalWrite(PinOpenCollectorOutput3,HIGH); //Angle Valve Turbo OPEN
//     requestGranted =false;
//     waitTimer =0;
//     while(waitTimer <=20 && requestGranted ==false){
//        ReadOut12bit = ADC12bit(PinPressureArray[1]); 
//        if(ReadOut12bit >= 2800 || ReadOut12bit <= 256){//Pressure in Chamber >=1mbar
//          requestGranted = false;
//        } else{ //Pressure in Dock <=1mbar
//          requestGranted =true;
//        }
//        //tft.setTextColor(colorFont[settingsMemory[3]]);
//        MeasureTemperature();
//        MeasurePressure();
//        displayTemperature1();
//        displayTemperature2();
//        displayPressure1();
//        displayPressure2();
//        //tft.setTextColor(colorBackground[settingsMemory[2]]);
//        delay(900);
//        waitTimer ++;
//     }      
//    tft.fillRect(2, 190, 155, 15, colorBackground[settingsMemory[2]]);
//    tft.fillRect(2,220,155,15, colorBackground[settingsMemory[2]]);
//    tft.setTextSize(2);
//    tft.setCursor(40, 220);
//    tft.println("PUMPED");
//    tft.setTextColor(colorFont[settingsMemory[3]]);  
//  }  
//}

void menuModeCryoDock() {
  //Mode Menu: Switch Display Values between Vent and Pump
  digitalWrite(PinOpenCollectorOutput1,LOW); //Turn off LN2 Dewar in case it is open (Safety Feature)
  Beeper(20);
  if (cryoDockBoolean == true) {//Vent
      if (gateValveBoolean == true){
        tft.setTextColor(colorFont[settingsMemory[3]]);
        tft.setTextSize(1);
        tft.setCursor(30, 195);
        tft.println("Close Gate Valve");
        Beeper(2000);
        tft.fillRect(25, 195, 100, 20, colorBackground[settingsMemory[2]]);
        cryoDockBoolean = false;
        return;          
      }
      tft.fillRect(2, 164, 155, 73, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(25, 167);
      tft.println("CRYO DOCK");
      tft.setTextSize(2);
      tft.setCursor(55, 220);
      tft.println("WAIT");

      tft.fillRect(20, 190, 120, 15, colorFont[settingsMemory[3]]);
      tft.setTextSize(1);
      tft.setCursor(25, 195);
      tft.println("Angle Valve Closed");
      digitalWrite(PinOpenCollectorOutput3,LOW); //Angle Valve Turbo CLOSE
      delay(2000);
      turboPumpOFF();
      if(loadLockBoolean == false){ //If load lock is pumped
        scrollPumpON();
        digitalWrite(PinPowerSwitch2,HIGH); //Angle Valve Dock OPEN
        delay(1000);
      }else{
        digitalWrite(PinPowerSwitch2,LOW); //Angle Valve Dock CLOSE
        delay(1000);
        scrollPumpOFF();
      }
      //Update Temperature and Pressure Measurements
      MeasureTemperature();
      MeasurePressure();
      displayTemperature1();
      displayTemperature2();
      displayPressure1();
      displayPressure2();
      tft.fillRect(20, 190, 120, 15, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setCursor(30, 195);
      tft.setTextSize(1);
      tft.println("Turbo Spins Down");
      Beeper(20);
      tft.fillRect(20, 190, 120, 15, colorFont[settingsMemory[3]]);
      tft.fillRect(2,220,155,15, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(45, 220);
      tft.println("VENTED");
      tft.setTextColor(colorFont[settingsMemory[3]]);
  }
  else { //Pump
      tft.fillRect(2, 164, 155, 73, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setTextSize(2);
      tft.setCursor(25, 167);
      tft.println("CRYO DOCK"); 
      tft.setCursor(55, 220);
      tft.println("WAIT");
      
      tft.fillRect(20, 190, 120, 15, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setCursor(40, 195);
      tft.setTextSize(1);
      tft.println("Pumping Down");


      if(loadLockBoolean == false){ //if load lock is pumped
        digitalWrite(PinPowerSwitch2,LOW);//Angle Valve Load Lock CLOSE
        turboPumpON();
        scrollPumpON();
        delay(2000);
        digitalWrite(PinOpenCollectorOutput3, HIGH); // Angle Valve Turbo Pump OPEN
      }else{
        digitalWrite(PinPowerSwitch2, LOW); // Angle Valve Load Lock CLOSE
        delay(2000);
        turboPumpON();
        delay(1000); 
        scrollPumpON();
        digitalWrite(PinOpenCollectorOutput3, HIGH); // Angle Valve Turbo Pump OPEN       
      }
      int ReadOut12bit = ADC12bit(PinPressureArray[1]);
      if(ReadOut12bit >= 2800 || ReadOut12bit <=256){
        int countLoop =0;
        ReadOut12bit = ADC12bit(PinPressureArray[1]); // Read the pressure from the pressure gauge
        while (ReadOut12bit >= 2800 || ReadOut12bit <= 256){
              //Update Temperature and Pressure Measurements
              MeasureTemperature();
              MeasurePressure();
              displayTemperature1();
              displayTemperature2();
              displayPressure1();
              displayPressure2();
              delay(400);
              ReadOut12bit = ADC12bit(PinPressureArray[1]);
              ++ countLoop ;
              if(countLoop >=40){  //STOP the while loop if it has been vented for longer than 10 seconds
                ReadOut12bit = 1000;
              }
        }
    }
    // pressure in range
    if(loadLockBoolean == false){//if load lock is pumped            
      scrollPumpON();
      turboPumpON();
      digitalWrite(PinPowerSwitch2, HIGH); //Angle Valve Dock OPEN

    }else{
      turboPumpON();
      scrollPumpON();
      digitalWrite(PinPowerSwitch2, LOW); //Angle Valve Dock CLOSE
      delay(1000);        
    }
    
    tft.fillRect(20, 190, 120, 15, colorBackground[settingsMemory[2]]);
    tft.fillRect(40, 215, 100, 20, colorBackground[settingsMemory[2]]);
    tft.setTextSize(2);
    tft.setCursor(45, 220);
    tft.println("PUMPED");
  }  
}


void menuGateValve() {
  //Gate Valve Menu: Switch Display Values between Gate Valve Open and Close
  digitalWrite(PinOpenCollectorOutput1,LOW); //Turn off LN2 Dewar in case it is open (Safety Feature)
  Beeper(20);
  if (gateValveBoolean == true){ //OPEN the Gate Valve
      
      if(digitalRead(PinDigitalInIsolated6) == false){ //during startup, if the gate valve is already detected to be open, keep it open
        tft.fillRect(164, 84, 155, 73, colorFont[settingsMemory[3]]);
        tft.setTextColor(colorBackground[settingsMemory[2]]);
        tft.setTextSize(2);
        tft.setCursor(180, 87);
        tft.println("GATE VALVE");
        tft.fillRect(210, 135, 50, 20, colorFont[settingsMemory[3]]);
        tft.setTextColor(colorBackground[settingsMemory[2]]);
        tft.setTextSize(2);
        tft.setCursor(215, 140);
        tft.println("OPEN");
        digitalWrite(PinPowerSwitch4, false); //Close Gate Valve OFF
        digitalWrite(PinPowerSwitch2, false); //Close Angle Valve Dock
        delay(50);
        digitalWrite(PinPowerSwitch3, true); //Open Gate Valve ON
        delay(800);
        digitalWrite(PinPowerSwitch3, false); //Open Gate Valve OFF           
        tft.setTextColor(colorFont[settingsMemory[3]]);
        return;
      }

      if (loadLockBoolean == true){ // if load lock is "VENTED", do not open the Gate Valve
        tft.fillRect(185, 112, 155, 15, colorBackground[settingsMemory[2]]);
        tft.setTextSize(1);
        tft.setCursor(182, 118);
        tft.println("Load Lock not PUMPED");
        tft.setTextSize(2);
        Beeper(2000);
        gateValveBoolean = false;
        menuGateValve();
        return;          
      }

      if (cryoDockBoolean == true){ // if cryo dock is "VENTED", do not open the Gate Valve
        tft.fillRect(185, 112, 155, 15, colorBackground[settingsMemory[2]]);
        tft.setTextSize(1);
        tft.setCursor(182, 118);
        tft.println("Cryo Dock not PUMPED");
        tft.setTextSize(2);
        Beeper(2000);
        gateValveBoolean = false;
        menuGateValve();
        return;          
      }

      // Measure the Pressure from the Pressure Gauge and validate if the Pressure is in the agreeable range or not
      boolean pressureInRangeDock = true;
      //Check if pressure is okay
      float ReadOut12bit;
      ReadOut12bit = ADC12bit(PinPressureArray[0]); // Read the pressure from the pressure gauge
      if(ReadOut12bit >= 3350 || ReadOut12bit <= 256){ //<256 equals Gauge Error, >3350 is above Pressure Threshold for FIB/SEM
        pressureInRangeDock = false;
      }
      if (pressureInRangeDock == false){ // Only open the Gate Valve if the Pressure is in Range <256 equals Gauge Error, >3350 is above Pressure Threshold for FIB/SEM
        //tft.fillRect(2, 190, 155, 15, colorFont[settingsMemory[3]]);
        tft.fillRect(185, 113, 155, 15, colorBackground[settingsMemory[2]]);
        tft.setTextSize(1);
        tft.setCursor(205, 118);
        tft.println("P1 not OK");
        tft.setTextSize(2);
        Beeper(2000);
        gateValveBoolean = false;
        menuGateValve();
        return;               
      }
      boolean pressureInRangeChamber = true;

      ReadOut12bit = ADC12bit(PinPressureArray[1]); // Read the pressure from the pressure gauge
      if(ReadOut12bit >= 3350 || ReadOut12bit <= 256){ //<256 equals Gauge Error, >3350 is above Pressure Threshold for FIB/SEM
        pressureInRangeChamber = false;
      }
      if (pressureInRangeChamber == false){ // Only open the Gate Valve if the Pressure is in Range <256 equals Gauge Error, >3350 is above Pressure Threshold for FIB/SEM
        //tft.fillRect(2, 190, 155, 15, colorFont[settingsMemory[3]]);
        tft.fillRect(185, 113, 155, 15, colorBackground[settingsMemory[2]]);
        tft.setTextSize(1);
        tft.setCursor(200, 118);
        tft.println("P2 not OK");
        tft.setTextSize(2);
        Beeper(2000);
        gateValveBoolean = false;
        menuGateValve();
        return;               
      }
      
      tft.fillRect(164, 84, 155, 73, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(180, 87);
      tft.println("GATE VALVE");
      tft.setTextSize(2);
      tft.setCursor(215, 140);
      tft.println("WAIT");
      
      digitalWrite(PinPowerSwitch4, false); //Close Gate Valve OFF
      digitalWrite(PinPowerSwitch2, false); //Close Angle Valve
      delay(100);
      digitalWrite(PinPowerSwitch3, true); //Open Gate Valve ON
      delay(3000);
      digitalWrite(PinPowerSwitch3, false); //Open Gate Valve OFF
      
      tft.fillRect(210, 135, 50, 20, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(215, 140);
      tft.println("OPEN");
      tft.setTextColor(colorFont[settingsMemory[3]]);
  }
  else { //CLOSE the Gate Valve
      tft.fillRect(164, 84, 155, 73, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setTextSize(2);
      tft.setCursor(180, 87);
      tft.println("GATE VALVE");

      tft.fillRect(210, 135, 50, 20, colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(215, 140);
      tft.println("WAIT");
      ///
      int waitTimer =0;
      int IsButtonPressedTimer = 0;
      if(digitalRead(PinDigitalInIsolated6) == false){ //check if gate valve is open
        Beeper(1000);
        tft.fillRect(164, 84, 155, 73, colorBackground[settingsMemory[2]]);
        tft.setTextColor(colorFont[settingsMemory[3]]);
        tft.setTextSize(2);
        tft.setCursor(170, 87);
        tft.println("CLOSE VALVE?");
        tft.setTextSize(1);
        tft.setCursor(170, 115);
        tft.println("Transfer Arm Retracted?");
        tft.setCursor(170, 140);
        tft.println("Press Button To Confirm");

        while(waitTimer <=1000){ 
          IsButtonPressedTimer =0;
          while(digitalRead(PinButtonArray[4])==HIGH && IsButtonPressedTimer <=10){
            IsButtonPressedTimer++ ;
            Beeper(200);
          }
          if(IsButtonPressedTimer >=10){
              waitTimer = 1001;
          }
          //Beeper(10);
          delay(10);
          waitTimer ++;
        }
      if(IsButtonPressedTimer >=10){
        tft.fillRect(164, 84, 155, 73, colorBackground[settingsMemory[2]]);
        tft.setTextColor(colorFont[settingsMemory[3]]);
        tft.setTextSize(2);
        tft.setCursor(180, 87);
        tft.println("GATE VALVE");
        tft.fillRect(210, 135, 50, 20, colorBackground[settingsMemory[2]]);
        tft.setTextSize(2);
        tft.setCursor(215, 140);
        tft.println("WAIT");
        digitalWrite(PinPowerSwitch3, false); //Open Gate Valve OFF
        digitalWrite(PinPowerSwitch4, true); //Close Gate Valve ON
        delay(1000);
        digitalWrite(PinPowerSwitch4, false); //Close Gate Valve OFF
        if(loadLockBoolean == false){
          digitalWrite(PinPowerSwitch2, true); //Open Angle Valve Load Lock
        }else{
          digitalWrite(PinPowerSwitch2, false); //Open Angle Valve Load Lock
        }
        tft.fillRect(210, 135, 50, 20, colorBackground[settingsMemory[2]]);
        tft.setTextSize(2);
        tft.setCursor(200, 140);
        tft.println("CLOSED");
      } else{

        Beeper(50);
        delay(100);
        Beeper(50);
        delay(100);
        Beeper(50);

        tft.fillRect(165, 135, 155, 20, colorBackground[settingsMemory[2]]);
        tft.setCursor(213, 140);
        tft.println("Aborted");
        delay(1000);
        gateValveBoolean = true;
        menuGateValve();    
        
        }
      }else{
        digitalWrite(PinPowerSwitch3, false); //Open Gate Valve OFF
        digitalWrite(PinPowerSwitch4, true); //Close Gate Valve ON
        delay(500);
        digitalWrite(PinPowerSwitch4, false); //Close Gate Valve OFF
        if(loadLockBoolean == false){
          digitalWrite(PinPowerSwitch2, true); //Open Angle Valve Load Lock
        }
        else{
          digitalWrite(PinPowerSwitch2, false); //Open Angle Valve Load Lock
        }
        tft.fillRect(195, 135, 100, 20, colorBackground[settingsMemory[2]]);
        tft.setTextSize(2);
        tft.setCursor(200, 140);
        tft.println("CLOSED");
      }
  } 
}

void menuSettings() {
  digitalWrite(PinOpenCollectorOutput1,LOW); //Turn off LN2 Dewar in case it is open (Safety Feature)
  //Settings Menu: Display the Settings Submenu in the Main Menu
  if (settingsBoolean == false){
      tft.fillRect(164, 164, 155, 73, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setTextSize(2);
      tft.setCursor(190, 167);
      tft.println("SETTINGS");
  }  
  else{
      tft.fillRect(164, 164, 155, 73, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(190, 167);
      tft.println("SETTINGS");
      tft.setTextColor(colorFont[settingsMemory[3]]);
      currentMenu=3;
      Beeper(20);
      delay(500);
      settingsBoolean == false;
  }
}

void menuNextMenu() {
  digitalWrite(PinOpenCollectorOutput1,LOW); //Turn off LN2 Dewar in case it is open (Safety Feature)
  //Settings Menu: Display the Settings Submenu in the Main Menu
  if (nextMenuBoolean == false){
      tft.fillRect(164, 164, 155, 73, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setTextSize(2);
      tft.setCursor(190, 167);
      tft.println("NEXT PAGE");
      tft.fillTriangle(270,190,270,220,300,205,colorFont[settingsMemory[3]]);
      
  } else{
      tft.fillRect(164, 164, 155, 73, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(190, 167);
      tft.println("NEXT PAGE");
      tft.fillTriangle(270,190,270,220,300,205,colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      currentMenu=2;
      Beeper(20);
      delay(500);
      nextMenuBoolean == false;
  }
}

void menuPreviousMenu() {
  digitalWrite(PinOpenCollectorOutput1,LOW); //Turn off LN2 Dewar in case it is open (Safety Feature)
  //Settings Menu: Display the Settings Submenu in the Main Menu
  if (previousMenuBoolean == false){
      tft.fillRect(2, 164, 155, 73, colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      tft.setTextSize(2);
      tft.setCursor(25, 167);
      tft.println("PAST PAGE");
      tft.fillTriangle(50,190,50,220,20,205,colorFont[settingsMemory[3]]);
  } else{
      tft.fillRect(2, 164, 155, 73, colorFont[settingsMemory[3]]);
      tft.setTextColor(colorBackground[settingsMemory[2]]);
      tft.setTextSize(2);
      tft.setCursor(25, 167);
      tft.println("PAST PAGE");
      tft.fillTriangle(50,190,50,220,20,205,colorBackground[settingsMemory[2]]);
      tft.setTextColor(colorFont[settingsMemory[3]]);
      currentMenu=1;
      Beeper(20);
      delay(500);
      previousMenuBoolean == false;
  }
}

void settingsMenuSetTempUnit() {
  //Mode Menu: Switch Display Values between Vent and Pump
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setTextSize(2);
  tft.setCursor(20, 167);
  tft.println("Temp Unit"); 
  tft.setCursor(65, 210);
  tft.println(temperatureUnit[settingsMemory[1]]);
}

void settingsMenuSafetyMode() {
  tft.fillRect(2, 84, 155, 73, colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setTextSize(2);
  tft.setCursor(10, 87);
  tft.println("Safety Mode");
  tft.setCursor(60, 130);
  tft.println(safetyModeValuesDisplayed[settingsMemory[5]]);
}

void settingsMenuLN2Dewar() {
  tft.fillRect(2, 84, 155, 73, colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setTextSize(2);
  tft.setCursor(10, 87);
  tft.println("LN2 Filling");
  tft.setCursor(30, 130);
  tft.println(LN2DewarValuesDisplayed[settingsMemory[6]]);
}

void settingsMenuHeaterTimer() {
  tft.fillRect(2, 84, 155, 73, colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setTextSize(2);
  tft.setCursor(10, 87);
  tft.println("Heater Timer");
  tft.setCursor(50, 130);
  tft.println(heaterTimingValuesDisplayed[settingsMemory[4]]);
}

void settingsMenuPressUnit() {
  tft.fillRect(164, 164, 155, 73, colorBackground[settingsMemory[2]]);
  tft.setTextColor(colorFont[settingsMemory[3]]);
  tft.setTextSize(2);
  tft.setCursor(180, 167);
  tft.println("Press Unit");
  tft.setCursor(210, 210);
  tft.println(pressureUnit[settingsMemory[0]]);
}

void settingsMenuDisplayColor(){
    tft.fillRect(164, 84, 155, 73, colorBackground[settingsMemory[2]]);
    tft.setTextColor(colorFont[settingsMemory[3]]);
    tft.setTextSize(2);
    tft.setCursor(180, 87);
    tft.println("Disp Color");
    tft.setCursor(200, 130);
    tft.println(colorDisplayed[settingsMemory[2]]); 
}

void colorSettings(int number) {
  // This handles all Color Settings: 
  switch (number){
    case 0: 
    // Blue Background with White Font
      settingsMemory[2]=0;
      settingsMemory[3]=0;
      break;
    case 1:
    // Black Background with White Font
      settingsMemory[2]=1;
      settingsMemory[3]=0;
      break;
    case 2:
    // Red Background with White Font
      settingsMemory[2]=2;
      settingsMemory[3]=0;
      break;
    case 3:
    // Green Background with Black Font
      settingsMemory[2]=3;
      settingsMemory[3]=1;
      break;
    case 4:
    // Cyan Background with Black Font
      settingsMemory[2]=4;
      settingsMemory[3]=1;
      break;
    case 5:
    // Magenta Background with Black Font
      settingsMemory[2]=5;
      settingsMemory[3]=1;
      break;
    case 6:
    // Yellow Background with Black Font
      settingsMemory[2]=6;
      settingsMemory[3]=1;
      break;
    case 7:
    // White Background with Black Font
      settingsMemory[2]=7;
      settingsMemory[3]=1;
      break;
  }
  if(currentMenu ==1){
    displayMainMenu();
    displayTemperature1();
    displayTemperature2();
    displayPressure1();
    displayPressure2();
    //menuHeater();
    menuLN2Dewar();
    menuGateValve();
    menuNextMenu();
    menuModeCryoDock();
  }
  if(currentMenu ==2){
    displayMainMenu();
    displayTemperature1();
    displayTemperature2();
    displayPressure1();
    displayPressure2();
    //menuHeater();
    menuPreviousMenu();
    menuGateValve();
    menuSettings();
    menuLoadLock();
  }
//  if(currentMenu ==2){
//    tft.fillRect(162,82,158,76,colorFont[settingsMemory[2]]);
//    settingsMenuDisplayColor();
//  }
}

int ADC12bit (int channel){
  // Read internal ADC Signal
  unsigned int Value12bit, ValueADC, i=0, sum=0;
  while(i < 5){
    // original mean for 20 values
    ValueADC = analogRead(channel);
    sum = sum+ValueADC;
    delay(1); // original delay = 
    i++;
  }
  return (sum / 5); //return 12bit-value averaged
}

void MeasureLN2() {
  // Measure Temperature and Convert
  /* LN2 Calculation:
   * R = 500 OHM
   * I = 1mA to 20mA5
   * 
   * U = R*I = 500mV (empty) to 10V (full)
   * R_v = 3300 Ohm
   * R_pt = resistance of Pt100 (Temperature Dependent)
   * 
   * ADC Signal: 4096 = Full
   * ADC Signal: 200 = empty

   */
  
  int Value12bit;
  Value12bit = ADC12bit(PinLN2Array[0]);
  if (Value12bit <100){
    LN2Measured[0] = "--";
  } else{
      // Calculate Level: 
      // Spannung geht von 2V =0% bis 10V =100%
      // ADC Signal geth von ~800 bit bis ~4050 bit
      // f(x) =0.030769x - 24.6153, x =bit, y=percent
      float percent = (0.0307*float(Value12bit))-24.61; 
      
      if(percent<0){
        percent =0;
      } else if(percent>=100){
        percent =100;
      }
      Serial.println(Value12bit);
      Serial.println(percent);
      
      LN2Measured[0] = String(percent,0);
      Serial.println(LN2Measured[0]);
    }
}

void MeasureTemperature() {
  // Measure Temperature and Convert
  /* Temperature Calculation:
   * Gain= 1+(49.2/2.7)
   * R_v = 3300 Ohm
   * R_pt = resistance of Pt100 (Temperature Dependent)
   * R_tot = R_v + R_pt
   * ADC12bit Value = 1.241*Gain*R_v*R_pt/R_tot
   * -> This formula can be rewritten to R_pt = ADC12bit * 3300/(78720.8 -ADC12bit)
   * 
   * ADC12bit to R_pt: R_pt = ADC12bit * 3300/(78720.8-ADC12bit)
   * Once R_pt is calculated, one can calculate the Temperature by using Binomial Calculation: 
   * 
   * T [in degrees Celsius] = a + b*R_pt + c*R_pt^2 + d*R_pt^3
   * where a=-2.429E+02, b=2.279E+00, c=1.674E-03, d=-1.815E-06;
   */
  
  int Value12bit;
  
  for (int i = 0; i < 3 ; i++) {
    Value12bit = ADC12bit(PinTemperatureArray[i]);

    if (Value12bit <235){
      temperatureMeasured[i] = "--";
    } else{
      // Calculate Resistivity (OHM): 
      float F = 78720.8;  // Factor for Conversion
      float RPt = float(Value12bit) * 3300 / (F - float(Value12bit)); 
      RPt-=0.5;
      // Binomial Calculation (integer not necessary) in Celsius:
      float a=-2.429E+02, b=2.279E+00, c=1.674E-03, d=-1.815E-06;
      float Temp = (d * pow(RPt, 3) + c * RPt * RPt + b * RPt + a);
              
      if(settingsMemory[1]==0){
        //°Celsius
        Temp = Temp;
      } else if(settingsMemory[1]==1){
        //Kelvin
        Temp += 273.15;
      } else {
        //°Fahrenheit
        Temp = Temp*1.8+32;
      }
      temperatureMeasured[i] = String(Temp,1);
    }
  }
}

void MeasurePressure() {
  // Measure Pressure and Convert
  /* Pressure Calculation for PfeifferTPR280:
  * pressure = 10^(U-c) <-> U = c + log10(p)
  * formula valid in the range 5x10^-4mbar to 1000mbar
  * 
  *  U      p       c
  * [V]   [mbar]    5.5
  * [V]   [ubar]    2.5
  * [V]   [Torr]    5.625
  * [V]   [mTorr]   2.625
  * [V]   [micron]  2.625
  * [V]   [Pa]      3.5
  * [V]   [kPa]     6.5
  * 
  Voltage Divider to reduce voltage of Pirani (+2.2...+8.5) to (0...3.3V)
  R1 = 4.7 kOhm
  R2 = 7.5 kOhm
  Gain = 4.7/(4.7+7.5) =~ 0.28

  V = (Value12bit/4095)*3.3V* 1/Gain = Value12bit/4096*8.5
  
  Pressure Calculation for Pfeiffer PKR261:
  pressure = 10^(1.667U-d) <-> U = d + log10(p)

  *  U      p       d
  * [V]   [mbar]    11.33
  * [V]   [Torr]    11.46
  * [V]   [Pa]      9.33  
  *  
  */

  float Value12bit;
  float Press;
  float voltageConversion;
  for (int i = 0; i < 2 ; i++) {
    int counter =0;
    Value12bit = ADC12bit(PinPressureArray[i]);
    //Value12bit = random(1,4096);
    if (Value12bit <= 1024){
      if (Value12bit <= 256){                  // no pressure sensor connected
        pressureMeasured[i] = "--";
      }
      else{ // Pressure under Range)
        if(settingsMemory[0]==0){
          if(i ==0){
            pressureMeasured[i] = "<5E-4";
          }else{
            pressureMeasured[i] = "<5E-9";
          }
        }else if(settingsMemory[0]==1){
          if(i==0){
            pressureMeasured[i] = "<3E-4";
          }else{
            pressureMeasured[i] = "<3E-9";
          }
        }else{
          if(i==0){
           pressureMeasured[i] = "<4E-2"; 
          }else{
            pressureMeasured[i] = "<4E-7";
          }
        }
      }
    }
    else{
      if(i ==0){
        voltageConversion = Value12bit/4096*8.5;
      }else{
        voltageConversion = Value12bit/4096*10;
        if(voltageConversion>8.6){
          voltageConversion =8.6;
        }
      }
      if(settingsMemory[0]==0){
        //mbar
        if(i==0){
          float exponential = voltageConversion - 5.5 + 1E-10;
          Press = powf(10.0, exponential);          
        }else{
          float exponential = 1.667*voltageConversion - 11.33 + 1E-10;
          Press = powf(10.0, exponential);   
        }
      } else if(settingsMemory[0]==1){
        //Torr
        if(i==0){
          float exponential = voltageConversion - 5.625 + 1E-10;
          Press = powf(10.0, exponential);        
        }else{
          float exponential = 1.667*voltageConversion - 11.46 + 1E-10;
          Press = powf(10.0, exponential);             
        }
      } else{
        //Pascal
        if(i==0){
          float exponential = voltageConversion - 3.5 + 1E-10;
          Press = powf(10.0, exponential);          
        }else{
          float exponential = 1.667*voltageConversion - 9.33 + 1E-10;
          Press = powf(10.0, exponential);            
        }
      }
      float value = Press;
      if (value >= 1000){
        while (Press >=10){
          Press = Press/10;
          counter +=1;
        }
        if(Press >=9.91){
          //Otherwise, it will display 10E4 instead of 1.0E5
          Press = 1;
          counter = counter +1;
        }
        pressureMeasured[i] = String(Press,1)+ "E" +String(counter); 
      }else if (value <1000 && value >=100){
        pressureMeasured[i] = String(Press,0);
      }else if (value <100 && value >=1){
        pressureMeasured[i] = String(Press,1);
      }else{
        while (Press <1){
          Press = Press*10;
          counter = counter -1;
        }
        if(Press >9.91){
          //Otherwise, it will display 10E4 instead of 1.0E5
          Press = 1;
          counter = counter +1;
        }
        pressureMeasured[i] = String(Press,1)+ "E" +String(counter);
      }   
    }
  }
}

void menuChanged(){
  if(currentMenu != previousMenu){
    switch (currentMenu){
      case 0: // Welcome Screen
        welcomeScreen();
        Serial.println(currentMenu);
        break;
      case 1: // Main Menu Screen 1
//        if(cryoDockBoolean==false){
//          turboPumpBoolean =true;
//        }
        displayMainMenu();
        //menuHeater();
        menuLN2Dewar();
        menuGateValve();
        menuNextMenu();
        menuModeCryoDock();
        saveToSDCard();
        Serial.println(currentMenu);
        break;
      case 2: // Main Menu Screen 2
//        if(cryoDockBoolean==false){
//          turboPumpBoolean =true;
//        }
        displayMainMenu();
        //menuHeater();
        menuGateValve();
        menuSettings();
        menuPreviousMenu();
        menuLoadLock();
        saveToSDCard();
        Serial.println(currentMenu);
        break;
      case 3: // Settings Screen 1
        displaySettingsMenu(currentMenu);
        //settingsMenuHeaterTimer();
        //settingsMenuSafetyMode();
        settingsMenuLN2Dewar();
        settingsMenuSetTempUnit();
        settingsMenuDisplayColor();
        settingsMenuPressUnit();
        Serial.println(currentMenu);
        break;
    }    
  }
  previousMenu = currentMenu;
}

void buttonPressedSimpleClick() {
  // read the pushbutton input pin and directly switches the state upon pressing a button. This function was replaced by the function buttonPressed
  for (int i = 0; i < 6 ; i++) {
    presentButtonStates[i] = digitalRead(PinButtonArray[i]);
    if (presentButtonStates[i] != lastButtonStates[i]) {
      // if the state has changed, increment the counter
      if (presentButtonStates[i] == HIGH){
          if(i==0){
            switch (currentMenu){
              case 3:
                colorSettings(switchColor);
                currentMenu = 1;
                break;
            }
            break;
          } else if(i==1){
            switch (currentMenu){
              case 3:
                ++ settingsMemory[6]; //LN2Filling
                if(settingsMemory[6]>2){
                  settingsMemory[6] =0;
                }
                tft.fillRect(25,125,120,30,colorBackground[settingsMemory[2]]);
                tft.setTextColor(colorFont[settingsMemory[3]]);
                tft.setCursor(30, 130);
                //tft.println(heaterTimingValuesDisplayed[settingsMemory[4]]);
                tft.println(LN2DewarValuesDisplayed[settingsMemory[6]]);
                break;
            }
          } else if(i==2){
            switch (currentMenu){
              case 3:
                ++ settingsMemory[1]; //Temperature Unit
                if(settingsMemory[1]>2){
                  settingsMemory[1] =0;
                }
                tft.fillRect(60,205,80,30,colorBackground[settingsMemory[2]]);
                tft.setTextColor(colorFont[settingsMemory[3]]);
                tft.setCursor(65, 210);
                tft.println(temperatureUnit[settingsMemory[1]]);
                break;
            }
          } else if(i==3){
            switch (currentMenu){
              case 3:
                // ++ currentMenu; Uncomment this part if you want to add another Settings Menu
                break;
            }
          } else if(i==4){
            switch (currentMenu){
              case 3: // Settings Menu 1
                ++ switchColor;
                if(switchColor>7){
                  switchColor =0;
                }
                switch (switchColor){
                  case 0: 
                  // Blue Background with White Font
                    tft.fillRect(164, 84, 155, 73, colorBackground[0]);
                    tft.setTextColor(colorFont[0]);
                    tft.setTextSize(2);
                    tft.setCursor(180, 87);
                    tft.println("Disp Color");
                    tft.fillRect(195,125,100,30,colorBackground[0]);
                    tft.setTextColor(colorFont[0]);
                    tft.setCursor(200, 130);
                    tft.println(colorDisplayed[0]);
                    break;

                  case 1:
                  // Black Background with White Font
                    tft.fillRect(164, 84, 155, 73, colorBackground[1]);
                    tft.setTextColor(colorFont[0]);
                    tft.setTextSize(2);
                    tft.setCursor(180, 87);
                    tft.println("Disp Color");
                    tft.fillRect(195,125,100,30,colorBackground[1]);
                    tft.setTextColor(colorFont[0]);
                    tft.setCursor(200, 130);
                    tft.println(colorDisplayed[1]);
                    break;
                  case 2:
                  // Red Background with White Font
                    tft.fillRect(164, 84, 155, 73, colorBackground[2]);
                    tft.setTextColor(colorFont[0]);
                    tft.setTextSize(2);
                    tft.setCursor(180, 87);
                    tft.println("Disp Color");
                    tft.fillRect(195,125,100,30,colorBackground[2]);
                    tft.setTextColor(colorFont[0]);
                    tft.setCursor(200, 130);
                    tft.println(colorDisplayed[2]);
                    break;
                  case 3:
                  // Green Background with Black Font
                    tft.fillRect(164, 84, 155, 73, colorBackground[3]);
                    tft.setTextColor(colorFont[1]);
                    tft.setTextSize(2);
                    tft.setCursor(180, 87);
                    tft.println("Disp Color");
                    tft.fillRect(195,125,100,30,colorBackground[3]);
                    tft.setTextColor(colorFont[1]);
                    tft.setCursor(200, 130);
                    tft.println(colorDisplayed[3]);
                    break;
                  case 4:
                  // Cyan Background with Black Font
                    tft.fillRect(164, 84, 155, 73, colorBackground[4]);
                    tft.setTextColor(colorFont[1]);
                    tft.setTextSize(2);
                    tft.setCursor(180, 87);
                    tft.println("Disp Color");
                    tft.fillRect(195,125,100,30,colorBackground[4]);
                    tft.setTextColor(colorFont[1]);
                    tft.setCursor(200, 130);
                    tft.println(colorDisplayed[4]);
                    break;
                  case 5:
                  // Magenta Background with Black Font
                    tft.fillRect(164, 84, 155, 73, colorBackground[5]);
                    tft.setTextColor(colorFont[1]);
                    tft.setTextSize(2);
                    tft.setCursor(180, 87);
                    tft.println("Disp Color");
                    tft.fillRect(195,125,100,30,colorBackground[5]);
                    tft.setTextColor(colorFont[1]);
                    tft.setCursor(200, 130);
                    tft.println(colorDisplayed[5]);
                    break;
                  case 6:
                  // Yellow Background with Black Font
                    tft.fillRect(164, 84, 155, 73, colorBackground[6]);
                    tft.setTextColor(colorFont[1]);
                    tft.setTextSize(2);
                    tft.setCursor(180, 87);
                    tft.println("Disp Color");
                    tft.fillRect(195,125,100,30,colorBackground[6]);
                    tft.setTextColor(colorFont[1]);
                    tft.setCursor(200, 130);
                    tft.println(colorDisplayed[6]);
                    break;
                  case 7:
                  // White Background with Black Font
                    tft.fillRect(164, 84, 155, 73, colorBackground[7]);
                    tft.setTextColor(colorFont[1]);
                    tft.setTextSize(2);
                    tft.setCursor(180, 87);
                    tft.println("Disp Color");
                    tft.fillRect(195,125,100,30,colorBackground[7]);
                    tft.setTextColor(colorFont[1]);
                    tft.setCursor(200, 130);
                    tft.println(colorDisplayed[7]);
                    break;
                }
              
//                ++ settingsMemory[2]; //Display Color
//                if(settingsMemory[2]>7){
//                  settingsMemory[2] =0;
//                }
//                colorSettings(settingsMemory[2]);
//                tft.fillRect(195,125,100,30,colorBackground[settingsMemory[2]]);
//                tft.setTextColor(colorFont[settingsMemory[3]]);
//                tft.setCursor(200, 130);
//                tft.println(colorDisplayed[settingsMemory[2]]);
//                break;
            }
           } else if(i==5){
            switch (currentMenu){
              case 3:
                ++ settingsMemory[0]; //Pressure Unit
                if(settingsMemory[0]>2){
                  settingsMemory[0] =0;
                }
                tft.fillRect(205,205,100,30,colorBackground[settingsMemory[2]]);
                tft.setTextColor(colorFont[settingsMemory[3]]);
                tft.setCursor(210, 210);
                tft.println(pressureUnit[settingsMemory[0]]);
                break;
            }
           }
      }else {
      // if the current state is LOW then the button went from on to off:
      }
      // Delay a little bit to avoid bouncing
      delay(10);
      }
  }
  for (int i = 0; i < 6 ; i++) {
    lastButtonStates[i] = presentButtonStates[i];
  }  
}

void buttonPressedLong() {
  // read the pushbutton input pin:
  for (int i = 0; i < 6 ; i++) {
    presentButtonStates[i] = digitalRead(PinButtonArray[i]);
    if (presentButtonStates[i] != lastButtonStates[i]) {
      // if the state has changed, increment the counter
      if (presentButtonStates[i] == HIGH){
        buttonPressTimer =0;
        while(digitalRead(PinButtonArray[i])==HIGH && buttonPressTimer <=5){
          delay(100);
          if(buttonPressTimer == 5){
          if(i==1){
            switch (currentMenu){
              case 1:
                 if (modeBooleanLN2Dewar == true){
                  modeBooleanLN2Dewar = false;
                } else{
                  modeBooleanLN2Dewar = true;            
                }
                menuLN2Dewar();
                break;
              case 2:
                 if (loadLockBoolean == true){
                  Serial.println("here1");
                  loadLockBoolean = false;
                } else{
                  loadLockBoolean = true; 
                  Serial.println("here2");           
                }
                menuLoadLock();
                break;
            }          
          } else if(i==2){
            switch (currentMenu){
              case 1:
                if (cryoDockBoolean == true){
                  cryoDockBoolean = false;
                } else{
                  cryoDockBoolean = true;              
                }
                menuModeCryoDock();
                break;
              case 2:
                if (previousMenuBoolean == true){
                  previousMenuBoolean = false;
                } else{
                  previousMenuBoolean = true;              
                }
                menuPreviousMenu();
                break;
            }
          } 
          else if(i==4){
            switch (currentMenu){
              case 1:
                if (gateValveBoolean == true){
                   gateValveBoolean = false;
                } else{
                  gateValveBoolean = true;              
                }
                menuGateValve();
                break;
              case 2:
                if (gateValveBoolean == true){
                   gateValveBoolean = false;
                } else{
                  gateValveBoolean = true;              
                }
                menuGateValve();
                break;
            }
          } else if(i==5){
            switch (currentMenu){
              case 1:
                if (nextMenuBoolean == true){
                  nextMenuBoolean = false;
                } else{
                  nextMenuBoolean = true;              
                }
                menuNextMenu();
                break;
              case 2:
                if (settingsBoolean == true){
                  settingsBoolean = false;
                } else{
                  settingsBoolean = true;              
                }
                // Only access settings menu if the Gate Valve is closed -> this is important, otherwise there might be issues when changing between Saftety Modes
                if (gateValveBoolean == true && settingsBoolean==true){
                  tft.setTextColor(colorFont[settingsMemory[3]]);
                  tft.setTextSize(1);
                  tft.setCursor(193, 195);
                  tft.println("Close Gate Valve");
                  Beeper(2000);
                  //tft.fillRect(25, 195, 100, 20, colorBackground[settingsMemory[2]]);
                  settingsBoolean = false;
                }
                switchColor = settingsMemory[2];
                menuSettings();
                break;
            }
          }            
          buttonPressTimer +=1;
          }else{
            buttonPressTimer +=1;
          }
      }
      }
      // Delay a little bit to avoid bouncing

      }
      buttonPressTimer =0;
  }
  for (int i = 0; i < 6 ; i++) {
    lastButtonStates[i] = presentButtonStates[i];
  }  
}

void Beeper(int howlong){
   digitalWrite(PinBeep,1);
   delay(howlong);
   digitalWrite(PinBeep,0);
  }
