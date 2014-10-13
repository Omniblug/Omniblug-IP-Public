/*
 OMNIBLUG IP Public - V01R00
 
 Simple job for obtain ip public in thelephone android.
 
 Circuit:
 arduino + ethernet W5100
 
 created 10 October 2014
 by Juan Manuel Hernández
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

// Configuración de direccion MAC e IP.

IPAddress ip(192,168,1,150); //IP local
int port = 8080; //Port
char serverGoogle[] = "android.googleapis.com"; //GCM server adress
char serverIPPublic[] = "icanhazip.com"; //server ip public

//

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
long wait = 900000;
long consulta = 0;


EthernetServer server(port);
  
void setup() {
    //inicializamos ip
    EEPROM.write(0, 15);
  
    // Inicia el puerto serie.
    Serial.begin(9600);
      
    // Inicia la conexión Ethernet y el servidor.
    Ethernet.begin(mac, ip);
    server.begin();
    
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
        EEPROM.write(0, 15);
        enviarIP();
        
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
      char msg[] = "{\"data\":{\"message\":\"";
      char msg1[] = "\"},\"registration_ids\":[\"";
      char msg2[] = "\"]}"; 
      
      //creamos msg
      String message = msg+ipPublic+msg1+getRegID()+msg2; 
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
