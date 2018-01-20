#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h> // Скачанная библиотека для дисплея.

boolean KeyPressed=false;
unsigned long KeyTimer=0;
int CurrTemp=0;
byte ButtonR=LOW,ButtonC=LOW,ButtonL=LOW;
unsigned long MainDelayMillis = 0,FastDelayMillis=0;
Servo servo;//Подключаем библиотеку для управления сервоприводом
#define ONE_WIRE_BUS A0 //Определям вход для термометра
#define OLED_RESET 4 
Adafruit_SSD1306 display(OLED_RESET); //Создаем объект дисплей
OneWire oneWire(ONE_WIRE_BUS); //Создаем объект oneWire
int  SetTemp,ManualTemp=1,posReg=11,RememberPos=11,Delta=0,Delta2=0,CountPress=0;//Текущая температура,установленная,предыдущая,режим 1-manual 0-auto,позиция регулятора,разница температур
unsigned long Pause=10,MainTimeOut,CurrentMillis;
byte minPos=1,maxPos=17; 
DallasTemperature sensors(&oneWire);//Создаем объект термометра

void setup(void)
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  display.clearDisplay(); // Очистить буфер.
  display.setTextColor(WHITE); // Цвет текста.
  display.setTextSize(2); // Размер текста (2).
  display.setCursor(0,0); // Устанавливаем курсор в 0  0
 // Serial.begin(115200);
  //Определяем режим пинов 5,4,3
  pinMode(5,INPUT);
  pinMode(4,INPUT);
  pinMode(3,INPUT);
  
  sensors.begin(); //Стартуем термометр
  delay(100);
  
  SetTemp=EEPROM.read(0);//Читаем сохраненную температуру в EEPROM
  if(SetTemp<0 || SetTemp>80) SetTemp=18; //Если вне диапазона 0..80 тогда устанавливаем 18
  ManualTemp=EEPROM.read(4);//Читаем режим 
  Pause=EEPROM.read(12);//Считываем значение паузы
  
  if(Pause==255) Pause=10;//При первом запуске значение в ячейке будет 255, тогда устанавливаем значение по умолчанию
  MainTimeOut=Pause*1000;

  if(!ManualTemp)
  {
    servo.attach(2);
    delay(50);
    servo.write(90);
    delay(1000);
  } else
  {
    posReg=EEPROM.read(8);
    delay(100);
    setPos(posReg);
  }
  printTemp();
}

void loop(void)
{ 
  Delta=CurrTemp-SetTemp;
  //Serial.print("1 ");
  //Serial.println(Delta);
  CurrentMillis=millis();
  while(millis()-CurrentMillis<=MainTimeOut) {
    if(KeyPressed && (millis()-KeyTimer)>15000){ //Если клавиши не нажимались в течении 15сек. то записываем значения в EEPROM
      EEPROM.write(0, SetTemp);
      EEPROM.write(4, ManualTemp);
      EEPROM.write(8,posReg);
      KeyPressed=false;
    }
    
    ReadKeyTemp();
    printTemp();  
  }
  Delta2=CurrTemp-SetTemp;
  //Serial.print("2 ");
  //Serial.println(Delta2);
  if( (abs(Delta2)>=abs(Delta)) && (abs(Delta2)>2) && (ManualTemp==0) ){
    setPos(posReg-copysign(1,Delta2));
    //Serial.print("Delta2>=10");}Serial.print("2 ");
  }
  /*
   if(Delta2>=7 && Delta2<10) {
      setPos(minPos+3);//Serial.println("Delta2>=7 && Delta2<10");}
   }
   if(Delta2<=-7 && Delta2>-10) {
      setPos(maxPos-3);//Serial.println("Delta2<=-7 && Delta2<-10");}
   }*/
   if(Delta2>=10 ) { 
    setPos(minPos);//Serial.println("Delta2>=10");}
   }
   if(Delta2<=-10 ) { 
      setPos(maxPos);//Serial.println("Delta2<=-10");}
   }
}

void printTemp()
{
  GetTemp();
  display.clearDisplay();//Очищаем буфер дисплея
  display.setCursor(0,44);//Устанавливаем курсор в позицию 0 44
  CurrTemp>=0?display.print("+"):display.print(" ");//Если температура положительная печатаем +, если нет то пробел
  display.print(CurrTemp);//Выводим температуру
  display.setCursor(90,44);//Устанавливаем курсор в позицию 90 44
  SetTemp>=0?display.print("+"):display.print(" ");//Если температура положительная печатаем +, если нет то пробел
  display.print(SetTemp);//Выводим температуру
  display.setCursor(0,0);//Устанавливаем курсор в позицию 0 0
  ManualTemp?display.print("manual"):display.print("auto");//Выводим режим
  display.setCursor(90,0);//Устанавливаем курсор в позицию 90 0
  display.print(posReg);//Печатаем позицию регулятора с учетом инверсии
  display.drawRect(0, 20, 128, 20,WHITE);//Отображаем бегунок (прямоугольник)
  display.fillRect((posReg-1)*7.53, 20, 8, 20,WHITE);//Отображаем позицию в бегуноке с учетом инверсии
  display.display(); //Выводим информацию из буфера на дисплей
  //Delta=CurrTemp-SetTemp;
  return; //Возврат

}
void ReadKeyTemp()//Опрос клавиатуры
{
  //Считываем состояние клавишь
  ButtonL=digitalRead(5);
  ButtonC=digitalRead(4);
  ButtonR=digitalRead(3);
  if(ButtonL || ButtonC || ButtonR)
  {
    KeyPressed=true;
    KeyTimer=millis();
  }  
   
  if(!ManualTemp)//Если автоматический режим
  {
    if(ButtonL==HIGH && SetTemp>0)//Если нажата левая клавиша  температура больше 0 
    { 
      SetTemp--;//убавляем на единицу
    }
    if(ButtonR==HIGH && SetTemp<70)//Если нажата правая клавиша  температура меньше 30
    {
      SetTemp++;//Прибавляем на единицу
    }
  } else
  {
    //Если ручной режим
    if(ButtonL==HIGH && posReg>minPos) //Если нажата левая клавиша и позиция регулятора больше мин без инверсии
    {
      posReg--;//Убавляем на единицу
      setPos(posReg);//Устанавливае позицию
    }
    if(ButtonR==HIGH && posReg<maxPos ) //Если нажата правая клавиша и позиция регулятора меньше макс без инверсии
    { 
      posReg++;//Прибавляем на единицу
      setPos(posReg);//Устанавливае позицию
    }
  }
   
  if((ButtonC==HIGH) && ManualTemp) //Если нажата центральная клавиша и режим ручной
  {
    ManualTemp=0;//Меняем режим
  }
  else if((ButtonC==HIGH) && !ManualTemp) //Если нажата центральная клавиша и автоматический ручной
  {                                              
    ManualTemp=1;//Меняем режим
  } 
 
  if(ButtonC==HIGH) CountPress++; else CountPress=0;
  if(CountPress>=5) setDelay();//Если средняя клавиша была нажата в течении 5ти циклов то переходим к функции установки паузы
 
  ButtonR=LOW;
  ButtonC=LOW;
  ButtonL=LOW;
  return;  
}

void setPos(int Pos)//Устанавливаем позицию
{ 
  
  if(Pos<=0 ) Pos=minPos;//Если позиция меньше минимальной тогда устанавливаем минимальную
  if(Pos>17 ) Pos=maxPos;//Если позиция больше максимальной тогда устанавливаем максимальную
  if(RememberPos==Pos) return;//Проверяем не пытаемся ли мы установить туже позицию что уже установлена

  servo.attach(2);
  delay(50);
  posReg=Pos;//Устанавливаем глобальную переменную
  RememberPos=Pos;
   switch(posReg)//Установка регулятора по заранее отградуированную позицию
  {
    case 1: servo.write(0); break;;
    case 2: servo.write(20); break;
    case 3: servo.write(30); break;
    case 4: servo.write(40); break;
    case 5: servo.write(50); break;
    case 6: servo.write(60); break;
    case 7: servo.write(70); break;
    case 8: servo.write(80); break;
    case 9: servo.write(90); break;
    case 10: servo.write(100); break;
    case 11: servo.write(110); break;
    case 12: servo.write(120); break;
    case 13: servo.write(130); break;
    case 14: servo.write(140); break;
    case 15: servo.write(150); break;
    case 16: servo.write(160); break;
    case 17: servo.write(180); break;
    
  }
printTemp();
servo.detach();
return;
}

void setDelay(){
display.clearDisplay();//Очищаем буфер дисплея
display.setCursor(10,24);//Устанавливаем курсор в позицию 10 24
display.print("Pause: ");
display.print(Pause);//Выводим паузу
display.display(); //Выводим текст из буфера
delay(1000);// Делаем задержку в секунду что бы не пролететь держа кнопку

while(1){ //Бесконечный цикл опроса
  byte ButtonL=digitalRead(5);//Считываем положение кнопок
  byte ButtonC=digitalRead(4);
  byte ButtonR=digitalRead(3);
  display.clearDisplay(); //Очищаем буфер дисплея
  display.setCursor(10,24);//Устанавливаем курсор в позицию 10 24
  display.print("Pause: ");
  display.print(Pause);//Выводим паузу
  display.display();//Выводим текст из буфера
  delay(150);//Компенсируем дребезг
  //KeyPressed=false;

  if(ButtonR==HIGH ) //Если нажата клавиша вправо
    {
      Pause++; //Увеличиваем паузу
    }
  if(ButtonL==HIGH  ) //Если нажата клавиша влево
    { 
      Pause--;  //Уменьшаем паузу   
    }    
  if((ButtonC==HIGH) ) //Если нажата центральная Сохраняем паузу и выходим
    {  
      EEPROM.write(12, Pause);//Сохраняем
      MainTimeOut=Pause*1000;
      return;//Возврат из функции
    }
}
}



void GetTemp()
{
  sensors.requestTemperatures();//Делаем опрос термометров
  CurrTemp=sensors.getTempCByIndex(0);//Считываем температуру
}
