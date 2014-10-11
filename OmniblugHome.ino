/*
 OMNIBLUG IP Public
 
 Simple job for obtain ip public in thelephone android.
 
 Circuit:
 * Hlk-Rm04
 
 created 13 July 2014
 by Juan Manuel Hernández

 */

#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

// Configuración de direccion MAC e IP.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,177);
int port = 8080;

long wait = 900000;
long consulta = 0;
// datos
//char API_key[] = "AIzaSyB4aePWdTMNiZWgTFNWUen7CDYGxoIMch8"; // API Key - no modify this parameter
//char RegID[] = "APA91bElbx8jBzy3qd08UWFbs5HU6kT6XM1lpQ5A7-t8QibKZIR6QKClGV3d32cQSlKRqACTOeJWKBwzzUOvrxd5xUV-Fy1wKZW6jhiUNutA7fcC0gkHZLimUFkawxPefbd22b34EfS-4ZOK8N29IfKZgIK3Tch5Nw"; // <---- Modifier this parameter for you registry. Never public.
char serverGoogle[] = "android.googleapis.com"; //GCM server adress
char serverIPPublic[] = "icanhazip.com"; //server ip public

EthernetServer server(port);
  
void setup() {
    //inicializamos ip
    EEPROM.write(0, 15);
  
    // Inicia el puerto serie.
    Serial.begin(9600);
      
    // Inicia la conexión Ethernet y el servidor.
    Ethernet.begin(mac, ip);
    server.begin();
    
    /*Serial.print("IP local del servidor ");
    Serial.println(Ethernet.localIP());
    Serial.println(EEPROM.read(19));
    Serial.println(getRegID());*/
    enviarIP();
}
  
void loop() {
  
  EthernetClient client = server.available();
  
  if (client) {
    //Serial.println("new client");
    char accion = client.read();
    //Serial.println(accion);
    
    switch (accion) {
    case 'p':{
        //PROGRAMER
        int pos=20;
        while (client.connected()) {
          if (client.available()) {
            char c = client.read();
            //Serial.print(c);
            EEPROM.write(pos, c);
            
            if (c == '\n') {
              // fin
              //Serial.println("fin");
              client.write("FIN");
              break;
            }
            pos++;
          }
        }
        EEPROM.write(19,pos-59); //máx 256 bytes
        client.stop();
        break;
    }
    case '2':{
      //do something when var equals 2
      client.stop();
      break;
    }
    default: {
      // if nothing else matches, do the default
      // default is optional
    }
      
    }
    
  }else{
    
    if(consulta == wait){
      enviarIP();
      consulta=0;
    }
    consulta++;
    //Serial.println(consulta);
    delay(1);
    
  }  
}
    
    
    
/*************METODOS*************/
void enviarIP(){
  //Serial.println("Consulta y envio");
      String ipPublica = getIP(serverIPPublic);
      if(!ipPublica.equals("")){
          if(compruebaIP(ipPublica)){
            //Serial.println("Enviando mensaje GCM");
            sendGCM(serverGoogle, ipPublica);
          } 
      }
}

// Comprueba si ha cambiado la ip pública.
boolean compruebaIP(String ipPublic){
  //comprobamos ip con la de la eeprom si es distinta guardamos y return true
  // 111.111.111.111->15 bytes pos 0->tam ip
  //leemos la ip anterior
  //Serial.println("inicio rutina");
  
  int tamIp = EEPROM.read(0);
 // Serial.println("tamano ip: ");
 // Serial.println(tamIp);
  
  String ipAntigua;
  
  for (int i = 1; i <= tamIp; i++)
    ipAntigua.concat((char)EEPROM.read(i));
    
 /* Serial.print("Ip leida eeprom:");
  Serial.print(ipAntigua);
  Serial.println("***");
  Serial.print("Ip publica:");
    Serial.print(ipPublic);
    Serial.println("***");*/
  
  if(!ipPublic.equals(ipAntigua)){
    int tamipp = ipPublic.length();
    char ipChar[tamipp+1]; 
    //Serial.println(tamipp);
    ipPublic.toCharArray(ipChar, tamipp+1);
    /*Serial.print(ipChar);
    Serial.println("es distinta");*/
    EEPROM.write(0, tamipp);
    for (int i = 1; i <= tamipp; i++)
      EEPROM.write(i, ipChar[i-1]);
    return true;
  }else{
      //Serial.println("es la misma");
      return false;
  }
}

// obtenemos ip publica del servidor icanhazip.com 
String getIP(char server[]){
  EthernetClient getIPpublic;
  String ipPublic;
  String webIP;
  int fin=0;
    if (getIPpublic.connect(server, 80)) {
          getIPpublic.println("GET / HTTP/1.1"); 
          getIPpublic.print("Host: ");
          getIPpublic.println(server);
          getIPpublic.println("Connection: keep-alive");
          getIPpublic.println();
          
          //Serial.println("enviadoMsgIP");
          delay(100);
          
          while(getIPpublic.connected() && !getIPpublic.available()) {
            delay(1);
            fin++;
            if(fin==100)break;
          }
          while (getIPpublic.available()) {
              char c = getIPpublic.read();
              webIP.concat(c);
          }
          
          //Serial.println(webIP);
          
          getIPpublic.stop();  
          int desde = webIP.indexOf("\r\n\r\n") + 4;
          ipPublic = webIP.substring(desde,webIP.length()-1); 
     }
        
     return ipPublic;
}


//enviamos mensaje GCM a nuestro android
void sendGCM(char server[],String ipPublic){
   //creamos datos
        
    if(ipPublic){
      //Serial.println(ipPublic);
      
      char msg[] = "{\"data\":{\"message\":\"";
      char msg1[] = "\"},\"registration_ids\":[\"";
      char msg2[] = "\"]}"; 
      
      //creamos msg
      String message = msg+ipPublic+msg1+getRegID()+msg2; 
      //Serial.println(message);
      EthernetClient gcm;
    
      if (gcm.connect(server, 80)) {
          gcm.println("POST /gcm/send HTTP/1.1"); // http POST request
          gcm.print("Host: ");
          gcm.println(server);
          gcm.println("User-Agent: Arduino");
          gcm.println("Content-Type: application/json");
          gcm.print("Authorization:key=");
          gcm.println(getAPIKey());
          gcm.print("Content-length: "); // has to be exactly the number of characters (bytes) in the POST body
          gcm.println(message.length()); // calculate content-length    
          gcm.println();
          gcm.print(message);
          gcm.println();
          
          delay(100);
          
          /*while(gcm.connected() && !gcm.available()) delay(1);
          
          while (gcm.available()) {
              char c = gcm.read();
              Serial.write(c);
          }*/
          gcm.stop();
      }
    }   
}

//obtener RegID
String getRegID(){
  // 19 tamRegID
  int tam = EEPROM.read(19);
  String regIDsend;
  for (int i=0; i<tam; i++){
      regIDsend.concat((char)EEPROM.read(i+59));
  }
  return regIDsend;
}

//obtener API key
String getAPIKey(){
  int tam = 39;
  String regAPIKey;
  for (int i=0; i<tam; i++){
      regAPIKey.concat((char)EEPROM.read(i+20));
  }
  return regAPIKey;
}
