#include "Arduino.h"
#include <String.h>
#include <IPaddress.h>
#include <ESP8266WiFi.h>
#include <WebSocketClient.h>

void parse (String s);

String parsed[3];
const char* ssid     = "DEFAULT";
const char* password = "1234567890";

char path[] = "/";
char hostWS[] = "";

int ledPin = 2;
WebSocketClient webSocketClient;

int cont1 = 500000;
int cont2 = 0;
String nuevo_ssid;
String nuevo_password;
String nuevo_servidor;

IPAddress host;

const String ID = "11111";
const String tipo = "Interruptor";

int Valorantiguo;
int Valornuevo;
int bandera_enviar;
int bandera_keep;
long tiempo_ahora;
long tiempo_despues;
long tiempo_keep_ahora;
long tiempo_keep_despues;
unsigned long tiempo = 300000;
String valor = "0";
String data;
String estado;
String respuesta;
void buscar_server();
void config();
void conexion_nueva();
void envio_datos();
void keepalive();
WiFiClient client;

//----------------------------------------------------------------------------
//CONFIGURACION PREDETERMINADA
void setup()
{
  Serial.begin(115200); // Se comienza la conexión a la red Wifi

  pinMode(ledPin, INPUT);

  Serial.println();
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  buscar_server();
  config();
  conexion_nueva();

  Valorantiguo = digitalRead(ledPin) + 1;
  bandera_keep = 0;
}

//---------------------------------------------------------------------------
// LOOP PRINCIPAL
void loop() {

  Valornuevo = digitalRead(ledPin);
  if (Valornuevo != Valorantiguo) {
    bandera_enviar = 0;
    Valorantiguo = Valornuevo;
  }

  if (bandera_enviar == 0) {
    webSocketClient.sendData(ID + ";" + tipo + ";" + (String)Valornuevo);
    bandera_enviar = 1;
    tiempo_ahora = millis();
  } else if (bandera_enviar == 1) {
    webSocketClient.getData(data);
    if (data == "OK") {
      bandera_enviar = 2;
    } else {
      if ((tiempo_despues - tiempo_ahora) >= 5) {
        bandera_enviar = 0;
      }
    }
    tiempo_despues = millis();
  }



  if (bandera_keep == 0) {
    webSocketClient.sendData(ID + ";" + tipo + ";" + (String)Valornuevo);
    bandera_keep = 1;
    tiempo_keep_ahora = millis();
  } else if (bandera_enviar == 1) {
    webSocketClient.getData(data);
    if (data == "OK") {
      bandera_keep = 2;
    } else {
      if ((tiempo_keep_despues - tiempo_keep_ahora) >= 5000) {
        bandera_keep = 0;
      }
    }
  }

  if ((tiempo_keep_despues - tiempo_keep_ahora) >= 300000) {
    bandera_keep = 0;
    tiempo_keep_ahora = millis();
  }
  tiempo_keep_despues = millis();
}


//CONFIGURACION INICIAL DEL DISPOSITIVO
void config() {

  Serial.print("Conectando a ");
  Serial.println(host);

  // Uso de la clase WiFiClient para crear conexion TCP
  WiFiClient client;
  const int httpPort = 80;

  if (!client.connect(host, httpPort))
  {
    Serial.println("connection failed");
    return;
  }

  // Creacion del URL request
  String url = "/config.php";
  String key = "?pass=";
  String id = "&id=";
  String valor = "&valor=";

  Serial.print("Requesting URL: ");
  Serial.println();
  Serial.print(url);

  // Se envia el request al servidor
  Serial.println();
  Serial.print(String("GET ") + url + " HTTP/1.1 \r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println();
  client.print(String("GET ") + url + " HTTP/1.1 \r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Lee todas las lineas de respuesta del servidor y las imprime en el Serial
  int i = 0;
  String s;
  while (client.available())
  {
    String line = client.readStringUntil('\r');
    if (i < 20){
      s = line;
      Serial.print(s);
    }
    i++;
  }
  
  parse(s);
  Serial.println();
  Serial.print(nuevo_ssid);
  nuevo_ssid.remove(0, 0);
  Serial.println();
  WiFi.disconnect();
  Serial.println("Conexion terminada");


}

//SEPARA LOS NUEVOS DATOS QUE RECIBE DE config.php
void parse (String s) {
  int cont = 1;
  int j = 0;
  for (int i = 1; i < s.length(); ++i) {
    if (s[i] == ';') {
      if (cont == 1) {
        nuevo_ssid = parsed[j];
      }
      if (cont == 2) {
        nuevo_password = parsed[j];
      }
      j++;
      cont++;

    } else {
      parsed[j].concat(s[i]);
    }
  }
  nuevo_servidor = parsed[2];
  Serial.println();
}

// BUSCA UN SERVIDOR
void buscar_server() {
  WiFiClient client;
  const int httpPort = 80;

  host[0] = WiFi.localIP()[0];
  host[1] = WiFi.localIP()[1];
  host[2] = WiFi.localIP()[2];
  host[3] = 1;
  Serial.println();
  Serial.println("BUSCANDO SERVIDOR");
  while (host[3] < 255) {
    if (client.connect(host, httpPort) ) {
      Serial.print("Servidor encontrado en: ");
      Serial.println(String(host[0]) + '.' + String(host[1]) + '.' + String(host[2]) + '.' + String(host[3]));
      return;
    }
    else {
      Serial.print(String(host[0]) + '.' + String(host[1]) + '.' + String(host[2]) + '.' + String(host[3]));
      Serial.println(" Conexion fallida");
    }
    host[3]++;
  }
}

//REALIZA LA NUEVA CONEXION CON LOS DATOS RECIBIDOS DE config.php
void conexion_nueva() {
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println();
  Serial.print(nuevo_ssid);
  WiFi.begin(nuevo_ssid.c_str(), nuevo_password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  // Connect to the websocket server
  if (client.connect(nuevo_servidor.c_str(), 999)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
    while (1) {
      // Hang on failure
    }
  }

  // Handshake with the server
  nuevo_servidor.toCharArray(hostWS, 15);
  webSocketClient.path = path;
  webSocketClient.host = hostWS;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {
    Serial.println("Handshake failed.");
    while (1) {
      // Hang on failure
    }
  }



}
