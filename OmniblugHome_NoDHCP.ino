/*
 OMNIBLUG IP Public - V01R01
 
 Simple job for obtain ip public in thelephone android.
 
 Circuit:
 arduino + ethernet W5100 + APP Omniblug Home IP
 
 Created 15 Julio 2016
 by Juan Manuel Hernández Ramírez
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

// MAC
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEC };

// Configuración W5100.
IPAddress ip(192,168,1,150); 		// IP local
IPAddress gateway(192,168,1,1); 	// Puerta enlace
IPAddress subnet(255,255,255,0);	// Subnet
IPAddress dnss(8,8,8,8);			// DNS

int port = 8080; // Port

char serverGoogle[] = "android.googleapis.com"; // GCM server adress
char serverIPPublic[] = "icanhazip.com"; // Server ip public

long wait = 900000;
long consulta = 900000;

EthernetServer server(port);
  
void setup() {
    //Tiempo espera 1' conexión internet router en caso de fallo de alimentación
    //delay(60000); //Descomentar retardo en caso que sea necesario	
	
    // Inicializamos IP
    EEPROM.write(0, 15);
  
    // Inicia el puerto serie.
    Serial.begin(9600);
      
    // Inicia la conexión Ethernet y el servidor.
    Ethernet.begin(mac, ip, dnss, gateway, subnet); 
	
    delay(1000);
    server.begin();   
}
  
void loop() {
  
  EthernetClient client = server.available();
  
  if (client) {
    // Serial.println("conexion");
    char accion = client.read();
    // Serial.println(accion);
    
    switch (accion) {
    case 'p':{
        // PROGRAMER
        int pos=20;
        while (client.connected()) {
          if (client.available()) {
            char c = client.read();
            EEPROM.write(pos, c);
            if (c == '\n') {
              client.write("FIN");
              break;
            }
            pos++;
          }
        }
        client.flush();
        client.stop();
       
        EEPROM.write(19,pos-59); // máx 256 bytes
        EEPROM.write(0, 15);
        enviarIP();
        
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
    }
      
    }
    
  }else{
    
    if(consulta == wait){
      enviarIP();
      consulta=0;
    }
    consulta++;
    
    delay(1);
    
  }  
}
    
    
    
/*************METODOS*************/
//enviar IP GCM
void enviarIP(){
      String ipPublica = getIP(serverIPPublic);
      if(!ipPublica.equals("")){
          if(compruebaIP(ipPublica)){
            // Serial.println("Enviando mensaje GCM");
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
  EthernetClient getIPpublic;
  String ipPublic;
  String webIP;
    if (getIPpublic.connect(server, 80)) {
          getIPpublic.println("GET / HTTP/1.1"); 
          getIPpublic.print("Host: ");
          getIPpublic.println(server);
          getIPpublic.println("Connection: keep-alive");
          getIPpublic.println();
          
          // Serial.println("enviadoMsgIP");
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
      char msg1[] = "\"},\"registration_ids\":[\"";
      char msg2[] = "\"]}"; 
      
      // creamos msg
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


