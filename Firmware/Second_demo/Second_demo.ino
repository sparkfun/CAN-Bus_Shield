
#include <Canbus.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include<TinyGPS.h>

const int chipSelect = 9;  

//Create instance of serial LCD connection
SoftwareSerial lcdSerial(3,6); // pin 6 = TX, pin 3 = RX (unused)

#define COMMAND 254
#define CLEAR   0
#define LINE1  128
#define LINE2  192

// Define Joystick connection pin 
#define CLICK  A4

//Initialize GPS pins
SoftwareSerial uart_gps(4,5);

//Create instance of TinyGPS
TinyGPS gps;

//Set up buffer for CAN-Bus
char buffer[512];

//********************************Setup Loop*********************************//
void setup()
{
  //Initialize pins as necessary
  pinMode(CLICK,INPUT);
  
  //Pull analog pins high to enable reading of joystick movements
  digitalWrite(CLICK, HIGH);
  
  lcdSerial.begin(9600);
  Serial.begin(9600);
  uart_gps.begin(4800);
  
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.print("Initializing SD card...");
  pinMode(chipSelect, OUTPUT);

  if (!SD.begin(chipSelect)) 
  {
    lcdSerial.println("Card failed, or not present");
    return;
  }
  Serial.println("Card Initialized.");
  lcdSerial.println("uSD Card Ready.");
  
  if(Canbus.init(CANSPEED_500))
  {
    write_line2();
    lcdSerial.println("CAN init OK");
  }
  else
  {
    write_line2();
    lcdSerial.println("No CAN func.");  
  }
  
  delay(1000);
  
}

//********************************Main Loop*********************************//
void loop()
{
  String dataString = "";
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  if (dataFile)   
  {  
    clear_lcd();
    lcdSerial.print("Logging data.");
    write_line2();
    lcdSerial.print("CLICK to stop.");
    
    int timeStamp = millis();
    //write to uSD card
    dataFile.print(timeStamp);
    dataFile.print(" ms");
    dataFile.print(", ");

  for (unsigned long start = millis(); millis() - start < 500;)
  {
    while (uart_gps.available())
    {
      char c = uart_gps.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    Serial.print("LAT=");
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    dataFile.print("Latitude= ");
    dataFile.print(flat,6);
    dataFile.print(", ");
    Serial.print(" LON=");
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    dataFile.print("Longitude= ");
    dataFile.print(flon, 6);
    dataFile.print(", ");
  }
  if (chars == 0)
   {
    Serial.println("No GPS characters received"); 
    dataFile.println("No gps data received.");
   }
  
//Request CAN BUS information
  if(Canbus.ecu_req(ENGINE_RPM, buffer) ==1)
  {
    dataFile.print("Engine RPM: ");
    dataFile.print(buffer);
    dataFile.print(", ");
  }
  if(Canbus.ecu_req(VEHICLE_SPEED,buffer)==1)
  {
    dataFile.print("Vehicle speed: ");
    dataFile.print(buffer);
    dataFile.print(", ");
  }
   if(Canbus.ecu_req(ENGINE_COOLANT_TEMP,buffer)==1)
  {
    dataFile.print("Coolant temp: ");
    dataFile.print(buffer);
    dataFile.print(", ");
  }
   if(Canbus.ecu_req(THROTTLE,buffer)==1)
  {
    dataFile.print("Throttle: ");
    dataFile.print(buffer);
    dataFile.print(", ");
  }
   
    dataFile.println(); //create a new row to read data more clearly
    dataFile.close();   //close file
    Serial.println();   //print to the serial port too:

  }  
  
  else
  {
    clear_lcd();
    lcdSerial.print("Can not open");
    write_line2();
    lcdSerial.print("datalog.txt");
    Serial.println("Could not open SD file.");
    

  } 
  
  if(digitalRead(CLICK)==0)
 {
   Serial.println("CLICK- stop recording");
   clear_lcd();
   lcdSerial.print("RECORDING");
   write_line2();
   lcdSerial.print(" PAUSED.");
   while(1);
 }
 
}

//********************************LCD Functions*********************************//
void write_line2()
{
    lcdSerial.write(COMMAND);
    lcdSerial.write(LINE2); 
}

void clear_lcd(void)
{
  lcdSerial.write(COMMAND);
  lcdSerial.write((byte)CLEAR);
  lcdSerial.write(COMMAND);
  lcdSerial.write(LINE1);
}

