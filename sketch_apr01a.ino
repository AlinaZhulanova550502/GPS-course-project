#include <TinyGPS++.h>
#include <EEPROM.h>      //память, которая хранит значения переменных, пока плата выключена (подобно крошечному жесткому диску)
#include <SoftwareSerial.h>
#include <GSM.h>
 

#define TELLNUMBER "70000000000"                       // номен телефона для команды
#define SMSNUMBER "AT+CMGS=\"+70001111111\""           // номер на который будем отправлять SMS
GSM gsm;

boolean GSMSMSflag = 0;           // команда на отправку SMS

TinyGPSPlus gps; 

SoftwareSerial gsm(9, 8);         // программный UART для GSM модема
//SoftwareSerial ss(3, 4);         // программный UART для GPS приемника
#define ss Serial1                // аппаратный UART для GPS приемника

void setup()
{
  Serial.begin(9600);         
  ss.begin(9600);                // скорость GPS
  gsm.begin(9600);               // скорость GSM
  
  delay(100);     //???????????????????
  Serial.println("start");
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  pinMode(A0, INPUT_PULLUP);  // чтобы ловить смс

  
  GSMSMSflag = EEPROM.read(0);    //  достаем флаг - 1 байт с адреса 0
  
  delay(2000);            // нужно дождаться включения модема и соединения с сетью
  gsm.println("ATE0");                   // выключаем эхо  
  
  while(1){                              // ждем подключение модема к сети
        gsm.println("AT+COPS?");    // выбор и регистрация сети GSM
        if (gsm.find("+COPS: 0")) break; // автоматический выбор сети
   }
    
  gsm.println("AT+CMGF=1");            // настройки для SMS текстовый режим
  delay(100);
  gsm.println("AT+CSCS=\"GSM\""); // устанавливаем схему кодирования  
}


void loop(){
  if ((GSMSMSflag == 0)&&(gsm.find("+CMTI: «SM»"))){      // если ожидаем и пришло сообщение (+CMTI: «SM», 1)
    GSMSMSflag = 1;                     // меняем режим  
    EEPROM.write(0, GSMSMSflag);        // пишем его в еепром

    delay(500);
    gsm.println("AT+CMGDA=«DEL INBOX»"); // удаление всех полученных сообщений (кавычки??)
  }
    // работа от смс GPS
  if (GSMSMSflag == 1){                               //  если сменился режим
    do{          
      do{                                             //  в цикле забираем данные от GPS
         while (ss.available() > 0) gps.encode(ss.read());                   
      }while (!gps.location.isUpdated() && gps.location.age() > 1500);   
    } while (!gps.location.isValid());                  // пока не получили свежие данные  

  GSMsendSMS();                           // отправляем SMS
  GSMSMSflag = 0;                         // меняем режим  
  EEPROM.write(0, GSMSMSflag);            // пишем его в еепром  
  }
}

                                
void GSMsendSMS(){                          // отправка SMS
  delay(100);   
  gsm.println(SMSNUMBER);               // отправляем команду на отправку смс
  delay(100);

  gsm.print(gps.location.lat(), 6);     // передаем координаты
  gsm.print(",");
  gsm.print(gps.location.lng(), 6); 

  gsm.print(" S:");                     // передаем скорость движения
  gsm.print(gps.speed.kmph());

  gsm.print(" T:");                     // передаем время с GPS
  if (gps.time.hour() < 10) gsm.print("0");
  gsm.print(gps.time.hour());
  gsm.print(":");
  if (gps.time.minute() < 10) gsm.print(F("0"));
  gsm.print(gps.time.minute());
      
  gsm.print((char)26);                  // символ завершающий передачу
  Serial.println("ok");   
}
