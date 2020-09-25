#include <SD.h>
#include <LiquidCrystal_I2C.h>
#define PD_SCK 2
#define DOUT 3
#define BOTAO_1 8     //Entrar
#define BOTAO_2 7     //Sair
#define POT_3 A0
#define POT_4 A1
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
    unsigned long ShowVolume(LiquidCrystal_I2C &lcd, unsigned long range_min, unsigned long range_max, int x, int y, unsigned long p) {
      lcd.setCursor(x, y);
      unsigned long k = range_min + ((range_max - range_min) * p) / 1024L;
      lcd.print(k);
      return k;
    }
    unsigned long ShowOption(LiquidCrystal_I2C &lcd, unsigned long opt_pot) {
      lcd.clear();
      unsigned long b = opt_pot * crs / 1024L;
      float c = opt_pot * crs / 1024.0f;
      if ( b == s_buff) {
        b--;
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

/************ Paginas de usuario *************/
bool Escolher_Ganho(LiquidCrystal_I2C &lcd, Menu &menu, int &gain) {
  unsigned long pot;
  int rcv = LOW;
  int sa = LOW;
  while (rcv != 'e') {
    rcv = digitalRead(BOTAO_2);
    sa = digitalRead(BOTAO_1);
    pot = analogRead(0);
    lcd.clear();
    lcd.print("Ganho:");
    gain = menu.ShowVolume(lcd, 1, 4, 7, 0, pot);
    if (sa == HIGH) {
      return true;
    }
    delay(100);
  }
  return false;
}
bool Escolher_Duracao(LiquidCrystal_I2C &lcd, Menu &menu, unsigned long &hour, unsigned long &minute) {
  unsigned long pot3, pot4;
  int rcv = LOW;
  int sa = LOW;
  while (rcv == LOW) {
    rcv = digitalRead(BOTAO_2);
    sa = digitalRead(BOTAO_1);
    pot3 = analogRead(POT_3);
    pot4 = analogRead(POT_4);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Duracao:");
    hour = menu.ShowVolume(lcd, 0, 101, 10, 0, pot3);
    lcd.setCursor(14, 0);
    lcd.print("h");
    minute = menu.ShowVolume(lcd, 0, 60, 10, 1, pot4);
    lcd.setCursor(13, 1);
    lcd.print("min");
    delay(100);
    if (sa == HIGH) {
      return true;
    }
  }
  return false;
}




Menu menu(3);
ADCHX711 adc;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

void setup() {
  Serial.begin(9600);
  pinMode(BOTAO_1, INPUT);
  pinMode(BOTAO_2, INPUT);
  adc.SetUpPin(PD_SCK, DOUT);
  adc.TurnOn();
  lcd.begin(16, 2);
  menu.AddOption("(1) Gravar");
  menu.AddOption("(2) Leitura");
  menu.AddOption("(3) Tarar");
  menu.AddOption("(4) Sobre");
}

unsigned long pot, pot2;
long int tara[3] = {0, 0, 0};                       //Tara para cada ganho

int selected;
int rcv;
void loop() {
  pot = analogRead(0);
  selected = menu.ShowOption(lcd, pot);
  rcv = digitalRead(BOTAO_1);
  if (rcv == HIGH) {       //Substituir por botao
    rcv = LOW;
    if (selected == 3) {                        //Sobre
      while (rcv == LOW) {
        rcv = digitalRead(BOTAO_2);
        lcd.clear();
        lcd.print("github.com/");
        lcd.setCursor(0, 1);
        lcd.print("Rafaelbvj");
        delay(100);
      }
    }
    else {
      int ganho;
      if (Escolher_Ganho(lcd, menu, ganho)) {
        if (selected == 2) {                    //Tarar
          while (true) {
            if (adc.GetSignalNumber(ganho, tara[ganho - 1])) {
              Serial.println(tara[ganho - 1]);
              Serial.println(ganho);
              break;
            }
          }
        }
        if (selected == 1) {                    //Leitura
          long int res;
          while (rcv == LOW) {
            rcv = digitalRead(BOTAO_2);
            if (adc.GetSignalNumber(ganho, res)) {
              lcd.clear();
              lcd.print(res - tara [ganho - 1]);
              lcd.setCursor(13, 0);
              lcd.print("bit");
              Serial.print(tara[ganho - 1]);
              Serial.print(" ");
              Serial.println(res);
            }
          }
        }

        if (selected == 0) {                    //Gravar
          unsigned long hour, minute;
          if (Escolher_Duracao(lcd, menu, hour, minute)) {
            Serial.println(hour);
            Serial.println(minute);


          }
        }


      }

    }
  }



  delay(100);

}
