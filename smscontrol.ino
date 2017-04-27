/**
 * This sketch is written for the Arduino Mega ADK and the official Arduino GSM shield (Quectel M10 modem).
 * It reads incoming SMS messages and translates them to mavlink commands for the Pixhawk autopilot.
 * For more information read the ReadMe.txt
 *
 *  Copyright (C) 2016 Giorgos Solidakis
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software completedation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author     Giorgos Solidakis <gsolidakis@outlook.com>
 * @github     https://github.com/PGSIR/GSM_PX4.git
 * @license    http://www.gnu.org/licenses/gpl.txt  GNU - General Public License 3
 * @version    1.01
 */

#include <FastSerial.h>
#include <MAVLink_Commands.h>
#include <GSM_Commands.h>
#include <SMS_Auth.h>

// Baudrate
#define baud 57600

// Default password for SMS_Auth. Password is case sensitive !
#define DEFAULT_PASSWORD "a"

MAVLink_Commands mv(baud);
GSM_Commands gsm(baud);
SMS_Auth auth(DEFAULT_PASSWORD);

// Serial communication (TERMINAL / DEBUGGING)
FastSerialPort0(Serial);


// Store recieved sms data.
// struct SMS, defined in GSM_commands.h
SMS sms;

// Indicates weather GPRS is activated to use the modem as a bridge between UAV and GCS
bool GPRS_isActive = false;

unsigned long cur_time = millis();
unsigned long timer = millis();


void setup() {

  //Initialize Serial communication
  Serial.begin(baud);

  // Initialize system settings
  while(!initialize())
  {
    // Retry initializing the system if there were errors
  }
  if(GPRS_isActive)
  {
    gsm.SET_MODEM_BAUD(baud);
    Serial.println("Modem baud set");
    delay(1000);

    gsm.SET_TRANSPARENT_TRANSFERRING_MODE();
    Serial.println("TRANSPARENT_BRIDGE_MODE");
    delay(1000);

    gsm.GPRS_CONNECT();
    Serial.println("GPRS CONNECTED");
    delay(1000);

    gsm.SET_LOCAL_PORT("UDP","14550");
    Serial.println("LOCAL_PORT");
    delay(1000);

    gsm.START_CONNECTION("UDP","141.237.164.236", "14550");
    Serial.println("START_CONNECTION");
    delay(1000); 
  }
    
}

void loop() {
 /**
  // Relay All GSM Module communication to Autopilot
    if(Serial1.available()){
        char a = Serial1.read();
        Serial2.write(a);
        Serial.println(a);
    }

    // Relay all Autopilot communication to GSM module and USB (USB for monitor/debug only)
    if(Serial2.available()){
        char b = Serial2.read();
        Serial1.write(b);
        Serial.println(b);
    }

 **/

  sms = gsm.NEXT();

  handle(sms);
 

}

void handle(SMS sms) {

  String phone_num = sms.get_phone();
  String date_time = sms.get_date();
  String message = sms.get_msg();


  // If the message is empty return
  if(message == "") 
    return;

  Serial.println("\n->Phone : "+phone_num+"\n->Date & time : "+date_time+"\n->Payload : "+message);

  // Check if the command came from the authorized operator's phone number
  if(!auth.is_AUTHORIZED(phone_num))
  {
    Serial.println("Command from unauthorized user, trying to login");
    delay(500);
    // If the message is the correct password then login the user
    if(auth.LOGIN(phone_num,message))
    {
      Serial.println("Logged in successfully");
    }else
    {
      Serial.println("Authorization failed.");
    }
    return;
  }else
  {
    Serial.println("Executing command from opperator :"+auth.GET_OPPERATOR());
    EXECUTE(message);
  }
}

void EXECUTE(String message) {

  message.toLowerCase();

  if(message.indexOf("test") >= 0 || message.indexOf("example") >= 0)
  {
    mv.MAV_SET_MODE(GUIDED); //set mode to guided
    delay(200);
    mv.MAV_SEND_COMMAND(400,1,0,0,0,0,0,0); //arm
    delay(200);
    mv.MAV_SEND_COMMAND(22,0,0,0,0,0,0,10); //takeoff 10m
    delay(200);
    mv.MAV_SET_WP(-35.361252,149.162002,1000); //go to  wp -35.361252 149.162002
    Serial.println("Executing demonstration procedure.");
    delay(1000);
  }
  if(message.indexOf("arm") >= 0)
  {
    mv.MAV_SET_MODE(GUIDED); //set mode to guided
    mv.MAV_SEND_COMMAND(400,1,0,0,0,0,0,0); //arm

    int throttle = message.substring(4, message.length()).toInt();
    throttle = throttle/100;
    throttle = throttle * 2000;
    mv.MAV_RC_OVERRIDE(0,0,throttle,0,0,0,0,0); // Set throttle
  }
  if(message.indexOf("throttle") >= 0 )
  {
    int throttle = message.substring(8, message.length()).toInt();
    throttle = throttle/100;
    throttle = throttle * 2000;
    mv.MAV_RC_OVERRIDE(0,0,throttle,0,0,0,0,0); // Set throttle
  }
  if(message.indexOf("takeoff") >= 0)
  {
    float alt = message.substring(7, message.length()).toInt();
    mv.MAV_SEND_COMMAND(22,0,0,0,0,0,0,alt); //takeoff 
  }
  if(message.indexOf("goto") >= 0)
  {

    String parameters = message.substring(4, message.length());

    String lon = parameters.substring(0,parameters.indexOf(','));
    char floatlonbuf[lon.length()+1];
    lon.toCharArray(floatlonbuf, sizeof(floatlonbuf));
    float longitude = atof(floatlonbuf);

    parameters = parameters.substring(parameters.indexOf(',')+1,parameters.length());

    String lat = parameters.substring(0,parameters.indexOf(','));
    char floatlatbuf[lat.length()+1];
    lon.toCharArray(floatlatbuf, sizeof(floatlatbuf));
    float latitude = atof(floatlatbuf);

    parameters = parameters.substring(parameters.indexOf(',')+1,parameters.length());

    String alt = parameters.substring(0,parameters.indexOf(','));
    char floataltbuf[alt.length()+1];
    lon.toCharArray(floataltbuf, sizeof(floataltbuf));
    float altitude = atof(floataltbuf);

    //go to waypoint
    mv.MAV_SET_WP(longitude,latitude,altitude); 
    delay(100);
  }
  if(message.indexOf("rtl") >= 0)
  {
    mv.MAV_SEND_COMMAND(20,0,0,0,0,0,0,0);
    delay(100);
  }
  if(message.indexOf("gprs on") >= 0)
  {
    gsm.SET_MODEM_BAUD(baud);
    Serial.println("Modem baud set");
    delay(1000);

    gsm.SET_TRANSPARENT_TRANSFERRING_MODE();
    Serial.println("TRANSPARENT_BRIDGE_MODE");
    delay(1000);

    gsm.GPRS_CONNECT();
    Serial.println("GPRS CONNECTED");
    delay(1000);

    gsm.SET_LOCAL_PORT("UDP","14550");
    Serial.println("LOCAL_PORT");
    delay(1000);

    gsm.START_CONNECTION("UDP","141.237.164.236", "14550");
    Serial.println("START_CONNECTION");
    delay(1000); 

    GPRS_isActive = true;
  }
  if(message.indexOf("gprs off") >= 0)
  {
    gsm.SET_NORMAL_TRANSFERRING_MODE();
    gsm.DEACTIVATE_PDP_CONTEXT();

    GPRS_isActive = false;
  }
}


/********************************************
 * Initializes the system
 *******************************************/

bool initialize() {

  Serial.println("**********************************************************************************");
  Serial.println("                                   GSM_PX4                                        ");
  Serial.println("**********************************************************************************");

  int error = 0;

  if(gsm.DISABLE_ECHO())
  {
    Serial.println("ECHO : OFF");
  }else
  {
    Serial.println("ECHO : ERROR");
    error++;
  }
  if(gsm.GSM_STATUS())
  {
    Serial.println("GSM : OK");
  }else
  {
    Serial.println("GSM : ERROR");
    error++;
  }
  if(gsm.GPRS_STATUS())
  {
    Serial.println("GPRS : OK");
  }else
  {
    Serial.println("GPRS : ERROR");
    error++;
  }
  if(gsm.SMS_MODE())
  {
    Serial.println("SMS_MODE : TEXT");
  }else
  {
    Serial.println("SMS_MODE : ERROR");
    error++;
  }
  if(gsm.SMS_NOTIFICATION())
  {
    Serial.println("SMS_NOTIFICATION : ON");
  }else
  {
    Serial.println("SMS_NOTIFICATION : ERROR");
    error++;
  }

  if(error > 0) 
  {
    Serial.println("WARNING: There were errors during system initialization restarting"); // why????
    delay(500);
    Serial.print(".");
    delay(500);
    Serial.print(".");
    delay(500);
    Serial.println(".");
    return false;
  }

  SHOW_WARNING();

  return true;
  
}

void SHOW_WARNING() {
  
  Serial.println("**********************************************************************************");
  Serial.println("*                                  WARNING!                                      *");
  Serial.println("**********************************************************************************");
  Serial.println("\nThis version of the program only accepts English characters for SMS commands,");
  Serial.println("make sure your phone is set to use the English language for SMS texts.\n");
  Serial.println("**********************************************************************************");
  
}