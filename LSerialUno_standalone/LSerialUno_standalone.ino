#include <SD.h>
#include <LiquidCrystal_I2C.h>
#define PD_SCK 2
#define DOUT 3
#define BOTAO_1 A1

/**********ADCHX611 class**************/

#define GANHO_20MV 1
#define GANHO_80MV 2    //CHANNEL B
#define GANHO_40MV 3

class ADCHX711 {
  private:
    byte data[3], kp;
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
            res = (uint32_t)0xFF << 24;                             //Se for negativo o byte mais significativo do resultado e preenchido por 1's em binario (ver complemento para 2)
          }
          res |= (uint32_t)data[0] << 16 | (uint32_t)data[1] << 8 | (uint32_t)data[2];
          return true;
        }

      }
      return false;
    }

};
/*****************************************************/


/**************************Menu class***************************/
class Menu {
  private:
    char **buff;
    int s_buff;
    int crs;

  public:
    Menu(int n_opt): s_buff{n_opt}, crs{0} {
      buff = (char**)malloc(sizeof(char*)*n_opt);
    }
    void AddOption(const char *txt) {
      buff[crs]  = txt;
      crs++;
    }
    unsigned long ShowVolume(LiquidCrystal_I2C &lcd, unsigned long range_max, int x, int y, unsigned long p) {
      lcd.setCursor(x, y);
      unsigned long k = (range_max * p) / 1024L;
      lcd.print(k);
      return k;
    }
    int ShowAction(LiquidCrystal_I2C &lcd, unsigned long opt_pot) {
      lcd.clear();
      unsigned long b = opt_pot * crs / 1024L;
      float c = opt_pot * crs / 1024.0f;
      if (b == s_buff - 1) {
        b --;
      }
      lcd.setCursor(0, 0);
      lcd.print(buff[b]);
      lcd.setCursor(0, 1);
      lcd.print(buff[b + 1]);

      if ((b + 1) - c < c - b) {
        lcd.setCursor(13, 1);
        b++;
      }
      else {
        lcd.setCursor(13, 0);
      }
      lcd.print("<-");
      return b;
    }

};
/*********************************************************/


Menu menu(3);
ADCHX711 adc;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

void setup() {
  Serial.begin(9600);
  adc.SetUpPin(PD_SCK, DOUT);
  adc.TurnOn();
  lcd.begin(16, 2);
  menu.AddOption("(1) Gravar");
  menu.AddOption("(2) Leitura");
  menu.AddOption("(3) Sobre");
}

unsigned long pot, pot2;
int selected;
void loop() {
  pot = analogRead(0);
  selected = menu.ShowAction(lcd, pot);
  if (Serial.available() > 0) {
    byte rcv = Serial.read();
    if (rcv == 'c') { //Substituir por botao
      switch (selected) {
        case 1:             //Leitura
          long int res;
          while (rcv != 'e') {
            rcv = Serial.read();
            if (adc.GetSignalNumber(1, res)) {
              lcd.clear();
              lcd.print(res);
            }
          }
          break;
        case 0:             //Gravar
          while (rcv != 'e') {
            rcv = Serial.read();
            pot = analogRead(0);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Duracao:");
            menu.ShowVolume(lcd, 101, 10, 0, pot);
            lcd.setCursor(14, 0);
            lcd.print("h");
            menu.ShowVolume(lcd, 60, 10, 1, pot);
            lcd.setCursor(13, 1);
            lcd.print("min");
            delay(100);

          }
          break;
        case 2:
          while (rcv != 'e') {
            rcv = Serial.read();
            lcd.clear();
            lcd.print("github.com/");
            lcd.setCursor(0, 1);
            lcd.print("Rafaelbvj");
            delay(100);
          }
          break;
      }
    }
  }
  delay(100);

}
