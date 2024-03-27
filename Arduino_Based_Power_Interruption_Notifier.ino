//Voltage Monitoring System

// It sends Message to Auth. num. if Amp is equal t0 zero (No power brownout) 
// Materials
/* Arduino Uno
   1X DF Voltage Sensor
   1X LCD module with I2C
   2X Push Button
   1X SIM900L Module
   
*/
//Connections
/*  AC Current Sensor ----------------------------- Arduino
        A          -----------------------------  A2
        +          -----------------------------  5V
        -          -----------------------------  GND
  LCD             
        GND        -----------------------------  GND
        VCC        -----------------------------  5v
        SDA        -----------------------------  A4
        SCL        -----------------------------  A5 
SIM900A
//https://wiki.dfrobot.com/Gravity_Analog_AC_Current_Sensor__SKU_SEN0211_
*/
#include <LiquidCrystal_I2C.h> // library for displa(LCD)
#include <SoftwareSerial.h> // for GSM serial communication
#include "SIM900.h" // GSM 900 library from http://www.gsmlib.org/
#include "sms.h" // SMS library

#define ACTectionRange 20    //set Non-invasive AC Current Sensor tection range (5A,10A,20A)
//#define _GSM_TXPIN_ 2
//#define _GSM_RXPIN_ 3 
#define VREF 5.0

LiquidCrystal_I2C lcd(0x27,16,2);//16x2 display
SMSGSM sms;
const int ACPin = A2;         //set arduino signal read pin
boolean started=false; // set to true if GSM communication is okay
char smsbuffer[160]; // storage for SMS message 
byte Position; //address for sim card messages
char number[20]; //who texted
char authNumber[20] = "09*********"; // mobile number to send SMS to

bool has_run_off = true; 
bool has_run_on = true;
bool alert = true;
String location = "Staff House";
int reset = 4;

// VREF: Analog reference
// For Arduino UNO, Leonardo and mega2560, etc. change VREF to 5
// For Arduino Zero, Due, MKR Family, ESP32, etc. 3V3 controllers, change VREF to 3.3

float readACCurrentValue()
{
  float ACCurrtntValue = 0;
  float peakVoltage = 0;  
  float voltageVirtualValue = 0;  //Vrms
  for (int i = 0; i < 5; i++)
  {
    peakVoltage += analogRead(ACPin);   //read peak voltage
    delay(1);
  }
  peakVoltage = peakVoltage / 5;   
  voltageVirtualValue = peakVoltage * 0.707;    //change the peak voltage to the Virtual Value of voltage
  /*The circuit is amplified by 2 times, so it is divided by 2.*/
  voltageVirtualValue = (voltageVirtualValue / 1024 * VREF ) / 2;  
  ACCurrtntValue = voltageVirtualValue * ACTectionRange;
  return ACCurrtntValue;
}
 byte p20[8] = {
   B10000,   
   B10000,
   B10000,
   B10000,
   B10000,
   B10000,
   B10000,
   B10000,
  };
byte p40[8] = {
   B11000,
   B11000,
   B11000,
   B11000,
   B11000,
   B11000,
   B11000,
   B11000,
   };
 byte p60[8] = {
   B11100,
   B11100,
   B11100,
   B11100,
   B11100,
   B11100,
   B11100,
   B11100,
   };
 byte p80[8] = {
   B11110,
   B11110,
   B11110,
   B11110,
   B11110,
   B11110,
   B11110,
   B11110,
   };
 byte p100[8] = {
   B11111,
   B11111,
   B11111,
   B11111,
   B11111,
   B11111,
   B11111,
   B11111,
   };

void GSMsetup()
{
  //Serial.println(F("Initializing GSM"));
  lcd.print("Initializing GSM");
  lcd.setCursor(0,1);
  lcd.print("                "); /*16 empty spaces */
  for(int i=0;i<16;++i)
   {
     for (int j=0;j<5;j++)
      {
      lcd.setCursor(i,1);
      lcd.write(j);
      delay(100);          
      }       
   }   
   lcd.clear();
  if (gsm.begin(2400)) // communicate with GSM module at 2400
  {
    Serial.println(F("GSM is ready"));
    lcd.setCursor(1,0);
    lcd.println("GSM is ready");
    delay(1000);
    lcd.clear();
    started=true;  // established communication with GSM module
    sendMessage(authNumber,"System is ready!, GSM connected send help for list of commands"); // send SMS that system is ready to accept commands
    lcd.setCursor(1,0);
    Serial.println("System is up!");
    lcd.println("System is up!");
  }
  else 
  Serial.println(F("GSM is not ready"));
  lcd.println("GSM not ready");
  delay(1000);
}
void sendMessage (char phone[20], char message[160])
{
  if(started){ // check if we can communicate with the GSM module
    digitalWrite(LED_BUILTIN, HIGH); // turn on status LED before sending SMS
    if (sms.SendSMS(phone, message)){
      Serial.print(F("\nSMS sent to "));
      Serial.println(phone);
    }
    digitalWrite(LED_BUILTIN, LOW); // turn off LED after sending SMS
  }
}
void receiveMessage()
{  
  if(started){// check if we can communicate with the GSM module
    Position = sms.IsSMSPresent(SMS_UNREAD); //check location of unread sms
    if (Position !=0){ // check if there is an unread SMS message
      sms.GetSMS(Position, number, smsbuffer,160); // get number and SMS message
      Serial.print(F("Received SMS from ")); 
      Serial.println(number); // sender number
      Serial.println(smsbuffer); // sender message
      sms.DeleteSMS(Position); // delete read SMS message

      Serial.println(F("Sending to SMS processor")); // sender message
      processSMScommand(); // check if valid SMS command and process
    }
  }
}
void processSMScommand()
{
  // Blank SMS are sometimes received -- we will ignore them
  Serial.print(F("Processing SMS from ")); 
  Serial.println(number); // send number
  Serial.println(smsbuffer); // sender message
  digitalWrite(LED_BUILTIN, HIGH); // turn on LED before processing SMS message
    
  if (strcmp(strlwr(number),"SMART") == 0){
    sendMessage(authNumber,smsbuffer); // forward smart message to authorized numbers
    }
  if (strcmp(strlwr(smsbuffer),"help") == 0)
    {
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print("Command Recv");
      /*Commands      
       * help - for list of commands
       * #abt* - to check systems idetity (Phone Number and Location)
       * #stp* - stop system alerts
       * #rst* - to restart/reboot the system
      */
      sendMessage(authNumber,"Send the following command \n#abt* - for About \n#stp* - to stop alerts  \n#rst* - to reset the system \n #alrton* - Setting alert to active");
    }
    if (strcmp(strlwr(smsbuffer),"#abt*") == 0)
    {
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("Command Recv");
    delay(1000);
    lcd.clear();
    sendMessage(authNumber, "Location: BINGA STAFF HOUSE \nNumber : 09280435560");  
    if(alert == true)
    {
    sendMessage(authNumber,"Alert is active");
    }
    if(alert == false)
    {
      sendMessage(authNumber,"Alert Dissabled");
    }
    }
     if (strcmp(strlwr(smsbuffer),"#stp*") == 0)
    {
     lcd.clear();
     lcd.setCursor(1,0);
     lcd.print("Command Recv");
     delay(1000);
     lcd.clear();
     sendMessage(authNumber,"Alert Stopped");
     alert = false;
    }
     if (strcmp(strlwr(smsbuffer),"#alrton*") == 0)
    {
     lcd.clear();
     lcd.setCursor(1,0);
     lcd.print("Command Recv");
     delay(1000);
     lcd.clear();
     sendMessage(authNumber,"Alert activated");
     alert = true;
    }
     if (strcmp(strlwr(smsbuffer),"#rst*") == 0)
    {
     lcd.clear();
     lcd.setCursor(1,0);
     lcd.print("Command Recv");
     sendMessage(authNumber,"SYSTEM WILL RESTART AFTER 5 SECS");
     lcd.clear();
     lcd.setCursor(1,0);
     Serial.println("System reboot");
     lcd.println("System reboot");
     delay(5000);
     digitalWrite(reset, LOW);
    }
    
  memset(smsbuffer, 0, sizeof(smsbuffer)); // clear SMS buffer
  digitalWrite(LED_BUILTIN, LOW); // turn off LED
}
void read_amp(){
  float ACCurrentValue = readACCurrentValue(); //read AC Current Value
  Serial.print(ACCurrentValue);
  Serial.println(" A");
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Current Value");
  lcd.setCursor(4,1);
  lcd.print(ACCurrentValue);  
  lcd.print(" A");
  delay(1000);

  if(readACCurrentValue() == 0.00 && has_run_off == true){
    if(alert == true){//alert switch
      sendMessage(authNumber,"NO POWER DETECTED"); // send SMS that system is ready to accept commands
      has_run_off = false;
      has_run_on = true;
    }
  }
  if(readACCurrentValue() > 0.01 && has_run_on == true){
    if(alert == true){//alert switch
    sendMessage(authNumber,"POWER DETECTED"); // send SMS that system is ready to accept commands
    has_run_off = true;
    has_run_on = false;
  }
  }
}
void setup() 
{
  lcd.init();// initialize the lcd 
  lcd.backlight();
  
  Serial.begin(115200); //for Clip sensor
  Serial.begin(9600);// for SMS
  pinMode(LED_BUILTIN, OUTPUT);

  GSMsetup(); //check GSM module if working 
  
  lcd.createChar(0,p20);
  lcd.createChar(1,p40);
  lcd.createChar(2,p60);
  lcd.createChar(3,p80);
  lcd.createChar(4,p100);  
  
  digitalWrite(reset, HIGH);
  pinMode(reset, OUTPUT);  
}

void loop() 
{
lcd.backlight();
receiveMessage(); // check for incoming SMS commands
read_amp();
}







