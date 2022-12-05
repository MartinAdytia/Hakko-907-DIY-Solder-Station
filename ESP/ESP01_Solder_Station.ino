// Load Wi-Fi library
#include <ESP8266WiFi.h>


// Replace with your network credentials
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASS";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";
String output4State = "off";

// Assign output variables to GPIO pins
const int output5 = 5;
const int output4 = 4;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//new
String inputString = "";      // a String to hold incoming data
String input_web = "";
char tempstr[4] = "000";
char targetstr[4] = "000";
char pwmstr[4] = "000";
char pwrstr[4] = "000";
bool stringComplete = false;  // whether the string is complete

void setup() {
  Serial.begin(9600);
  // Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  pinMode(output4, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, LOW);
  digitalWrite(output4, LOW);

  //new
  inputString.reserve(200);

  // Connect to Wi-Fi network with SSID and password


  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  // Print local IP address and start web server
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients
  if (stringComplete) {
    // clear the string:
    for(int i=0; i<=24; i++){
      if(inputString[i] == '~'){
        tempstr[0]=inputString[i+1];
        tempstr[1]=inputString[i+2];
        tempstr[2]=inputString[i+3];
      }
      if(inputString[i] == '#'){
        targetstr[0]=inputString[i+1];
        targetstr[1]=inputString[i+2];
        targetstr[2]=inputString[i+3];
      }
      if(inputString[i] == '^'){
        pwmstr[0]=inputString[i+1];
        pwmstr[1]=inputString[i+2];
        pwmstr[2]=inputString[i+3];
      }
      if(inputString[i] == '@'){
        pwrstr[0]=inputString[i+1];
        pwrstr[1]=inputString[i+2];
        pwrstr[2]=inputString[i+3];
      }
    }
    inputString = "";
    stringComplete = false;
  }


  if (client) {                             // If a new client connects,
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("input1=") >= 0) { //if input1 is in the header then...
              int reqEnd = header.indexOf(" HTTP/1.1");   
              input_web = "";
              for (int n = 16; n < reqEnd; n++) {  //put the input value from the header to the variable
                input_web += header[n];
              }
              for (int m = 0; m < 32; m++){
                Serial.print("~");
                Serial.print(input_web);
                Serial.println("|");
              }


            }     
            
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<meta http-equiv=\"refresh\" content=\"3\">");
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; background-color:#AEC6CF;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Praktikum SET Solder Station</h1>");
            client.println("<body><h1>Martin Adytia - 5022201173</h1>");
            client.println("<body><h1>Kadek Wirawan Suryajaya - 5022201244</h1>");
            
            // show data
            client.println("<p>Real Temp = " + String(tempstr) + "\xB0""C</p>");
            client.println("<p>Target Temp = " + String(targetstr) + "\xB0""C</p>");
            client.println("<p>PWM Status =  " + String(pwmstr) + "%</p>");
            client.println("<p>Current Power =  " + String(pwmstr) + "W</p>");

            // form
            client.println("<form action=\"/get\">");
            client.println("Temp Set (\xB0""C): <input type=\"text\" name=\"input1\">");
            client.println("<input type=\"submit\" value=\"Submit\">");
            client.println("</form><br>");

            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
  }
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
