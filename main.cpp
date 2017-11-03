#include "mbed.h"
#include "ESP8266.h"
 
Serial pc(USBTX,USBRX);

static EventQueue eventQueue;               // An event queue
static Thread eventThread;                  // An RTOS thread to process events in

// Moisture sensor
AnalogIn temperatureSensor(A0);

//wifi UART port and baud rate
ESP8266 wifi(D8, D2, 115200); 

//buffers for wifi library
char resp[1000];
char http_cmd[300], comm[300];

int timeout = 8000; //timeout for wifi commands

//SSID and password for connection
#define SSID "manu" 
#define PASS "20002000"  

//Remote IP
#define IP "184.106.153.149"

//global variable
float moistvalue = 0; 

//Update key for thingspeak
char* Update_Key = "L9CSTK6I7D7RBYWQ";
 
//Wifi init function
void wifi_initialize(void){
    
    pc.printf("******** Resetting wifi module ********\r\n");
    wifi.Reset();
    
    //wait for 5 seconds for response, else display no response receiveed
    if (wifi.RcvReply(resp, 5000))    
        pc.printf("%s",resp);    
    else
        pc.printf("No response");
    
    pc.printf("******** Setting Station mode of wifi with AP ********\r\n");
    wifi.SetMode(1);    // set transparent  mode
    if (wifi.RcvReply(resp, timeout))    //receive a response from ESP
        pc.printf("%s",resp);    //Print the response onscreen
    else
        pc.printf("No response while setting mode. \r\n");
    
    pc.printf("******** Joining network with SSID and PASS ********\r\n");
    wifi.Join(SSID, PASS);     
    if (wifi.RcvReply(resp, timeout))    
        pc.printf("%s",resp);   
    else
        pc.printf("No response while connecting to network \r\n");
        
    pc.printf("******** Getting IP and MAC of module ********\r\n");
    wifi.GetIP(resp);     
    if (wifi.RcvReply(resp, timeout))    
        pc.printf("%s",resp);    
    else
        pc.printf("No response while getting IP \r\n");
    
    pc.printf("******** Setting WIFI UART passthrough ********\r\n");
    wifi.setTransparent();          
    if (wifi.RcvReply(resp, timeout))    
        pc.printf("%s",resp);    
    else
        pc.printf("No response while setting wifi passthrough. \r\n");
    wait(1);    
    
    pc.printf("******** Setting single connection mode ********\r\n");
    wifi.SetSingle();             
    wifi.RcvReply(resp, timeout);
    if (wifi.RcvReply(resp, timeout))    
        pc.printf("%s",resp);    
    else
        pc.printf("No response while setting single connection \r\n");
    wait(1);
}

void wifi_send(void){
    
    pc.printf("******** Starting TCP connection on IP and port ********\r\n");
    wifi.startTCPConn(IP,80);    //cipstart
    wifi.RcvReply(resp, timeout);
    if (wifi.RcvReply(resp, timeout))    
        pc.printf("%s",resp);    
    else
        pc.printf("No response while starting TCP connection \r\n");
    wait(1);
    
    //create link 
    sprintf(http_cmd,"/update?api_key=%s&field1=%f",Update_Key,moistvalue); 
    pc.printf(http_cmd);
    
    pc.printf("******** Sending URL to wifi ********\r\n");
    wifi.sendURL(http_cmd, comm);   //cipsend and get command
    if (wifi.RcvReply(resp, timeout))    
        pc.printf("%s",resp);    
    else
        pc.printf("No response while sending URL \r\n");
}
void update_moisture(){
          int ret;
     unsigned int a, beta = 3975, units, tens;
     float temperature, resistance;
  
            // send temperature value to M2X every 5.5 seconds
        a = temperatureSensor.read_u16(); /* Read analog value */
        /* Calculate the resistance of the thermistor from analog votage read. */
        resistance = (float) 10000.0 * ((65536.0 / a) - 1.0);

        /* Convert the resistance to temperature using Steinhart's Hart equation */
        temperature =(1/((log(resistance/10000.0)/beta) + (1.0/298.15)))-273.15; 
    moistvalue =temperature;
    }
void update_ThingSpeak()
{
        pc.printf("Current moistvalue is = %.3f \r\n", moistvalue);
        wifi_send();
        wait(1);
    
    }
int main () {
    pc.baud(115200);
    wifi_initialize();
      // Using an event queue is a very useful abstraction around many microcontroller 'problems', like dealing with ISRs
    // see https://developer.mbed.org/blog/entry/Simplify-your-code-with-mbed-events/
    eventThread.start(callback(&eventQueue, &EventQueue::dispatch_forever));
    // Read the moisture data every second
    eventQueue.call_every(1000, &update_moisture);
    //update the moisture value to Thingspeak Servers
    eventQueue.call_every(5000, &update_ThingSpeak);
    wait(osWaitForever);
}
