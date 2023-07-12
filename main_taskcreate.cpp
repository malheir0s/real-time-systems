#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include <esp_system.h>
#include <esp_log.h>
#include "driver/adc.h"

#include <WiFi.h>
// #include <Arduino.h>

#define NUM_LEDS 12
gpio_num_t leds[NUM_LEDS] = {GPIO_NUM_19, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23,
                             GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_25, GPIO_NUM_26,
                             GPIO_NUM_27, GPIO_NUM_15, GPIO_NUM_4, GPIO_NUM_2};

TimerHandle_t tmr, tmr2;//Cria o objeto do timer
TaskHandle_t task1_handler, task2_handler; // handlers para as tasks
    
volatile bool espera = false;
volatile int timesteps = 0;

// props dos servidores
WiFiServer server(3333);
const char* ssid = "IgorFi";
const char* password = "serrando";
///

// structs

struct Node* primeiro = NULL;
struct Node* terceiro = NULL;
struct Node* segundo = NULL;
struct Node* quarto = NULL;
//

#define NOP() asm volatile ("nop")


void IRAM_ATTR delayUS(uint32_t us)
{
    uint32_t m = (unsigned long) (esp_timer_get_time());
    if(us){
        uint32_t e = (m + us);
        if(m > e){ //overflow
            while((unsigned long) (esp_timer_get_time()) > e){
                NOP();
            }
        }
        while((unsigned long) (esp_timer_get_time()) < e){
            NOP();
        }
    }
}

void led_config()
{
    for (int i=0;i<NUM_LEDS;i++) {  
       gpio_reset_pin(leds[i]);
       gpio_set_direction(leds[i], GPIO_MODE_OUTPUT);
    }

    gpio_reset_pin(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
    gpio_reset_pin(GPIO_NUM_34);
    gpio_set_direction(GPIO_NUM_34, GPIO_MODE_INPUT);
    gpio_reset_pin(GPIO_NUM_14);
    gpio_set_direction(GPIO_NUM_14, GPIO_MODE_INPUT);
    gpio_reset_pin(GPIO_NUM_18);
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_INPUT);
    gpio_reset_pin(GPIO_NUM_35);
    gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT);    
}


void resume_task1(void*z)
{
 vTaskResume(task1_handler);     
}

void resume_task2(void*z)
{
 vTaskResume(task2_handler);   
}


struct Node {
    uint32_t delta;
    int ultimo_nivel;
    struct Node* next;
    gpio_num_t GPIO_NUM;
    int contador;
};
 

void verifica_hcsr04(void* xTimer){
//  for(;;){ 
  int contador = 0;
  uint32_t inicio = (unsigned long) (esp_timer_get_time());
  primeiro->next = segundo;
  segundo->next = terceiro;
  terceiro->next = quarto;
  quarto->next = primeiro;
  gpio_set_level(GPIO_NUM_13, 1);
  delayUS(10);
  gpio_set_level(GPIO_NUM_13, 0);
  struct Node* atual = primeiro;
  struct Node* anterior = primeiro;

  do{
    int nivel_atual = gpio_get_level(atual->GPIO_NUM);
    if (nivel_atual != atual->ultimo_nivel){
        atual->contador = atual->contador + 1;
    }
    atual->ultimo_nivel = nivel_atual;

    if (atual->contador == 2){
      uint32_t fim = (unsigned long) (esp_timer_get_time());
      atual->contador = 0;
      atual->delta = fim - inicio;

      anterior->next = atual->next;
      atual = atual->next;
      contador++;
    }else{
      anterior = atual;
      atual = atual->next;
    }
  }while(contador < 4);

  // vTaskSuspend( task1_handler ); // suspende a task1
  // vTaskSuspend(NULL);}
}

int led_ativo;
void recebe_request(void* xTimer) {
//  for(;;){ 
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    int read = 0;
    char stored_value;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println("Access-Control-Allow-Origin:*");
            client.println();
            client.println("{\"tempo_norte\": ");
            client.print(primeiro->delta);
            client.print(", ");
            client.print("\"tempo_leste\": ");
            client.print(segundo->delta);
            client.print(", ");
            client.print("\"tempo_sul\": ");
            client.print(terceiro->delta);
            client.print(", ");
            client.print("\"tempo_oeste\": ");
            client.print(quarto->delta);
            client.print("}");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        if (read == 1){
          stored_value = c;
          read = 0;

          int led_a_ativar = (int) stored_value - 97;

          if(led_a_ativar != led_ativo && led_a_ativar >=0 && led_a_ativar<=11){
            gpio_set_level(leds[led_ativo], 0);
            gpio_set_level(leds[led_a_ativar], 1);
            led_ativo = led_a_ativar;
          }
          
        }

        if(currentLine.startsWith("GET /") && c == '=' && currentLine.endsWith("?led=")){
          read = 1;
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");

    // vTaskSuspend( task2_handler ); // suspende a task2
    vTaskSuspend(NULL);
  //}
  }}


void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.print("Conectando-se a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectada.");
  Serial.println("Endereço de IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop(){
    // definindo as structs dos HC-SR04
    Serial.println("Entrou no loop");

    primeiro = (struct Node*)malloc(sizeof(struct Node));
    segundo = (struct Node*)malloc(sizeof(struct Node));
    terceiro = (struct Node*)malloc(sizeof(struct Node));
    quarto = (struct Node*)malloc(sizeof(struct Node));
    
    primeiro->GPIO_NUM = GPIO_NUM_34;
    segundo->GPIO_NUM = GPIO_NUM_35;
    terceiro->GPIO_NUM = GPIO_NUM_18;
    quarto->GPIO_NUM = GPIO_NUM_14;
    
    Serial.println("Definiu os hcsr");

    
    led_config();

    Serial.println("config os led");
    

    //
    uint8_t led_ativo = 0;
    uint32_t reading;
    // tmr = xTimerCreate("task1", pdMS_TO_TICKS(75), true, 0, resume_task1);
    // tmr2 = xTimerCreate("task2", pdMS_TO_TICKS(100), true, 0, resume_task2);

    xTaskCreate(
      verifica_hcsr04,
      "Task 1",
      1024,
      NULL,
      1,
      &task1_handler
    );
    xTaskCreate(
      recebe_request,
      "Task 1",
      1024,
      NULL,
      2,
      &task1_handler
    );
    // xTimerStart(tmr, pdMS_TO_TICKS(0));//Inicia o timer
    // xTimerStart(tmr2, pdMS_TO_TICKS(0));//Inicia o timer

    for(;;);
} 