/*
Desenvolvido por Judenilson Araujo Silva, www.judenilson.com.br
para uso no simulador de voo X-Plane, qualquer dúvida ou melhoramento, favor enviar pelo meio mais conveniente.
O uso é irrestrito e totalmente Open Source, você pode fazer o que quiser com o código, mas, se melhorar não deixe de informar por gentileza.
No entanto, ajude a difundir o código aberto fazendo o mesmo, se vc recebeu de graça, dê de graça também, ou pelo menos por um preço justo.
Abraço
*/

#include <WiFi.h>
#include "Arduino.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h> 

const char* ssid     = "Judenilson_2.4"; //Colocar aqui o SSID do WiFi
const char* password = "judenilson82"; //Senha do wifi

// definindo GPIO dos _potenciômetros
const int _sensorPinA = A0;  // speedbrake
const int _sensorPinB = A3;  // throttle 1
const int _sensorPinC = A6;  // throttle 2
const int _sensorPinD = A7;  // propeller
const int _sensorPinE = A4;  // mixture
const int _sensorPinF = A5;  // flaps
// variáveis para guardar o valor do _potenciômetro
int _sensorValueA = 0, _sensorValueSaveA = 0;
int _sensorValueB = 0, _sensorValueSaveB = 0;
int _sensorValueC = 0, _sensorValueSaveC = 0;
int _sensorValueD = 0, _sensorValueSaveD = 0;
int _sensorValueE = 0, _sensorValueSaveE = 0;
int _sensorValueF = 0, _sensorValueSaveF = 0;
const int _correcaoPot = 0;	// variação para correção do ruido no _potenciômetro

// definindo GPIO dos Leds
const int _redLeftPin = 27;
const int _redNosePin = 14;
const int _redRightPin = 12;
const int _greenLeftPin = 13;
const int _greenNosePin = 18;
const int _greenRightPin = 5;
const int _parkBrakePin = 19;

int _gear_handle_down = 0;
float _bus_volts = 0;
bool _canLight = false;

// Variáveis para conexão com X-Plane

IPAddress ipMulticast ( 239,255,1,1 ); // Broadcast do XP, será usado para saber qual ip oXP tá instalado
const int portMulticast = 49707; //Porta do broadcast do XP
WiFiUDP _Udp;	//instanciando o objeto de conexão.
WiFiUDP _UdpB;	//instanciando o objeto de conexão.

char _packetBuffer[500]; //Buffer onde se obtém a informação vinda do XP
int _packetSize; // tamanho da mensagem recebida
uint8_t _bufferEnvio[413]; //buffer de envio para o XP
int _longBuffer;

// Estruturas de dados para armazenamento de insformações
struct DATA {
  char comando[4];
  char nulo;
  char data[495];
};

struct DATABECN {
  char comando[4];
  char nulo;
  char beacon_major_version[1];
  char beacon_minor_version[1];
  char application_host_id[4];
  char version_number[4];
  char role[4];
  char port[2];
  char computer_name[500];
};

struct INFOXPLANE {
  unsigned int beacon_major_version;
  unsigned int beacon_minor_version;
  unsigned int application_host_id;
  unsigned int version_number;
  unsigned int role;
  unsigned int port;
  char computer_name[500];
  IPAddress Ip;
};

DATA _receivedData;
DATABECN _dataBecn;
INFOXPLANE _infoXplane;

TaskHandle_t task_read;
TaskHandle_t task_write;

bool _semaphore = true;

void setup() { // SETUP ----------------------------------------------------------------------------------------------------------------------------
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connectando wifi");

	pinMode(_redLeftPin, OUTPUT);
	pinMode(_redNosePin, OUTPUT);
	pinMode(_redRightPin, OUTPUT);
	pinMode(_greenLeftPin, OUTPUT);
	pinMode(_greenNosePin, OUTPUT);
	pinMode(_greenRightPin, OUTPUT);
	pinMode(_parkBrakePin, OUTPUT);
	digitalWrite(_redLeftPin, LOW);
	digitalWrite(_redNosePin, LOW);
	digitalWrite(_redRightPin, LOW);
	digitalWrite(_greenLeftPin, LOW);
	digitalWrite(_greenNosePin, LOW);
	digitalWrite(_greenRightPin, LOW);
	digitalWrite(_parkBrakePin, LOW);

  // analogSetWidth(9); // setando as entradas analógicas para 9 bits

  WiFi.mode(WIFI_STA); //setando o wifi p conectar num roteador wifi
  WiFi.begin(ssid, password); //inicializando o wifi

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  infoXplane(WiFi.localIP()); // Metodo para conectar no X-Plane a primeira vez e pegar alguns dados.

  _Udp.begin(_infoXplane.port); //iniciando a comunicação UDP na porta do XP (padrão 49000)
  // Assinando as variáveis, para o XP começar a transmitir via UDP. 
  // (Frequência em Hz (vezes por segundo), ID, DataRef)
  // subscribeDataRef(2,1,"sim/flightmodel/controls/parkbrake");
  // subscribeDataRef(2,2,"sim/flightmodel2/gear/deploy_ratio[1]");
  // subscribeDataRef(2,3,"sim/flightmodel2/gear/deploy_ratio[0]");
  // subscribeDataRef(2,4,"sim/flightmodel2/gear/deploy_ratio[2]");
  // subscribeDataRef(2,5,"sim/cockpit2/controls/gear_handle_down");
  // subscribeDataRef(1,6,"sim/cockpit2/electrical/bus_volts[0]");

  // remove
  // subscribeDataRef(0,1,"sim/flightmodel/controls/parkbrake");
  // subscribeDataRef(0,2,"sim/flightmodel2/gear/deploy_ratio[1]");
  // subscribeDataRef(0,3,"sim/flightmodel2/gear/deploy_ratio[0]");
  // subscribeDataRef(0,4,"sim/flightmodel2/gear/deploy_ratio[2]");
  // subscribeDataRef(0,5,"sim/cockpit2/controls/gear_handle_down");
  // subscribeDataRef(0,6,"sim/cockpit2/electrical/bus_volts[0]");

// Usando DualCore para ler e escreder os dados via UDP ---------------------------------------------------------------
	// xTaskCreatePinnedToCore(readDataRefs, "readDataRefs", 4096, NULL, 1, &task_read, 1);
	// delay(127);
	// xTaskCreatePinnedToCore(writeDataRefs, "writeDataRefs", 4096, NULL, 1, &task_write, 0); 
	// delay(127);
}

void loop(){ 
	_sensorValueA = analogRead(_sensorPinA); 
  _sensorValueB = analogRead(_sensorPinB); 
  _sensorValueC = analogRead(_sensorPinC); 
  _sensorValueD = analogRead(_sensorPinD); 
  _sensorValueE = analogRead(_sensorPinE); 
  _sensorValueF = analogRead(_sensorPinF); 
  float _potValue = 0.0;
  
  // Serial.print(_sensorValueA); Serial.print(" - ");
  // Serial.print(_sensorValueB); Serial.print(" - ");
  // Serial.print(_sensorValueC); Serial.print(" - ");
  // Serial.print(_sensorValueD); Serial.print(" - ");
  // Serial.print(_sensorValueE); Serial.print(" - ");
  // Serial.println(_sensorValueF);

  if ((_sensorValueA < _sensorValueSaveA-_correcaoPot) || (_sensorValueA > _sensorValueSaveA+_correcaoPot)){
  		int value = map(_sensorValueA,800,2700,255,0);
  		_potValue = (float)value/256;
  		if (_potValue <= 0){ _potValue = 0.0; }
  		if (_potValue >= 1){ _potValue = 1.0; } 
    	writeDataRef(_potValue,"sim/multiplayer/controls/speed_brake_request[0]");
    	_sensorValueSaveA = _sensorValueA;
	}
  if ((_sensorValueB < _sensorValueSaveB-_correcaoPot) || (_sensorValueB > _sensorValueSaveB+_correcaoPot)){
  		int value = map(_sensorValueB,500,2800,0,255);
  		_potValue = (float)value/256;
  		if (_potValue <= 0){ _potValue = 0.0; }
  		if (_potValue >= 1){ _potValue = 1.0; } 
    	writeDataRef(_potValue,"sim/flightmodel/engine/ENGN_thro[0]");	
    	_sensorValueSaveB = _sensorValueB;
	}
  if ((_sensorValueC < _sensorValueSaveC-_correcaoPot) || (_sensorValueC > _sensorValueSaveC+_correcaoPot)){
  		int value = map(_sensorValueC,500,2800,0,255);
  		_potValue = (float)value/256;
  		if (_potValue <= 0){ _potValue = 0.0; }
  		if (_potValue >= 1){ _potValue = 1.0; } 
    	writeDataRef(_potValue,"sim/flightmodel/engine/ENGN_thro[1]");
    	_sensorValueSaveC = _sensorValueC;
	}  
	if (_sensorValueB < 400){
  		if (_sensorValueB >= 200){ 
  			writeDataRef(1,"sim/flightmodel/engine/ENGN_propmode[0]");			
    	  writeDataRef(0.0,"sim/flightmodel/engine/ENGN_thro[0]");}
  		else { 
  			writeDataRef(3,"sim/flightmodel/engine/ENGN_propmode[0]");
    	  writeDataRef(1.0,"sim/flightmodel/engine/ENGN_thro[0]");
  		}    	
    	_sensorValueSaveB = _sensorValueB;
	}
  if (_sensorValueC < 400){
  		if (_sensorValueC >= 200){ 
  			writeDataRef(1,"sim/flightmodel/engine/ENGN_propmode[1]");
    	  writeDataRef(0.0,"sim/flightmodel/engine/ENGN_thro[1]");}
  		else { 
  			writeDataRef(3,"sim/flightmodel/engine/ENGN_propmode[1]");
    	  writeDataRef(1.0,"sim/flightmodel/engine/ENGN_thro[1]");
  		}    	
    	_sensorValueSaveC = _sensorValueC;
	}
  if ((_sensorValueD < _sensorValueSaveD-_correcaoPot) || (_sensorValueD > _sensorValueSaveD+_correcaoPot)){
  	  	int value = map(_sensorValueD,500,2800,115,270);
  		_potValue = value;
  		if (_potValue <= 115){ _potValue = 115; }
  		if (_potValue >= 270){ _potValue = 270; } 
    	writeDataRef(_potValue,"sim/cockpit2/engine/actuators/prop_rotation_speed_rad_sec_all");
    	_sensorValueSaveD = _sensorValueD;
	}
  if ((_sensorValueE < _sensorValueSaveE-_correcaoPot) || (_sensorValueE > _sensorValueSaveE+_correcaoPot)){
  		int value = map(_sensorValueE,500,2800,0,255);
  		_potValue = (float)value/256;
  		if (_potValue <= 0){ _potValue = 0.0; }
  		if (_potValue >= 1){ _potValue = 1.0; } 
    	writeDataRef(_potValue,"sim/flightmodel/engine/ENGN_mixt[0]");
    	writeDataRef(_potValue,"sim/flightmodel/engine/ENGN_mixt[1]");
    	_sensorValueSaveE = _sensorValueE;
	}
  if ((_sensorValueF < _sensorValueSaveF-_correcaoPot) || (_sensorValueF > _sensorValueSaveF+_correcaoPot)){
  		int value = map(_sensorValueF,500,2800,255,0);
  		_potValue = (float)value/256;
  		if (_potValue <= 0){ _potValue = 0.0; }
  		else if (_potValue > 0 && _potValue <= 0.125){ _potValue = 0.125; }
  		else if (_potValue > 0.125 && _potValue <= 0.250){ _potValue = 0.250; }
  		else if (_potValue > 0.250 && _potValue <= 0.375){ _potValue = 0.375; }
  		else if (_potValue > 0.375 && _potValue <= 0.500){ _potValue = 0.500; }
  		else if (_potValue > 0.500 && _potValue <= 0.625){ _potValue = 0.625; }
  		else if (_potValue > 0.625 && _potValue <= 0.750){ _potValue = 0.750; }
  		else if (_potValue > 0.750 && _potValue <= 0.875){ _potValue = 0.875; }
  		else { _potValue = 1.0; }  
    	writeDataRef(_potValue,"sim/cockpit2/controls/flap_ratio");
    	_sensorValueSaveF = _sensorValueF;
	}
}

void readDataRefs(void *args) {
	while(1){
	  bool infoOk = false;
	  char tipo[4];
	  unsigned int ordemID;
	  float valor;
	  int numDatos;	  

		if(_semaphore){
			_semaphore = false;

		  while (!infoOk) {
				bool received = false;
		    while (!received) {
		      _packetSize = _Udp.parsePacket();
		      if (_packetSize) {
		        _Udp.read(_packetBuffer, _packetSize);
		        received = true;
		      }
			  }
		    memcpy (&tipo, &_packetBuffer, 4);		    
		    numDatos = (_packetSize-5)/8;
		    for (int x=0;x <numDatos;x++) {
		      memcpy (&ordemID, &_packetBuffer[(x*8)+5], sizeof(unsigned int));
		      memcpy (&valor, &_packetBuffer[(x*8)+9], sizeof(float));
		      dataRefOut(ordemID, valor);
		    }
		    infoOk = true;
		  }  

		  _semaphore = true;
		  vTaskDelay(97);
		}	  

	}
}

// void writeDataRefs(void *args){
// 	while(1){
// 		_sensorValueA = analogRead(_sensorPinA);
// 	  _sensorValueB = analogRead(_sensorPinB);  
// 	  _sensorValueC = analogRead(_sensorPinC);
// 	  _sensorValueD = analogRead(_sensorPinD);
// 	  _sensorValueE = analogRead(_sensorPinE);
// 	  _sensorValueF = analogRead(_sensorPinF);

// 	  if ((_sensorValueA < _sensorValueSaveA-_correcaoPot) || (_sensorValueA > _sensorValueSaveA+_correcaoPot)){
// 	  		int value = map(_sensorValueA,90,330,511,0);
// 	  		_potA = (float)value/512;
// 	  		if (_potA <= 0){ _potA = 0.0; }
// 	  		if (_potA >= 1){ _potA = 1.0; } 
// 	    	writeDataRef(_potA,"sim/multiplayer/controls/speed_brake_request[0]");
// 	    	_sensorValueSaveA = _sensorValueA;
// 		}
// 	  if ((_sensorValueB < _sensorValueSaveB-_correcaoPot) || (_sensorValueB > _sensorValueSaveB+_correcaoPot)){
// 	  		int value = map(_sensorValueB,90,330,0,511);
// 	  		_potB = (float)value/512;
// 	  		if (_potB <= 0){ _potB = 0.0; }
// 	  		if (_potB >= 1){ _potB = 1.0; } 
// 	    	writeDataRef(_potB,"sim/flightmodel/engine/ENGN_thro[0]");
// 	    	_sensorValueSaveB = _sensorValueB;
// 		}
// 	  if ((_sensorValueC < _sensorValueSaveC-_correcaoPot) || (_sensorValueC > _sensorValueSaveC+_correcaoPot)){
// 	  		int value = map(_sensorValueC,90,330,0,511);
// 	  		_potC = (float)value/512;
// 	  		if (_potC <= 0){ _potC = 0.0; }
// 	  		if (_potC >= 1){ _potC = 1.0; } 
// 	    	writeDataRef(_potC,"sim/flightmodel/engine/ENGN_thro[1]");
// 	    	_sensorValueSaveC = _sensorValueC;
// 		}
// 	  if ((_sensorValueD < _sensorValueSaveD-_correcaoPot) || (_sensorValueD > _sensorValueSaveD+_correcaoPot)){
// 	  	  	int value = map(_sensorValueD,90,330,115,270);
// 	  		_potD = value;
// 	  		if (_potD <= 115){ _potD = 115; }
// 	  		if (_potD >= 270){ _potD = 270; } 
// 	    	writeDataRef(_potD,"sim/cockpit2/engine/actuators/prop_rotation_speed_rad_sec_all");
// 	    	_sensorValueSaveD = _sensorValueD;
// 		}
// 	  if ((_sensorValueE < _sensorValueSaveE-_correcaoPot) || (_sensorValueE > _sensorValueSaveE+_correcaoPot)){
// 	  		int value = map(_sensorValueE,90,330,0,511);
// 	  		_potE = (float)value/512;
// 	  		if (_potE <= 0){ _potE = 0.0; }
// 	  		if (_potE >= 1){ _potE = 1.0; } 
// 	    	writeDataRef(_potE,"sim/flightmodel/engine/ENGN_mixt[0]");
// 	    	writeDataRef(_potE,"sim/flightmodel/engine/ENGN_mixt[1]");
// 	    	_sensorValueSaveE = _sensorValueE;
// 		}
// 	  if ((_sensorValueF < _sensorValueSaveF-_correcaoPot) || (_sensorValueF > _sensorValueSaveF+_correcaoPot)){
// 	  		int value = map(_sensorValueF,90,330,511,0);
// 	  		_potF = (float)value/512;
// 	  		if (_potF <= 0){ _potF = 0.0; }
// 	  		else if (_potF > 0 && _potF <= 0.125){ _potF = 0.125; }
// 	  		else if (_potF > 0.125 && _potF <= 0.250){ _potF = 0.250; }
// 	  		else if (_potF > 0.250 && _potF <= 0.375){ _potF = 0.375; }
// 	  		else if (_potF > 0.375 && _potF <= 0.500){ _potF = 0.500; }
// 	  		else if (_potF > 0.500 && _potF <= 0.625){ _potF = 0.625; }
// 	  		else if (_potF > 0.625 && _potF <= 0.750){ _potF = 0.750; }
// 	  		else if (_potF > 0.750 && _potF <= 0.875){ _potF = 0.875; }
// 	  		else { _potF = 1.0; }  
// 	    	writeDataRef(_potF,"sim/cockpit2/controls/flap_ratio");
// 	    	_sensorValueSaveF = _sensorValueF;
// 		}
// 	}
// }

// método para enviar as datarefs e valores no simulador.
void writeDataRef(float value, String dataref) {
  if(_semaphore){
	_semaphore = false;
  _longBuffer = 509;
  memset (_bufferEnvio, 0, sizeof(505));
  memcpy (&_bufferEnvio, "DREF\0", 5);
  memcpy (&_bufferEnvio[5], &value, 4);
  for (int x = 0;x < _longBuffer; x++) {    
    if (x < dataref.length()){
    	_bufferEnvio[9+x] = dataref[x];
    } else if (x == dataref.length()){  		
    	_bufferEnvio[9+x] = '\0';
    } else{
    	_bufferEnvio[9+x] = ' ';
    }
  }    
		_Udp.beginPacket(_infoXplane.Ip, _infoXplane.port);
		_Udp.write(_bufferEnvio, _longBuffer);
		_Udp.endPacket();

	}		
	_semaphore = true;
	delay(7);
}

// método que mostra nos leds o que recebemos do X-Plane, o ID foi definido na subscrição
void dataRefOut(int ordemID, float valor){
  String titulo;
  switch (ordemID) {
    case 1: digitalWrite(_parkBrakePin, (valor == 1.0) && _canLight);
      break;
    case 2: digitalWrite(_redLeftPin, (_gear_handle_down != valor) && _canLight);
						digitalWrite(_greenLeftPin, (valor == 1.0) && _canLight);
      break;
    case 3: digitalWrite(_redNosePin, (_gear_handle_down != valor) && _canLight);
						digitalWrite(_greenNosePin, (valor == 1.0) && _canLight);
      break;
    case 4: digitalWrite(_redRightPin, (_gear_handle_down != valor) && _canLight);
						digitalWrite(_greenRightPin, (valor == 1.0) && _canLight);
      break;
    case 5: _gear_handle_down = valor;
      break;
    case 6: _bus_volts = valor;
    				_canLight = (_bus_volts > 10.0);
      break;
  }
}

// método inicial que verifica os dados e a comunicação com o simulaor.
void infoXplane(IPAddress localIp) {
  bool infoOk = false;
  _Udp.beginMulticast(ipMulticast , portMulticast);
  while (!infoOk) {
    bool received = false;

	  while (!received) {
	    _packetSize = _Udp.parsePacket();
	    if (_packetSize) {
	      _Udp.read(_packetBuffer, _packetSize);
	      received = true;
	    }
	    delay(10);
	  }	  

    memcpy (&_receivedData, &_packetBuffer, _packetSize);

  	//Verificando se as informações do XP foram recebidas.
    if (strcmp(_receivedData.comando,"BECN")==0) {
      infoOk = true;
      memcpy (&_dataBecn, &_packetBuffer, _packetSize);
      memcpy (&_infoXplane.beacon_major_version, &_dataBecn.beacon_major_version, 1);
      memcpy (&_infoXplane.beacon_minor_version, &_dataBecn.beacon_minor_version, 1);
      memcpy (&_infoXplane.application_host_id, &_dataBecn.application_host_id, 4);
      memcpy (&_infoXplane.version_number, &_dataBecn.version_number, 4);
      memcpy (&_infoXplane.role, &_dataBecn.role, 4);
      memcpy (&_infoXplane.port, &_dataBecn.port, 2);
      memcpy (&_infoXplane.computer_name, &_dataBecn.computer_name, 500);
      _infoXplane.Ip = _Udp.remoteIP();
    }
  }
  _Udp.stop(); //fechando o multicast!
}

// método para subscrevers as datarefs do XP enviando uma mensagem RREF
void subscribeDataRef(int frequency, int ordemID, String dataref) {
	  
	  memset (_bufferEnvio, 0, sizeof(_bufferEnvio));
	  memcpy (&_bufferEnvio, "RREF", 4);
	  memcpy (&_bufferEnvio[5], &frequency, sizeof(int));
	  memcpy (&_bufferEnvio[9], &ordemID, sizeof(int));
	  for (int x = 0;x < dataref.length(); x++) {
	    _bufferEnvio[13+x] = dataref[x];
	  }
	  _longBuffer = 413;

		_Udp.beginPacket(_infoXplane.Ip, _infoXplane.port);
	  _Udp.write(_bufferEnvio, _longBuffer);
	  _Udp.endPacket();
}

/*
BECN - Transmissão de beacon. Não há recurso equivalente para isso no ExtPlane. 
Toda instância em execução do X-Plane transmitirá seu endereço IP e número da porta de comando na sua LAN em uma mensagem BECN, a cada segundo. 
Com isso, é possível que os dispositivos na sua LAN "descubram" automaticamente o IP e o número da porta do servidor X-Plane. 
Também é possível detectar automaticamente se existem vários servidores X-Plane em execução na sua LAN e oferecer ao usuário a escolha de qual deles eles desejam que o dispositivo se conecte. 
É muito melhor do que o usuário configurar manualmente o dispositivo com o número de porta e IP do servidor e ajuda muito quando você tem mais de um PC X-Plane. 
Observe que este é um datagrama multicast, portanto, você precisará "assinar" a porta 49707 do endereço 239.255.1.1 do multicast.
RREF - Solicita assinatura para um DataRef. 
O cliente envia esta mensagem para a porta de comando do X-Plane, juntamente com uma frequência (em Hz) e um número de identificação. 
O X-Plane enviará automaticamente ao cliente uma mensagem RREF de resposta na frequência especificada, contendo o valor atual desse DataRef. 
Para cancelar a inscrição, você envia o RREF novamente com uma frequência de 0.
DREF - Defina o valor de um DataRef. Você envia um DREF junto com o valor desejado e o nome do dataref.
CMND - Pega o X-Plane para executar um comando. Você envia um CMND junto com o nome do comando que deseja chamar e o X-Plane o executa.
*/