#include "SIM900.h"
#include <SoftwareSerial.h>

#include "sms.h"
#include "call.h"

SMSGSM sms;
CallGSM call;


/*Пины 2 и 3 для Подключения GSM модуля*/

boolean alarmState = false; 					     // Переключатель состояния сигнализации : true - включена, false - выключена
boolean gsmState = false; 						     // Состояние GSM модуля
boolean powerState = true;						     // Переключатель для оповещения об отключ/включ. электричества
char remoteNumber[] = "+380931111111";			     // Контактный номер для управления сигнализацией и отправки оповещений

const char AlarmMessage[] = "Alarm"; 			     // Сообщение о тревоге
const char PowerOffMessage[] = "Power Lost"; 	     // Сообщение о пропаже электричества
const char PowerOnMessage[] = "Power Back"; 	     // Сообщение о появлении электричества
const char AlarmOffMessage[] = "Alarm Disabled";     // Сообщение о выключении сигнализации
const char AlarmOnMessage[] = "Alarm Enabled"; 	     // Сообщение о включении сигнализации
const char PhoneChangeMessage[] = "Phone Changed";   // Сообщение о смене номера для управления сигнализацией, отправки сообщений и звонков
const String PhoneChangePass = "08022017";             // Команда привязки номера для управления сигнализацией, отправки сообщений и звонков
const String AlarmOffCommand = "0";                  // Команда снятия с охраны
const String AlarmOnCommand = "1";                   // Команда постановки на охрану
const int phoneNumbSize = 13; 						 // Количество символов в номере телефона
const int LedPin = 4; 						    	 // Выход 2 на LED
const int RelayPin = 12; 					    	 // Пин для реле оповещения об отключении электричества
const int SensorPin = 5;						     // Пин для датчика вибрации
const int GsmModuleSpeed = 4800;				     // Скорость передачи данных GSM-модуля
const int SerialSpeed = 19200;					     // Скорость передачи данных последовательного порта
const int SmsBufferSize = 160;					     // Размер буффера SMS-сообщения
const String newLine = "\n";
//const String CheckBalanceCommand = "3";

void setup()
{

  Serial.begin(SerialSpeed); 					 // Запуск последовательного порта
  pinMode(LedPin, OUTPUT);
  pinMode(SensorPin, INPUT);
  pinMode(RelayPin, INPUT);

  //pinMode(6, INPUT);
  //pinMode(7, INPUT);
  //pinMode(8, INPUT);
  //pinMode(9, INPUT);
  //pinMode(10, INPUT);
  //pinMode(11, INPUT);

  if (gsm.begin(GsmModuleSpeed)) 				 // Старт GSM-модуля
  {
    Serial.println("\nStatus=READY");
    gsmState = true;
  }
  else
  {
    Serial.println("\nStatus=IDLE");
  }
}

void loop()
{
  CheckSms();

  CheckSensor();

  CheckRelay();
}

void CheckSms()  	 					    	 						// Проверяем, не пришло ли SMS-сообщение
{
  char receivedSms = sms.IsSMSPresent(SMS_UNREAD); 						// Смотрим, есть ли непрочитанные SMS-сообщения

  if (receivedSms) 								 						// Если непрочитанные SMS-сообщения  есть, то...
  {
    char smsNumber[phoneNumbSize]; 										// Переменная для хранения неотформатированного номера с SMS-сообщения

    String smsContent = GetSmsContent(receivedSms,smsNumber); 			// Получаем содержимое непрочитанного SMS-сообщения
    
    char smsPhoneNumber[phoneNumbSize];                   // Переменная для хранения номера с SMS-сообщения
    
    memcpy(smsPhoneNumber, smsNumber, sizeof(smsNumber));
    
    boolean phoneNumber = CheckPhoneNumber(smsPhoneNumber); 					// Проверяем номер телефона для управления сигнализацией
//    Serial.print("SMS Text : ");
//    Serial.print(smsContent);
//    Serial.print(newLine);
    if (phoneNumber) 													// Если совпадает
    {
      SwitchAlarm(smsContent); 											// Передаем его функции переключения состояния сигнализации
    }
    else if (smsContent == PhoneChangePass && alarmState == false) 		// Иначе, проверяем не пришел ли в SMS-сообщении пароль для смены номера
    { 																                              	// при этом, сигнализация должна быть отключена
      memcpy(remoteNumber, smsPhoneNumber, sizeof(smsPhoneNumber)); 				// Если пароль пришел, меняем номер телефона для управления сигнализацией на номер с SMS-сообщения, содержащее пароль
      Serial.print(PhoneChangeMessage);
//      Serial.print(newLine);
      sms.SendSMS(remoteNumber, PhoneChangeMessage);
    }

    sms.DeleteSMS(receivedSms);					    					// Удаляем SMS-сообщение из SIM-карты
  }
}

String GetSmsContent(char receivedSms, char smsNumber[]) 				// Получаем содержимое SMS-сообщения
{
  char smsBuffer[SmsBufferSize];
  sms.GetSMS(receivedSms, smsNumber, smsBuffer, sizeof(smsBuffer));
    
  return String(smsBuffer);
}

boolean CheckPhoneNumber(char smsPhone[]) 								// Проверяем номер на соответсвие с номером телефона для управления сигнализацией
{
  delay(100);

//  Serial.print(smsPhone);
//  Serial.print(newLine);

//  Serial.print(remoteNumber);
//  Serial.print(newLine);
  
  if(!(strcmp(smsPhone, remoteNumber)))
  {
//    Serial.print("True");
//      Serial.print(newLine);
    return true;
  }
//  Serial.print("False");
//    Serial.print(newLine);
  return false;
}

void SwitchAlarm(String smsContent)
{
  if (smsContent == AlarmOffCommand && alarmState == true) 				// Отключаем сигнализацию, если пришла команда отключения
  {
    Serial.print(AlarmOffMessage);
    sms.SendSMS(remoteNumber, AlarmOffMessage); 						// Отправляем SMS-сообщение об отключении сигнализации
    alarmState = false;
    digitalWrite(LedPin, LOW);
  }
  else if (smsContent == AlarmOnCommand && alarmState == false) 		// Включаем сигнализацию, если пришла команда включения
  {
    Serial.print(AlarmOnMessage);
    sms.SendSMS(remoteNumber, AlarmOnMessage); 						    // Отправляем SMS-сообщение о включении сигнализации
    alarmState = true;
  }
}

void CheckSensor() 														// Проверяем состояние датчика
{
  if (digitalRead(SensorPin) == HIGH && alarmState == true)
  {
    Serial.print("Calling : ");
    Serial.print(remoteNumber);
    call.Call(remoteNumber);											// Звоним, на заданный номер, при срабатывании датчика
    digitalWrite(LedPin, HIGH);
    Serial.print(AlarmMessage);
  }
}

void CheckRelay()  																	// Проверяем состояние реле оповещения об отключении электричества
{
  if (digitalRead(RelayPin) == LOW && alarmState == true && powerState == true)     // Если электричество пропало
  {
    powerState = false;
    Serial.print(PowerOffMessage);
    sms.SendSMS(remoteNumber, PowerOffMessage); 									// Отправляем SMS-сообщение о том, что пропало электричество
  }

  if (digitalRead(RelayPin) == HIGH && alarmState == true && powerState == false)   // Если электричество появилось
  {
    powerState = true;
    Serial.print(PowerOnMessage);
    sms.SendSMS(remoteNumber, PowerOnMessage); 										// Отправляем SMS-сообщение о том, что появилось электричество
  }
}



