#define BLYNK_PRINT Serial  
#define BUILTIN_LED D0        // Led interno do Nodemcu.
#define DOUT  D1              // Porta digital de output serial do componente HX711.
#define CLK  D2               // Porta do clock do input serial.
#define GREEN_LED D3          // Porta digital do LED Verde.
#define VIRTUAL_PORT_1 V1     // Portal virtual 1 do Blynk
#define VIRTUAL_PORT_2 V2     // Portal virtual 2 do Blynk
#define VIRTUAL_PORT_3 V3     // Portal virtual 3 do Blynk
#define VIRTUAL_PORT_4 V4     // Portal virtual 4 do Blynk
#define VIRTUAL_PORT_4 V5     // Portal virtual 5 do Blynk

#include <HX711.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

HX711 scale(DOUT, CLK); // Criação do objeto que representa a balança.

char ssid[] = "nome_da_rede";                             // Nome da rede wifi em que o Nodemcu será conectado.
char pswd[] = "senha_da_rede";                            // Senha da rede wifi em que o Nodemcu será conectado.
char auth[] = "ws_cJB9BgAMi-BZhx9j-I82vsXu3eyQv";         // Token de autenticação do aplicativo da Blynk que permite a conexão do Nodemcu ao servidor.

float calibration_factor = 22375.00;      // Fator de calibragem da balança.
float current_weight = 0;                 // Variável que guarda o peso atual da balança.
int rounded_weight = 0;                   // Variável que guarda o peso em gramas.
bool notifyUser = 0;                      // Variável que serve para checar se o aplicativo enviará uma notificação ou não.
bool isDirty = 0;                         // Variável que serve para checar se o lcd precisa ser limpado.

BlynkTimer timer;               // Timer do Blynk.
WidgetLCD lcd(VIRTUAL_PORT_3);  // LCD do Blynk.
WidgetLED led(VIRTUAL_PORT_2);  // LED do Blynk.

void blinkLED(int port, int delay_time);                             // Função que faz o LED interno piscar. Serve puramente para debug.
bool checkWeight(int weight, int threshold, int port);               // Função que checa se o peso está acima do limiar definido.
void send_notification(bool notifyUser);                             // Função que envia uma notificação ao usuário.
void send_weight();                                                  // Função que envia o peso atual ao aplicativo.
void print_info();                                                   // Função que imprime as informações do peso atual e do peso arrendodado no monitor serial.
void clear_lcd();                                                    // Função que limpa o LCD dependendo da variável isDirty.

BLYNK_WRITE(V4)      // Função que checa a borda de subida da porta virtual 5 e diminui o fator de alibragem em 100
{
  int pinValue = param.asInt();
  Serial.print("Pin value (V4): ");Serial.println(pinValue);
  calibration_factor -= 100;
}

BLYNK_WRITE(V5)      // Função que checa a borda de subida da porta virtual 5 e aumenta o fator de alibragem em 100
{
  int pinValue = param.asInt();
  Serial.print("Pin value (V5): ");Serial.println(pinValue);
  calibration_factor += 100;
}

void setup() {
  pinMode (BUILTIN_LED, OUTPUT);
  pinMode (GREEN_LED, OUTPUT);
  
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pswd); // Conexão do Nodemcu com o servidor da nuvem da Blynk.

  scale.set_scale();             // Função que seta o fator de calibragem da balança com o fator de calibragem padrão da classe.
  scale.tare();                  // Reseta o valor da balança para zero.

  timer.setInterval(100L, send_weight);
  timer.setInterval(10000L, clear_lcd);
}

void loop() {
  Blynk.run();  // Função que mantém as rotinas da biblioteca rodando.
  timer.run();  // Função que executa um ciclo do relógio interno do Nodemcu através da biblioteca do Blynk.

  scale.set_scale(calibration_factor);            // Função que seta o fator de calibragem da balança com o valor correto.
  current_weight = scale.get_units();             // A função get_units() retorna o valor atual da balança já convertido para kg e ele é guardado na variável current_weight.
  
  rounded_weight = current_weight * 1000;         
  rounded_weight = round(abs(rounded_weight));         
  notifyUser = checkWeight(rounded_weight, 1000, GREEN_LED);
  
  send_notification(notifyUser);      

  print_info();
  
  blinkLED (BUILTIN_LED, 100); 
}

void blinkLED (int port, int delay_time) {
  digitalWrite(port, LOW);
  delay(delay_time);

  digitalWrite(port, HIGH);
  delay(delay_time);
}

bool checkWeight(int weight, int threshold, int port){
  if (weight >= threshold){
    digitalWrite(port, HIGH);
    led.on();
    return true;
  }
  
  digitalWrite(port, LOW);
  led.off();
  return false;
}

void send_notification(bool notifyUser){
  if (notifyUser){
    Blynk.notify("Ei, tem alguém no seu sensor! É bom dar uma olhada.");
    lcd.clear(); 
    lcd.print(0, 0, "Individuo"); 
    lcd.print(0, 1, "Detectado!");
    isDirty = true;
  }
}

void send_weight(){
  Blynk.virtualWrite(VIRTUAL_PORT_1, rounded_weight); // Envia o valor do peso atual em gramas para a porta virtual 1 do aplicativo Blynk.
}

void clear_lcd(){
  if (isDirty){
    lcd.clear();
    isDirty = false;
  }
}

void print_info(){
  Serial.print("Peso atual: "); Serial.print(current_weight, 4);Serial.println(" kg");
  Serial.print("Peso arredondado: "); Serial.print(rounded_weight);Serial.println(" g");
  Serial.print("Fator de calibragem: "); Serial.print(calibration_factor);Serial.println();
  Serial.println("############################################################");
}
