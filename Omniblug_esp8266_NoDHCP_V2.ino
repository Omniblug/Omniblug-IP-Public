/*
 OMNIBLUG IP Public - V02R00
 
 Simple job for obtain ip public in thelephone android.
 
 Circuit:
 ESP8266 + APP Omniblug Home IP
 
 Created 20 Julio 2018
 by Juan Manuel Hernández Ramírez
*/

#include <ESP8266WiFi.h>
#include <EEPROM.h>

const char* ssid = "xxx";
const char* password = "xxx";

// Configuración de red.
IPAddress ip(192,168,1,150);      // IP local
IPAddress gateway(192,168,1,1);   // Puerta enlace
IPAddress subnet(255,255,255,0);  // Subnet
IPAddress dnss1(8,8,8,8);         // DNS 1

char serverGoogle[] = "fcm.googleapis.com"; // FCM server adress
char serverIPPublic[] = "icanhazip.com";    // Server ip public

int port = 8080; // Port

long previousMillis = 0;        
long intervalOff = 900000; 
boolean firstOne = true;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(port);

void setup() {
  Serial.begin(9600);
  delay(10);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.config(ip,gateway,subnet,dnss1);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  EEPROM.begin(512);
  // Inicializamos IP
  EEPROM.write(0, 15);
  EEPROM.commit();
}

void loop() {
  unsigned long currentMillis = millis();
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    if((currentMillis - previousMillis > intervalOff) || firstOne) {
         previousMillis = currentMillis;
         firstOne = false;   
         enviarIP();
    } 
    delay(1);
    return;
  }


  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  // Read the first line of the request
  char accion = client.read();
  Serial.println(accion);
  
  
  switch (accion) {
    case 'p':{
        // PROGRAMER
        int pos=20;
        char c; 
        while (c!='\n') {
            c = client.read();
            Serial.print(c);
            EEPROM.write(pos, c);
            if (c == '\n') {
              client.write("FIN");
              break;
            }
            pos++;
          }
        
        client.flush();
        client.stop();
       
        EEPROM.write(19,pos-59); // máx 256 bytes
        EEPROM.write(0, 15);
        enviarIP();
        EEPROM.commit();
        break;
    }
    case '2':{
      // do something when var equals 2
      client.stop();
      break;
    }
    default: {
      // if nothing else matches, do the default
      // default is optional
      client.flush();
      client.stop();
    }
      
    }
    
  
}

/*************METODOS*************/
//enviar IP GCM
void enviarIP(){
      String ipPublica = getIP(serverIPPublic);
      if(!ipPublica.equals("")){
          if(compruebaIP(ipPublica)){
            Serial.println("Enviando mensaje FCM");
            sendGCM(serverGoogle, ipPublica);
          } 
      }
}

// Comprueba si ha cambiado la ip pública.
boolean compruebaIP(String ipPublic){
  // comprobamos ip con la de la eeprom si es distinta guardamos y return true
  // 111.111.111.111->15 bytes pos 0->tam ip
  int tamIp = EEPROM.read(0);
  String ipAntigua;
  for (int i = 1; i <= tamIp; i++)
    ipAntigua.concat((char)EEPROM.read(i));
  
  if(!ipPublic.equals(ipAntigua)){
    int tamipp = ipPublic.length();
    char ipChar[tamipp+1]; 
    ipPublic.toCharArray(ipChar, tamipp+1);
    EEPROM.write(0, tamipp);
    for (int i = 1; i <= tamipp; i++)
      EEPROM.write(i, ipChar[i-1]);
    return true;
  }else{
      return false;
  }
}

// obtenemos ip publica del servidor icanhazip.com 
String getIP(char server[]){
  WiFiClient getIPpublic;
  String ipPublic;
  String webIP;
    if (getIPpublic.connect(server, 80)) {
          getIPpublic.println("GET / HTTP/1.1"); 
          getIPpublic.print("Host: ");
          getIPpublic.println(server);
          getIPpublic.println("Connection: keep-alive");
          getIPpublic.println();
          
          Serial.println("enviadoMsgComprobarIP");
          delay(100);
          
          while(getIPpublic.connected()){
            if(getIPpublic.available()){
              char c = getIPpublic.read();
              if(c!= 'n' && c != 'r') {
                webIP.concat(c);
              }
            } 
          }
          getIPpublic.flush(); 
          getIPpublic.stop();
           
          int desde = webIP.indexOf("\r\n\r\n") + 4;
          ipPublic = webIP.substring(desde,webIP.length()-1); 
     }
        
     return ipPublic;
}


// enviamos mensaje GCM a nuestro android
void sendGCM(char server[],String ipPublic){
   // creamos datos
        
    if(ipPublic){
       char msg[] = "{\"data\":{\"message\":\"";
      char msg1[] = "\"},\"to\":\"";
      char msg2[] = "\"}"; 
      
      // creamos msg
      String message = msg+ipPublic+msg1+getRegID()+msg2;
      Serial.println(message); 
      Serial.println(getAPIKey());
      WiFiClient gcm;
    
      if (gcm.connect(server, 80)) {
          gcm.println("POST /fcm/send HTTP/1.1"); // http POST request
          gcm.print("Host: ");
          gcm.println(server);
          gcm.println("User-Agent: Omniblug");
          gcm.println("Content-Type: application/json");
          gcm.print("Authorization:key=");
          gcm.println(getAPIKey());
          gcm.print("Content-length: "); // has to be exactly the number of characters (bytes) in the POST body
          gcm.println(message.length()); // calculate content-length    
          gcm.println();
          gcm.print(message);
          gcm.println();
          delay(100);
          gcm.stop();
      }
    }   
}

// obtener RegID
String getRegID(){
  // 19 tamRegID
  int tam = EEPROM.read(19);
  String regIDsend;
  for (int i=0; i<tam; i++){
      regIDsend.concat((char)EEPROM.read(i+59));
  }
  return regIDsend;
}

// obtener API key
String getAPIKey(){
  int tam = 39;
  String regAPIKey;
  for (int i=0; i<tam; i++){
      regAPIKey.concat((char)EEPROM.read(i+20));
  }
  return regAPIKey;
}

