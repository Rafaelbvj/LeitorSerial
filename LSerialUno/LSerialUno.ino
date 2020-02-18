#include <SD.h>
#define PD_SCK 2
#define DOUT 3
#define PULSOS 24
const float PRECISAO = 16777216.0f; // 2^24 numeros negativos possiveis ou 2^24 - 1 numeros positivos possíveis
byte data[3];
long int resultado;
typedef struct dataconf {
  unsigned long int segs;
  unsigned long int delaytime;
  unsigned long int prec;
  unsigned long int ganho;
  char localfile[100];
} DataConf;
typedef struct data {
  unsigned char signbegin[4];
  long int dt;
  unsigned char signend[4];
} DataProtocol;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(DOUT, INPUT_PULLUP);
  pinMode(PD_SCK, OUTPUT);
  digitalWrite(PD_SCK, LOW);
}

float volts;
DataConf dc;
char br[120];
byte dts[12];
int i = 0 ;

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {

    br[i] = Serial.read();
    i++;
    if (i == sizeof(DataConf)) {
      memset(&dc, 0, sizeof(dc));
      memcpy(&dc, br, sizeof(DataConf));
      Serial.flush();
      for (DataProtocol dp;;) {

        if (digitalRead(DOUT) == LOW) {
          for (i = 0; i < 3; i++) {
            data[i] = shiftIn(DOUT, PD_SCK, MSBFIRST);
          }
          for (i = 0; i < dc.ganho; i++) {
            digitalWrite(PD_SCK, HIGH);
            delayMicroseconds(1);
            digitalWrite(PD_SCK, LOW);
            delayMicroseconds(1);
          }

          if (digitalRead(DOUT) == HIGH) {
            byte kp = data[0];                  //Salva o byte mais significativo dos dados
            if (kp >> 7 == 1) {                 //Verifica se o sinal e negativo comparando o MSB == 1
              resultado = (uint32_t)0xFF << 24; //Se for negativo o byte mais significativo do resultado e preenchido por 1's em binario (ver complemento para 2)
            }

            resultado |= (uint32_t)data[0] << 16 | (uint32_t)data[1] << 8 | (uint32_t)data[2];
            volts = 80 * (resultado / PRECISAO);
            //Serial.println(resultado);
            dp.signbegin[0] = 'B';
            dp.dt = resultado;
            dp.signend[0] = 'E';
            memset(&dts, 0, sizeof(dts));
            memcpy(&dts, &dp, sizeof(dp));
            Serial.write(dts, sizeof(dts));
          }
          resultado = 0;
        }
        delay(dc.delaytime);
        i = 0;
      }

    }
  }
}
