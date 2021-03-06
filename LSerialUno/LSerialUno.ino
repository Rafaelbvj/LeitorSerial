#include <SD.h>
#define PD_SCK 2
#define DOUT 3
#define SERIAL_BAUDRATE 9600

long int resultado;
typedef struct dataconf {
  unsigned long int msegs;
  unsigned long int prec;
  unsigned long int ganho;
} DataConf;
typedef struct data {
  unsigned char signbegin[4];
  long int dt;
  unsigned long mtime;
  unsigned char signend[4];
} DataProtocol;


/**********ADCHX611 class**************/

#define GANHO_20MV 1
#define GANHO_80MV 2    //CHANNEL B
#define GANHO_40MV 3

class ADCHX711 {
  private:
    byte data[3],kp;
    int pd_sck_hx;
    int dout_hx;
  public:
    ADCHX711() {
      pd_sck_hx = 1;
      pd_sck_hx = 2;
    }
    ADCHX711(int pd_sck, int dout) {
      SetUpPin(pd_sck, dout);
    }
    void SetUpPin(int pd_sck, int dout) {
      pd_sck_hx = pd_sck;
      dout_hx = dout;
      pinMode(dout, INPUT_PULLUP);
      pinMode(pd_sck, OUTPUT);
    }
    void TurnOn() {                                               //Ativa o HX711
      digitalWrite(pd_sck_hx, LOW);
    }
    void TurnOff() {                                              //Desliga o HX711
      digitalWrite(pd_sck_hx, HIGH);
    }
    bool GetSignalNumber(int ganho, long int &res) {              //Verifica se há dados pronto para leitura e os lê em signed para res
      if (digitalRead(dout_hx) == LOW) {
        for (int i = 0; i < 3; i++) {
          data[i] = shiftIn(dout_hx, pd_sck_hx, MSBFIRST);
        }
        for (int i = 0; i < ganho; i++) {
          digitalWrite(pd_sck_hx, HIGH);
          delayMicroseconds(1);
          digitalWrite(pd_sck_hx, LOW);
          delayMicroseconds(1);
        }
        if (digitalRead(dout_hx) == HIGH) {
          kp = data[0];                                             //Salva o byte mais significativo dos dados
          if (kp >> 7 == 1) {                                       //Verifica se o sinal e negativo comparando o MSB == 1
            res = (uint32_t)0xFF << 24;                             //Se for negativo o byte mais significativo do resultado (4 bytes) e preenchido por 1's em binario (ver complemento para 2)
          }
          res |= (uint32_t)data[0] << 16 | (uint32_t)data[1] << 8 | (uint32_t)data[2];      //concatena os 3 bytes de dados
          return true;         
        }
        
      }
      return false;
    }

};
/*****************************************************/

ADCHX711 adc;
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  adc.SetUpPin(PD_SCK, DOUT);
  adc.TurnOn();
}

DataConf dc;
DataProtocol dp;
char br[50];
int i = 0 ;
unsigned long clock_st, clock_end;

void loop() {
  
  if (Serial.available() > 0) {
    br[i] = Serial.read();
    i++;
    if (i == sizeof(DataConf)) {
      memcpy(&dc, br, sizeof(DataConf));
      dp.signbegin[0] = 'B';        //Data signature
      dp.signend[0] = 'E';          //Data signature
      clock_st = millis();
      while (dp.mtime < dc.msegs) {
        clock_end = millis();
        if (adc.GetSignalNumber(dc.ganho, resultado)) {
          dp.mtime = clock_end-clock_st;
          dp.dt = resultado;
          Serial.write((byte*)&dp, sizeof(DataProtocol));
        }
      }
      dp.signbegin[0] = 'E';                         
      dp.signend[0] = 'D';
      Serial.write((byte*)&dp,sizeof(DataProtocol));//Finishing communication
    }
  }
}
