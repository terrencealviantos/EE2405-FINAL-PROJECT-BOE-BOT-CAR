#include "mbed.h"
#include "bbcar.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"

// GLOBAL VARIABLES
WiFiInterface *wifi;
//InterruptIn btn2(BUTTON1);
//InterruptIn btn3(SW3);
volatile int message_num = 0;
volatile int arrivedcount = 0;
volatile bool closed = false;

const char* topic = "Mbed";

Thread mqtt_thread;
EventQueue mqtt_queue;

Ticker servo_ticker;
Ticker servo_feedback_ticker;

//Thread carThread(osPriorityHigh);
Thread carThread;
EventQueue car_queue;

Thread measureThread;
EventQueue measure_queue;

Thread distanceThread;
EventQueue distance_queue;

//Car
PwmIn servo0_f(D9), servo1_f(D10); // servo0 - left ; servo1 - right
PwmOut servo0_c(D11), servo1_c(D12); 
BBCar car(servo0_c, servo0_f, servo1_c, servo1_f, servo_ticker, servo_feedback_ticker);

//Laser ping
DigitalInOut pin8(D8);
parallax_ping  ping1(pin8);

// QTI sensors
BusInOut qti_pin(D4,D5,D6,D7);
parallax_qti qti1(qti_pin);

int pattern;
int a; 
int objectDetection = 1; //for controlling object detection using LASER PING
int i = 0;
int distanceMeasurea = 1;
int distanceMeasureb = 1;
double theta0;
double theta1;
double distanceTotal;

const float PI = 3.14159;
const float rotP = 32.04424507;
float w[2];

int rotatingSpeed = 35; int carSpeed = 35;

void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;
    char msg[300];
    sprintf(msg, "Message arrived: QoS%d, retained %d, dup %d, packetID %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf(msg);
    ThisThread::sleep_for(2000ms);
    char payload[300];
    sprintf(payload, "Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    printf(payload);
    ++arrivedcount;
}

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client) {
    message_num++;
    MQTT::Message message;
    char buff[100];
    sprintf(buff, "QoS0 Hello, Python! #%d", message_num);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buff;
    message.payloadlen = strlen(buff) + 1;
    int rc = client->publish(topic, message);

    printf("rc:  %d\r\n", rc);
    printf("Puslish message: %s\r\n", buff);
}

void close_mqtt() {
    closed = true;
}

void widthCalculation (double d, double currentWheelAngle, double initWheelAngle ){
    float wheelAngle, rotAngle, p, theta;
    wheelAngle =  abs(currentWheelAngle) - abs(initWheelAngle);
    p = 6.7*PI*( abs(wheelAngle) / 360 );
    theta = (p / rotP)*360; 
    rotAngle = (theta*PI) / 180; // deg to rad
    w[i] = sin(rotAngle)*d;
    i++;
}

void obsMeasure(){
    printf("Start Obs Measure. \n");
    double d1, d2, obsMeasureAngle0;
    obsMeasureAngle0 = car.servo1.angle;
   // car_queue.call(&publish_message, &client);
    while(1) {
        d1 = (float)ping1;
        if (d1 > 20){
            car.rotate(-rotatingSpeed);
        }
        else{
            car.stop();
            break;
        }
        ThisThread::sleep_for(10ms);
    }
    widthCalculation(d1, car.servo1.angle, obsMeasureAngle0);
    ThisThread::sleep_for(500ms);
    
    // 2nd object
    while(1) {
        pattern = (int)qti1;
        if (pattern != 0b0110){
            car.rotate(rotatingSpeed);
        }
        else {
            car.stop();
            obsMeasureAngle0 = car.servo1.angle; 
            ThisThread::sleep_for(1s);
            break;
        }
    }
    while(1){
        d2 = (float)ping1;
        if (d2 > 20){
            car.rotate(rotatingSpeed);
        }
        else{
            car.stop();
            break;
        }
        ThisThread::sleep_for(10ms);
    }
    
    widthCalculation(d2 , car.servo1.angle, obsMeasureAngle0);
    
    while(1){
       pattern = (int)qti1;
       if ( pattern == 0b0110){
           car.stop();
           break;
       }
       else{ 
           car.rotate(-rotatingSpeed);
       }
    }

    printf("Distance between 2 objects: %f\n", w[0] + w[1]);

    if ( w[0] + w[1] > 11)
    {
       objectDetection = 1; // the gap is wide enough, give permission to go
       printf("Finished object detection. \n");
    }
    else 
    {
        objectDetection = 0;
    }
}

void carDriving(){
    if(objectDetection){
      pattern = (int)qti1;
      //printf("%d\n",pattern);
          switch (pattern) {
            case 0b1000: car.turn(carSpeed, 0.1); a = 0; break;
            case 0b1100: car.turn(carSpeed, 0.3); a = 0; break;
            case 0b0100: car.turn(carSpeed, 0.7); a = 0; break;
            case 0b0110: car.goStraight(carSpeed); a = 0; break;
            case 0b0010: car.turn(carSpeed, -0.7); a = 0; break;
            case 0b0011: car.turn(carSpeed, -0.3); a = 0; break;
            case 0b0001: car.turn(carSpeed, -0.1); a = 0; break;
            case 0b0111: 
            { // TASK 1: turning left, pattern 7
                if (a==0){
                    car.stop();
                    a++;
                    ThisThread::sleep_for(500ms); 
                    break;
                }
                else{
                    printf("GO left \n");
                    car.goStraight(carSpeed);
                    ThisThread::sleep_for(200ms);
                    car.turn(carSpeed, 0.2);
                    ThisThread::sleep_for(200ms); 
                    break;
                }
            }
            
            case 0b1111: 
            { //TASK 3: FINISHING THE LOOP 
                car.stop();
                //printf("ROUTE FINISHED\n");
               //distanceMeasure = 1;
                if(distanceMeasurea == 1)
                {
                    printf("Distance travelled finished : %f cm\n", distanceTotal);
                    distanceMeasurea = 0;
                }
                //car_queue.call(&publish_message, &client);
                a = 1; //to stop all the process

                break;
            }
            case 0b0000: {// TASK 2:OBJECT DETECTION
                if(a==0)
                {
                    printf("Start Object detection. \n");
                    car.goStraight(carSpeed);
                    ThisThread::sleep_for(800ms);
                    car.stop();
                    ThisThread::sleep_for(500ms);
                    objectDetection = 0;
                    a=1;
                    //ThisThread::sleep_for(500ms);
                    measure_queue.call(obsMeasure);
                    break;
                }
                else{
                    // car.goStraight(carSpeed);
                    // ThisThread::sleep_for(1s);
                    break;
                }
            } 
            default: car.goStraight(carSpeed); break;
          }
      }
      else 
      {
      //distanceMeasure = 1;
      if(distanceMeasureb == 1)
        {            
            printf("Distance travelled half : %f cm\n", distanceTotal);
            distanceMeasureb = 0;
        }
      }
}

void measureTheDistance()
{
    theta1 = car.servo1.angle;
    distanceTotal = 6.7*PI*(abs(theta1-theta0)/360);

    // if(distanceMeasure == 1)
    // {
    //     printf("Distance travelled FINAL : %f\n", distanceTotal);
    // }
    // else 
    // {
    //     printf("Distance travelled not final : %f\n", distanceTotal);
    // }
}

int main() {
    theta0 = car.servo1.angle; //first angle

    // wifi = WiFiInterface::get_default_instance();
    // if (!wifi) {
    //         printf("ERROR: No WiFiInterface found.\r\n");
    //         return -1;
    // }


    // printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
    // int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    // if (ret != 0) {
    //         printf("\nConnection error: %d\r\n", ret);
    //         return -1;
    // }


    // NetworkInterface* net = wifi;
    // MQTTNetwork mqttNetwork(net);
    // MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    // //TODO: revise host to your IP
    // const char* host = "172.20.10.10";
    // const int port=1883;
    // printf("Connecting to TCP network...\r\n");
    // printf("address is %s/%d\r\n", host, port);

    // int rc = mqttNetwork.connect(host, port);//(host, 1883);
    // if (rc != 0) {
    //         printf("Connection error.");
    //         return -1;
    // }
    // printf("Successfully connected!\r\n");

    // MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    // data.MQTTVersion = 3;
    // data.clientID.cstring = "Mbed";

    // if ((rc = client.connect(data)) != 0){
    //         printf("Fail to connect MQTT\r\n");
    // }
    // if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){
    //         printf("Fail to subscribe\r\n");
    // }

    // mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));
    // mqtt_queue.call(mqtt_queue.event(&publish_message, &client));

    //CAR PART//
    car_queue.call_every(10ms, &carDriving);
    carThread.start(callback(&car_queue, &EventQueue::dispatch_forever));
    measureThread.start(callback(&measure_queue, &EventQueue::dispatch_forever));
    distance_queue.call_every(15ms, &measureTheDistance);
    distanceThread.start(callback(&distance_queue, &EventQueue::dispatch_forever));
    ////////////////////////


    // while (1) {
    //     if (closed) break;
    //     client.yield(500);
    //     ThisThread::sleep_for(1s);
    // }
  
    // printf("Ready to close MQTT Network......\n");

    // if ((rc = client.unsubscribe(topic)) != 0) {
    //     printf("Failed: rc from unsubscribe was %d\n", rc);
    // }
    // if ((rc = client.disconnect()) != 0) {
    //     printf("Failed: rc from disconnect was %d\n", rc);
    // }

    // mqttNetwork.disconnect();
    // printf("Successfully closed!\n");

    // return 0;
}
