
/****************************************************************************
ECU CAN-Bus Reader and Logger

Toni Klopfenstein @ SparkFun Electronics
September 2015
https://github.com/sparkfun/CAN-Bus_Shield

This example sketch works with the CAN-Bus shield from SparkFun Electronics.

It enables reading of the MCP2515 CAN controller and MCP2551 CAN-Bus driver.
This sketch also enables logging of GPS data, and output to a serial-enabled LCD screen.
All data is logged to the uSD card. 

Resources:
Additional libraries to install for functionality of sketch.
-SD library by William Greiman. https://github.com/greiman/SdFat 
 
Development environment specifics:
Developed for Arduino 1.65

Based off of original example ecu_reader_logger by:
Sukkin Pang
SK Pang Electronics www.skpang.co.uk

This code is beerware; if you see me (or any other SparkFun employee) at the local, 
and you've found our code helpful, please buy us a round!

For the official license, please check out the license file included with the library.

Distributed as-is; no warranty is given.
*************************************************************************/

//Include necessary libraries for compilation
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <Canbus.h>
#include <TinyGPS.h>

//Initialize uSD pins
const int chipSelect = 9;

//Initialize lcd pins
SoftwareSerial lcd(3, 6);

//Initialize GPS pins
SoftwareSerial uart_gps(4,5);

// Define Joystick connection pins 
#define UP     A1
#define DOWN   A3
#define LEFT   A2
#define RIGHT  A5
#define CLICK  A4

//Define LED pins
#define LED2 8
#define LED3 7

//Define baud rates. GPS should be slower than serial to ensure valid sentences coming through
#define GPSRATE 4800
#define LCD_Rate 115200

//Create instance of TinyGPS
TinyGPS gps;

//Declare porototype for TinyGPS library functions
void getgps(TinyGPS &gps);

//Declare SD File
File dataFile;

//Declare CAN variables for communication
int CanOutput;
char buffer[512];  //Data will be temporarily stored to this buffer before being written to the file


//Define LCD Positions
#define COMMAND 0xFE
#define CLEAR   0x01
#define LINE1  0x80
#define LINE2  0xC0


//********************************Setup Loop*********************************//
void setup() {
  //Initialize Serial communication for debugging
  Serial.begin(9600);
  Serial.println("ECU Demo");
  
  //Begin LCD serial communication
  lcd.begin(9600);
  
  //Begin GPS communcation
  uart_gps.begin(GPSRATE);
  
  //Initialize pins as necessary
  pinMode(chipSelect, OUTPUT);
  pinMode(CLICK,INPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  //Pull analog pins high to enable reading of joystick movements
  digitalWrite(CLICK, HIGH);
  
  //Write LED pins low to turn them off by default
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  
  //Check if uSD card initialized
  if (!SD.begin(chipSelect)) {
    Serial.println("uSD card failed to initialize, or is not present");
    return;
  }
  else{
      Serial.println("uSD card initialized.");
  } 

  //Initialize CAN Controller 
  if(Canbus.init(CANSPEED_500))  /* Initialize MCP2515 CAN controller at the specified speed */
  {
    lcd.print("CAN Init ok");
  } 
  else
  {
    lcd.print("Can't init CAN");
    return;
  } 
  
  //Print menu to LCD screen
  clear_lcd();
  lcd.print("Click to begin");
  lcd.write(COMMAND);
  lcd.write(LINE2);
  lcd.print("Logging Data");
 
  while(CLICK ==HIGH)
  {
    digitalRead(CLICK); //Wait for user to click joystick to begin logging
  }

  delay(1000); 

}

//********************************Main Loop*********************************//
void loop(){
  
  dataFile = SD.open("ECU_data_log.txt", FILE_WRITE);
  
  //If data file exists, write data to it
  if (dataFile){
    
    clear_lcd();
    lcd.print("Logging data");
    clear_lcd();
    lcd.print("Click joystick");
    lcd.write(COMMAND);
    lcd.write(LINE2);
    lcd.print("to stop logging");
    
    if(uart_gps.available())     // While there is data on the RX pin...
       {
         digitalWrite(LED2, HIGH); //Signal on D8 that GPS data received.
         int c = uart_gps.read();    // load the data into a variable...
         if(gps.encode(c))      // if there is a new valid sentence...
         {
           getgps(gps);         // then grab the data.
         }
         digitalWrite(LED2, LOW); //Turn off D8 LED. 
       }
     
    get_CAN(); //grab CAN data
     
    
    dataFile.println();
    dataFile.close();   //Close data logging file
  }
  //If file cannot be opened,write error to LCD screen
  else{
    clear_lcd();
    lcd.print("Error opening");
    lcd.write(COMMAND);
    lcd.write(LINE2);
    lcd.print("ECU_datalog.txt");
  }

  digitalRead(CLICK);
  if (CLICK == HIGH)
  {
    return; //Stop logging if joystick is clicked
  
  }
  
}
//********************************CAN Bus Functions*********************************//

void get_CAN(void)
{
  digitalWrite(LED3, HIGH); //Turn on LED to indicate CAN Bus traffic
  
  CanOutput = Canbus.ecu_req(ENGINE_RPM,buffer); //Request engine RPM
  dataFile.print("Engine RPM: "); 
  dataFile.println(CanOutput);
  
  CanOutput = Canbus.ecu_req(VEHICLE_SPEED,buffer); //Request Vehicle speed
  dataFile.print("Vehicle speed: "); 
  dataFile.println(CanOutput);
  
  CanOutput = Canbus.ecu_req(ENGINE_COOLANT_TEMP,buffer); //Request engine coolant temp
  dataFile.print("Coolant temp: "); 
  dataFile.println(CanOutput);
  
  CanOutput = Canbus.ecu_req(THROTTLE, buffer); //Request throttle
  dataFile.print("Throttle: "); 
  dataFile.println(CanOutput);

  digitalWrite(LED3, LOW); //Turn off LED3
  delay(100);
  
}
//********************************LCD Functions*********************************//
void clear_lcd(void)
{
  lcd.write(COMMAND);
  lcd.write(CLEAR);
} 

//********************************GPS Functions*********************************//
void getgps(TinyGPS &gps)
{
  
  // Define the variables that will be used
  float latitude, longitude;
  // Then call this function
  gps.f_get_position(&latitude, &longitude);
  // You can now print variables latitude and longitude
  dataFile.print("Lat/Long: "); 
  dataFile.print(latitude,5); 
  dataFile.print(", "); 
  dataFile.println(longitude,5);
  
  // Same goes for date and time
  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
  // Print data and time
  dataFile.print("Date: "); dataFile.print(month, DEC); dataFile.print("/"); 
  dataFile.print(day, DEC); dataFile.print("/"); dataFile.print(year);
  dataFile.print("  Time: "); dataFile.print(hour, DEC); dataFile.print(":"); 
  dataFile.print(minute, DEC); dataFile.print(":"); dataFile.print(second, DEC); 
  dataFile.print("."); dataFile.println(hundredths, DEC);
  
  // Here you can print the altitude and course values directly since 
  // there is only one value for the function
  dataFile.print("Altitude(m): "); dataFile.println(gps.f_altitude());  
  // Same goes for course
  dataFile.print("Course(deg): "); dataFile.println(gps.f_course()); 
  // And same goes for speed
  dataFile.print("GPS Speed(kmph): "); dataFile.println(gps.f_speed_kmph());
  dataFile.println();
}
