#include <ModbusSerial.h>
#include <Wire.h>
#include <RTClib.h>
#include "EmonLib.h"
#include <ETH.h>
#include <HTTPClient.h>
#include "PCAL9535A.h"
#include <ADS7828.h>


TaskHandle_t Task0;
TaskHandle_t Task1;

PCAL9535A::PCAL9535A<TwoWire> gpio(Wire);
RTC_PCF8563 rtc;
EnergyMonitor emon1;                   // Create an instance
ADS7828 adc(0x4B);                    // Set the ADS7828 i2c address to 0x49 (A0 connected to ground, A1 connected to 5v)


double Irms;
float calibracaoCorrente = 23.7;

int vetotIN[3] = {39, 34, 35};

// Used Pins
const int servoPin = 32;
const int TxenPin = 2; // -1 disables the feature, change that if you are using an RS485 driver, this pin would be connected to the DE and /RE pins of the driver.
const byte SlaveId = 1;
const int ServoHreg = 0;
const int SensorPin = 34;
const int SensorIreg = 0;
const int SwitchPin = 39;
const int SwitchIsts = 0;
const int LedPin = 13;
const int Lamp1Coil = 0;
long int ethdelay;
int bit0, bit1, bit2, bit3, bit4;

unsigned long previousMillisRelesOn = 0;
unsigned long previousMillisRelesOff = 0;
unsigned long intervalRelesOn = 20;  // Intervalo entre ligar os relés
unsigned long intervalRelesOff = 20; // Intervalo entre desligar os relés
unsigned long delayInterval = 100;   // Tempo de espera entre ligar e desligar

bool relesOn = true;
bool delayPeriod = false;
int currentRele = 9;
unsigned long delayStart = 0;

long ts;
long temp;
long temp2;
long temp3;

// Definir os pinos de saída digital
const int outputPins[] = {13, 12, 15, 18}; // Pinos de saída que serão controlados



#define MySerial Serial2 // define serial port used, Serial most of the time, or Serial1, Serial2 ... if available
const unsigned long Baudrate = 19200;

// ModbusSerial object
ModbusSerial mb (MySerial, SlaveId, TxenPin);

// Tipo de Ethernet PHY
#define ETH_TYPE ETH_PHY_LAN8720
// Endereço I2C de Ethernet PHY (0 ou 1 para LAN8720)
#define ETH_ADDR 0
#define ETH_PHY_ADDR 1
// Pino do sinal de relógio I2C para Ethernet PHY



#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN



static bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}






void setup() {


  Serial.begin (Baudrate);
  Serial.println ("oi");

  MySerial.begin (Baudrate); // works on all boards but the configuration is 8N1 which is incompatible with the MODBUS standard
  // prefer the line below instead if possible
  // MySerial.begin (Baudrate, MB_PARITY_EVEN);

  mb.config (Baudrate);
  // mb.setAdditionalServerData ("SWITCH"); // for Report Server ID function (0x11)


  // Add ServoHreg register - Use addHreg() for analog outpus or to store values in device
  mb.addHreg (ServoHreg, 0);

  pinMode(23, OUTPUT);
  digitalWrite(23, 1);

  WiFi.onEvent(WiFiEvent);
  ETH.begin( 1 , 32, 14, 4 , ETH_PHY_LAN8720);
  // mb.addIreg (SensorIreg);

  ts = millis();
  temp = millis();
  temp2 = millis();
  temp3 = millis();
 
  



  for (int i = 0; i < 3; i++) pinMode (vetotIN[i], INPUT_PULLUP);



  // Add SwitchIsts register - Use addIsts() for digital inputs
  mb.addIsts (0);
  mb.addIsts (1);
  mb.addIsts (2);


  // Adiciona registradores de holding para o RTC
  mb.addHreg(3);  // Ano
  mb.addHreg(2);  // Mês
  mb.addHreg(1);  // Dia
  mb.addHreg(4);  // Hora
  mb.addHreg(5);  // Minuto
  mb.addHreg(6);  // Segundo

  // Adiciona outros registradores necessários
  mb.addHreg(ServoHreg, 0);

  // Adiciona registradores de holding para a leitura de corrente
  mb.addHreg(8);  // corrente





  for (int i = 9; i <= 14; i++) {
    mb.addHreg(i); // Adiciona o registrador Modbus para cada endereço
  }

  for (int i = 0; i < 4; i++) {
    pinMode(outputPins[i], OUTPUT); // Configura cada pino como saída
  }

  mb.addHreg(15); //leitura de ETH

  mb.addHreg(16); //leitura das chaves

  
  for (int i = 3; i <= 11; i++) {
    mb.addIsts(i); // Adiciona o registrador Modbus para leitura do optos da 9x7
  }






 for (int o = 17; o < 25; o++) {
   mb.addHreg(o); //variaveis entrada analogica
 }


    

  emon1.current(36, calibracaoCorrente);

  digitalWrite (LedPin, 1);
  // Set LedPin mode
  pinMode (LedPin, OUTPUT);
  // Add Lamp1Coil register - Use addCoil() for digital outputs
  // mb.addCoil (Lamp1Coil);



  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Task0code, "Task0", 10000, NULL, 1, &Task0, 0);

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 1);


  Wire.begin(33, 5);  // Inicializa o barramento I2C com SDA no pino 16 e SCL no pino 5
//////////////////////////////////
  adc.init(INT);  
//////////////////////////////////
  gpio.begin(PCAL9535A::HardwareAddress::A000);  // Endereço 0x20

  for (int i = 9; i < 16; i++) {
    gpio.digitalWrite(i, 0); // Relé Off
    gpio.pinMode(i, OUTPUT);
  }
  
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }


  //rtc.adjust(DateTime(2023, 12, 07, 10, 23, 0));//Ajustar data e hora
}



void loop() {

}
