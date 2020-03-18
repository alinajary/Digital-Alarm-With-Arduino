#include "Wire.h"
#include "LiquidCrystal.h"
#include "math.h"
#include "EEPROM.h"

#define DS1307_I2C_ADDRESS 0x68
#define PIN_MENU 2
#define AnalogPin A0
#define PIN_COMMAND_A A1                  //releu A
#define PIN_COMMAND_B A2                  //releu B
#define PIN_COMMAND_C A3                  //releu C
#define PIN_COMMAND_D 3                  //releu D
#define PIN_COMMAND_E 10                   //releu E
#define PIN_COMMAND_F 12                   //releu F
#define PIN_COMMAND_G 13                  //releu G
#define DESCHIS HIGH
#define INCHIS LOW


#define NR_ALARME 24
#define MAX_MENU_POS NR_ALARME*4+3        //trebuie sa fie NR_ALARME*4+3

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
byte menuPos=0;
byte subMenuPos = 0;
byte cursorPos = -1;
char linia1[17];
char linia2[17];
char linia3[17];
char linia[17];
boolean dataDirthy = false;
boolean oraDirthy = false;
long lastMillis=0;
boolean bAlarma = false;
int nextday = 0;

typedef struct alarm{
   byte hour;
   byte minute;
   byte days;
   byte duration; //era byte
   byte relee;
   byte unitAlarma;
};

typedef struct ds1307_time{
   byte second;
   byte minute;
   byte hour;
   byte dayOfWeek;
   byte dayOfMonth;
   byte month;
   byte year;
};

struct ds1307_time time;

alarm* alarme = (alarm*) malloc(sizeof(alarm) * NR_ALARME);

byte relee[]={PIN_COMMAND_G, PIN_COMMAND_F,PIN_COMMAND_E,PIN_COMMAND_D,PIN_COMMAND_C,PIN_COMMAND_B,PIN_COMMAND_A};

byte decToBcd(byte val){
   return ((val/10*16)+(val%10));
}

byte bcdToDec(byte val){
   return ((val/16*10)+(val%16));
}

void setDateDs1307(struct ds1307_time time)
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(time.second));
  Wire.write(decToBcd(time.minute));
  Wire.write(decToBcd(time.hour));

  Wire.write(decToBcd(time.dayOfWeek));
  Wire.write(decToBcd(time.dayOfMonth));
  Wire.write(decToBcd(time.month));
  Wire.write(decToBcd(time.year));

  Wire.endTransmission();

}

void getDateDs1307(struct ds1307_time *time)
{
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   Wire.write(0);
   Wire.endTransmission();

   Wire.requestFrom(DS1307_I2C_ADDRESS,7);

   (*time).second = bcdToDec(Wire.read() & 0x7f);
   (*time).minute = bcdToDec(Wire.read());
   (*time).hour   = bcdToDec(Wire.read() & 0x3f);
   (*time).dayOfWeek = bcdToDec(Wire.read());
   (*time).dayOfMonth = bcdToDec(Wire.read());
   (*time).month  = bcdToDec(Wire.read());
   (*time).year   = bcdToDec(Wire.read());
}


void showMenu(){
  char *zile[7];
  zile[0]="M\0";
  zile[1]="T\0";
  zile[2]="W\0";
  zile[3]="T\0";
  zile[4]="F\0";
  zile[5]="S\0";
  zile[6]="S\0";
  if(menuPos==1){
    strcpy(linia1,"Date\0");
    strcpy(linia2,getDate(time));
    strcat(linia2," ");
    strcat(linia2,getZiuaSaptamaniiShort(time.dayOfWeek));
  }else if(menuPos==2){
    strcpy(linia1,"Time\0");
    strcpy(linia2,getTime(time));
  }else if((menuPos+1)%4==0){  //incepand cu 3, din 4 in 4 avem ora alarmei
    strcpy(linia1,"P ");
    linia1[2] = 48+(byte)((menuPos+1)/4)/10;
    linia1[3] = 48+(byte)(((menuPos+1)/4)-((menuPos+1)/4)/10*10);
    linia1[4]='\0';
    strcat(linia1," Start Time\0");
    strcpy(linia2,getFmt2Char((alarme[(menuPos+1)/4-1]).hour)); //IMPORTANT
    strcat(linia2,":");
    strcat(linia2,getFmt2Char((alarme[(menuPos+1)/4-1]).minute));
    strcat(linia2,"    ");
  }else if((menuPos)%4==0){ //incepand cu 4, din 4 in 4 avem zielele alarmei
    strcpy(linia1,"P ");
    linia1[2] = 48+(byte)((menuPos)/4)/10;
    linia1[3] = 48+(byte)((menuPos)/4-(menuPos)/4/10*10);
    linia1[4]='\0';
    strcat(linia1," Days To Run\0");
    strcpy(linia2,"M");
    for(int i=6;i>=0;i--){
      if((alarme[(menuPos)/4-1]).days & (byte)ceil(pow(2,i))){
        strcat(linia2,"X");
      }else{
        strcat(linia2," ");
      }
      if(i>0)strcat(linia2,zile[7-i]);
    }
  }else if((menuPos-1)%4==0){  //incepand cu 5, din 4 in 4 avem durata alarmei
    strcpy(linia1,"P ");
    linia1[2] = 48+(byte)((menuPos-1)/4)/10;
    linia1[3] = 48+(byte)((menuPos-1)/4-(menuPos-1)/4/10*10);
    linia1[4]='\0';
    strcat(linia1," Duration\0");
    itoa(alarme[(menuPos-1)/4-1].duration,linia2,10);
    
    if(alarme[(menuPos-1)/4-1].unitAlarma==0){
         Serial.println(alarme[(menuPos-1)/4-1].unitAlarma);
   
      strcat(linia2," sec");
    }else if(alarme[(menuPos-1)/4-1].unitAlarma==1){
         Serial.println(alarme[(menuPos-1)/4-1].unitAlarma);
      strcat(linia2," min");
 
    }
    else if(alarme[(menuPos-1)/4-1].unitAlarma==2){
         Serial.println(alarme[(menuPos-1)/4-1].unitAlarma);
           strcat(linia2," hrs");
        
    }

   
  }
  
  else if((menuPos-2)%4==0){   //starting with 6 out of 4 in 4 we have alarm relays activated
    strcpy(linia1,"P ");
    linia1[2]= 48+(byte)((menuPos-2)/4)/10;
    linia1[3]= 48+(byte)((menuPos-2)/4-(menuPos-2)/4/10*10);
    linia1[4]='\0';
    strcat(linia1," Outputs\0");
    strcpy(linia2,"");

    for(int i=3;i>=0;i--){
      char chBuff[2];
      chBuff[0]=3-i+65;
      chBuff[1]='\0';
      strcat(linia2,chBuff);

      if((alarme[(menuPos-2)/4-1]).relee & (byte)ceil(pow(2,i))){
        strcat(linia2,"X");
      }else{
        strcat(linia2," ");
      }
      //if(i>0)strcat(linia2,zile[7-i]);
    }
  }

}


//we place the cursor according to our location in sumbeniu
void showSubMenu(){
   if(menuPos==1 || menuPos==2){ //data sau ora
      switch(subMenuPos){
        case 1://zile
          cursorPos = 1;
          break;
        case 2://luni
          cursorPos = 4;
          break;
        case 3://ani
          cursorPos = 7;
          break;
        case 4://ziua saptamanii
          if(menuPos==1){//doar in cazul datei avem si ziua saptamanii
            cursorPos=12;
            break;
          }
        default:
          subMenuPos = 0;
          cursorPos = -1;
          break;
      }
   }else if((menuPos+1)%4==0){ //ora alarmelor
      switch(subMenuPos){
        case 1: //ora
          cursorPos = 1;
          break;
        case 2: //minutul
          cursorPos = 4;
          break;
        default:
          subMenuPos = 0;
          cursorPos = -1;
          break;
      }
   }else if((menuPos)%4==0){ //zilele alarmelor
     if(subMenuPos>0 && subMenuPos<8){
        cursorPos = subMenuPos*2-1;
     }else{
        subMenuPos = 0;
        cursorPos = -1;
     }
   }else if((menuPos-1)%4==0){ //durata alarmelor
     if(subMenuPos==1){
        cursorPos = 0;
     }else if(subMenuPos==2){
        cursorPos=4;
     }else{
        subMenuPos=0;
        cursorPos = -1;
     }
   }else if((menuPos-2)%4==0){ //alarm relays
     if(subMenuPos>0 && subMenuPos<5){
        cursorPos = subMenuPos*2-1;
     }else{
        subMenuPos = 0;
        cursorPos = -1;
     }
   }
}

void menu(){
  if((millis()-lastMillis)>150){
    if(digitalRead(PIN_MENU)==LOW && menuPos==0){
      menuPos=(menuPos+1)%(MAX_MENU_POS);
      showMenu();
    }else if(digitalRead(PIN_MENU)==LOW){
      subMenuPos++;
      showSubMenu();
    }
  }
  lastMillis=millis();
}

void readValues(){
  //citim valorile initiale din eeprom.
  //daca nu exista nimic scris in eeprom, consideram si orele si durata ca fiind 0 si scriem in EEPROM
  for(int i=0;i<NR_ALARME;i++){
     byte b = EEPROM.read(i*5+1);
     if(b==0xFF){
       b = 0;
       EEPROM.write(i*5+1,b);
     }
     (alarme[i]).hour = b & 31;      //pozitiile 1, 6, 11 etc.
     (alarme[i]).unitAlarma = b >> 6;

     b = EEPROM.read(i*5+2);
     if(b==0xFF){
       b = 0;
       EEPROM.write(i*5+2,b);
     }
     (alarme[i]).minute = b;    //pozitiile 2, 7, 12 etc.
     b = EEPROM.read(i*5+3);
     if(b==0xFF){
       b = 0;
       EEPROM.write(i*5+3,b);
     }
     (alarme[i]).days = b;      //pozitiile 3, 8, 13 etc.
     b = EEPROM.read(i*5+4);
     if(b==0xFF){
       b = 0;
       EEPROM.write(i*5+4,b);
     }
     (alarme[i]).duration = b;  //pozitiile 4, 9, 14 etc. //este solo determina la duración de la alarma
     b = EEPROM.read(i*5+5);
     if(b==0xFF){
       b = 0;
       EEPROM.write(i*5+5,b);
     }
     (alarme[i]).relee = b;  //pozitiile 5, 10, 15 etc.
  }
}



void setup(){

   pinMode(PIN_MENU,INPUT);    //btn stg
   
   for(int i=0;i<7;i++){
     pinMode(relee[i],OUTPUT);
     digitalWrite(relee[i],DESCHIS);
   }
   digitalWrite(PIN_MENU,HIGH);  
   attachInterrupt(0,menu,CHANGE);

   //alarmele
   readValues();

   //initializam DS_1307
   Wire.begin();
   lcd.begin(16, 2);
   Serial.begin(9600);
   struct ds1307_time time;
   time.second = 15;
   time.minute = 55;
   time.hour = 11;
   time.dayOfWeek = 5;
   time.dayOfMonth = 14;
   time.month = 3;
   time.year = 14;


   //setDateDs1307(time);

}

char* getFmt2Char(byte val){
  char *buf = "00\0";
  buf[0] = 48+val/10;
  buf[1] = 48+val%10;
  return buf;
}

char* getDate(struct ds1307_time time){
  char *buf="        \0";
  buf[0]=48+time.dayOfMonth/10;
  buf[1]=48+time.dayOfMonth%10;
  buf[2]='.';
  buf[3]=48+time.month/10;
  buf[4]=48+time.month%10;
  buf[5]='.';
  buf[6]=48+time.year/10;
  buf[7]=48+time.year%10;
  return buf;
}

char* getTime(struct ds1307_time time){
  char *buf="        \0";
  buf[0]=48+time.hour/10;
  buf[1]=48+time.hour%10;
  buf[2]=':';
  buf[3]=48+time.minute/10;
  buf[4]=48+time.minute%10;
  buf[5]=':';
  buf[6]=48+time.second/10;
  buf[7]=48+time.second%10;
  return buf;
}

char* getZiuaSaptamaniiShort(byte val){
  switch(val){
     case 1:
       return "Mon\0";   //Mon
       break;
     case 2:
       return "Tue\0";  //Tue
       break;
     case 3:
       return "Wed\0";  //Wen
       break;
     case 4:
       return "Thu\0"; //Thu
       break;
     case 5:
       return "Fri\0";  //Fri
       break;
     case 6:
       return "Sat\0";  //Sat
       break;
     case 7:
       return "Sun\0"; //Sun
       break;
     default:
       return "?\0";
       break;
  }
}

char* getZiuaSaptamanii(byte val){
  switch(val){
     case 1:
       return "Monday\0";   //replace with Monday if you want
       break;
     case 2:
       return "Tuesday\0";  //replace with Tuesday if you want
       break;
     case 3:
       return "Wed'day\0";  //replace with Wednesday if you want
       break;
     case 4:
       return "Thu'day\0";  //replace with Thursday if you want
       break;
     case 5:
       return "Friday\0"; //replace with Friday if you want
       break;
     case 6:
       return "Sat'day\0"; //replace with Saturday if you want
       break;
     case 7:
       return "Sunday\0"; //replace with Sunday if you want
       break;
     default:
       return "?\0";
       break;
  }
}

void minusData(){
   if(subMenuPos==1 && time.dayOfMonth>1){ //zilele
     time.dayOfMonth--;
   }else if(subMenuPos==2 && time.month>1){
     time.month--;
   }else if(subMenuPos==3 && time.year>1){
     time.year--;
   }else if(subMenuPos==4 && time.dayOfWeek>1){
     time.dayOfWeek--;
   }
   dataDirthy=true;
   showMenu();
}

void plusData(){
  if(subMenuPos==1){ //zilele aici utilizatorul trebuie sa aiba grija sa nu puna mai multe zile dact are luna, eu am limitat doar la 31
     time.dayOfMonth=(time.dayOfMonth+1)%32;
     if(time.dayOfMonth==0){
        time.dayOfMonth=1;
     }
   }else if(subMenuPos==2){
     time.month=(time.month+1)%13;
     if(time.month==0){
        time.month=1;
     }
   }else if(subMenuPos==3){
     time.year=(time.year+1)%99;
   }else if(subMenuPos==4){
     time.dayOfWeek=(time.dayOfWeek+1)%8;
     if(time.dayOfWeek==0){
       time.dayOfWeek=1;
     }
   }
   dataDirthy=true;
   showMenu();
}

void minusOra(){
   if(subMenuPos==1 && time.hour>0){ //zilele
     time.hour=time.hour-1;
   }else if(subMenuPos==2 && time.minute>0){
     time.minute=time.minute-1;
   }else if(subMenuPos==3 && time.second>1){
     time.second=time.second-1;
   }
   oraDirthy = true;
   showMenu();
}

void plusOra(){
  if(subMenuPos==1){ //zilele
     time.hour=(time.hour+1)%24;
   }else if(subMenuPos==2){
     time.minute=(time.minute+1)%60;
   }else if(subMenuPos==3){
     time.second=(time.second+1)%60;
   }
   oraDirthy = true;
   showMenu();
}

void minusAlarma(int pos){
   alarme[pos-1];
   if(subMenuPos==1 && alarme[pos-1].hour>0){ //zilele
     alarme[pos-1].hour--;
   }else if(subMenuPos==2 && alarme[pos-1].minute>0){
     alarme[pos-1].minute--;
   }
   showMenu();
}

void plusAlarma(byte pos){
   if(subMenuPos==1){ //zilele
     alarme[pos-1].hour=(alarme[pos-1].hour+1)%24;
   }else if(subMenuPos==2){
     alarme[pos-1].minute=(alarme[pos-1].minute+1)%60;
   }
   showMenu();
}

void minusZileAlarma(byte pos){

     Serial.println(alarme[pos-1].days);
     alarme[pos-1].days =alarme[pos-1].days ^ (byte)ceil(pow(2,(7-subMenuPos)));
     Serial.println(alarme[pos-1].days);

   showMenu();

}

void plusReleeAlarma(byte pos){
  minusReleeAlarma(pos);
}

void minusReleeAlarma(byte pos){

     Serial.println(alarme[pos-1].relee);
     alarme[pos-1].relee = alarme[pos-1].relee ^ (byte)ceil(pow(2,(7-subMenuPos)));
     Serial.println(alarme[pos-1].relee);

     showMenu();

}

void plusZileAlarma(byte pos){
  minusZileAlarma(pos);
}

void minusUnitAlarma(byte pos){
   plusUnitAlarma(pos);
}

void plusUnitAlarma(byte pos){
  if(subMenuPos==2){
    alarme[pos-1].unitAlarma=(alarme[pos-1].unitAlarma+1)%3/*2*/;  //IMPORTANT /******************************************************************************************/
/*WE CHANGED TO 3 IN ORDER TO MAKE IT MOVE INTO HRS, MIN AND SECONDS*/
    
  }
}

void minusDurataAlarma(byte pos){
   if(alarme[pos-1].duration>0 && subMenuPos==1){
      alarme[pos-1].duration--;
   }else if(subMenuPos==2){
     minusUnitAlarma(pos);
   }
   showMenu();
}

void plusDurataAlarma(byte pos){
  if(subMenuPos==1){

    switch(alarme[pos-1].unitAlarma){
    case 0:
      alarme[pos-1].duration=(alarme[pos-1].duration+1)%60;//%240;
      break;
    case 1:
      alarme[pos-1].duration=(alarme[pos-1].duration+1)%60;//%240;
      break;
    case 2:
      alarme[pos-1].duration=(alarme[pos-1].duration+1)%24;//%240;
      break;
  }
    
//     alarme[pos-1].duration=(alarme[pos-1].duration+1)%240;
     
  }else if(subMenuPos==2){
     plusUnitAlarma(pos);
   }
  showMenu();
}

void saveValues(){
  for(int i=0;i<NR_ALARME;i++){
    EEPROM.write(i*5+1,(alarme[i]).hour | (alarme[i].unitAlarma << 6));
    EEPROM.write(i*5+2,(alarme[i]).minute);
    EEPROM.write(i*5+3,(alarme[i]).days);
    EEPROM.write(i*5+4,(alarme[i]).duration);
    EEPROM.write(i*5+5,(alarme[i]).relee);
  }
}

void applyNewDateHour(){
 if(dataDirthy || oraDirthy){
    if(oraDirthy){
      Serial.println("Cu ora");
          setDateDs1307(time);
    }else{ //daca ora nu s-a modificat o lasam si noi cum este
      Serial.println("Fara ora");
      struct ds1307_time tmp;
      getDateDs1307(&tmp);
      tmp.dayOfMonth = time.dayOfMonth;
    tmp.dayOfWeek = time.dayOfWeek;
      tmp.month = time.month;
      tmp.year = time.year;
      setDateDs1307(tmp);
    }
    dataDirthy=false;
    oraDirthy = false;
  }
  saveValues();
}
long valAlarmaMin(byte index){
  return (alarme[index]).hour*3600L+(alarme[index]).minute*60L;
}
long valAlarmaMax(byte index){
// byte multipl = (alarme[index]).unitAlarma?60:1;

  int m1=0;
  int m2=0;
  long DurationValue=0;
  
  switch((alarme[index]).duration){ //we set this pattern in order to convert alarme from BYTE to LONG
    case 0:
    DurationValue=0;break;
    case 1:
    DurationValue=1;break;
    case 2:
    DurationValue=2;break;
    case 3:
    DurationValue=3;break;
    case 4:
    DurationValue=4;break;
    case 5:
    DurationValue=5;break;
    case 6:
    DurationValue=6;break;
    case 7:
    DurationValue=7;break;
    case 8:
    DurationValue=8;break;
    case 9:
    DurationValue=9;break;
    case 10:
    DurationValue=10;break;
    case 11:
    DurationValue=11;break;
    case 12:
    DurationValue=12;break;
    case 13:
    DurationValue=13;break;
    case 14:
    DurationValue=14;break;
     case 15:
    DurationValue=15;break;
    case 16:
    DurationValue=16;break;
    case 17:
    DurationValue=17;break;
    case 18:
    DurationValue=18;break;
    case 19:
    DurationValue=19;break;
    case 20:
    DurationValue=20;break;
    case 21:
    DurationValue=21;break;
    case 22:
    DurationValue=22;break;
    case 23:
    DurationValue=23;break;
     
    }
  switch(alarme[index].unitAlarma){
       
    case 0:
      m1=1;
      m2=0;
      break;
    case 1:
      m1=60;
      m2=0;
      break;
    case 2:
      m1=0;
      m2=3600;
      break;
  }
//  Serial.println(m);
//    Serial.print("  Value m1: ");
//    Serial.println(m1);
//       Serial.print("  Value m2: ");
//       Serial.println(m2);
//       Serial.print("  Value m2*duration: ");
//    Serial.println(m2*DurationValue);
//           Serial.print("DurationValue: ");
//    Serial.println(DurationValue);
//    Serial.print("  Value Minima: ");
//  Serial.println((alarme[index]).hour*3600L+(alarme[index]).minute*60L);
// Serial.print("  Value Retorno Valmax: ");
//  Serial.println((alarme[index]).hour*3600L+(alarme[index]).minute*60L+m1*(alarme[index]).duration+m2*DurationValue);

  

  return (alarme[index]).hour*3600L+(alarme[index]).minute*60L+m1*(alarme[index]).duration+m2*DurationValue;

}

   
    
void checkAlarms(){
    long valComp = time.hour*3600L+time.minute*60L+time.second;
  if(valComp < 5 || nextday == 1){
  valComp = 24L*3600L + valComp;
  nextday = 1; 
  }
 
    bAlarma=false;
    strcpy(linia,getTime(time));
    strcat(linia,";P");
    char cAl[4];
    long timeToFinish=0,vAlMax=0;
    int tipTimp; //it was boolean
    byte statusRelee[7];
    for(int i=0;i<7;i++){
      statusRelee[i]=0;
    }
    for(int i=0;i<NR_ALARME;i++){
       struct alarm alarma = alarme[i];
       int vAlMax2=1;

       vAlMax=vAlMax2*valAlarmaMax(i);
//
//       Serial.print("  Value Alarm Max ");
//    Serial.println(vAlMax);
//    Serial.print("  Value comparative: ");
//    Serial.println(valComp);
   
       if(valComp>=valAlarmaMin(i) && valComp<=vAlMax){ //comparam valorile minime si maxime ale fiecarei alarme cu timpul actual
         if((byte)ceil(pow(2,7-time.dayOfWeek)) & (alarma).days){
           bAlarma=true;
           if((timeToFinish==0 && vAlMax-valComp>timeToFinish) || (vAlMax-valComp<timeToFinish)){//setam durata pana la sf primei alarme
             tipTimp = alarma.unitAlarma;
//                 Serial.print("  TimetoFinish ");
//    Serial.println(vAlMax-valComp);
//    Serial.print("  TipTime: ");
//    Serial.println(tipTimp);
                
             if(tipTimp==1 && vAlMax-valComp>=60){ //this is for minutes
               timeToFinish=(vAlMax-valComp)/60;
               
             }else if(tipTimp==0  && vAlMax-valComp<=60){//seconds
               timeToFinish=vAlMax-valComp;
               tipTimp=0;
             }
             else if(tipTimp==2 && vAlMax-valComp>=3600){//hours
             timeToFinish=(vAlMax-valComp)/3600;
                        
             }
       else if(tipTimp==1 && vAlMax-valComp<=60){
              timeToFinish = vAlMax-valComp;
              tipTimp=0;
             }
           }
           for(int j=6;j>=0;j--){
              if((alarma).relee & (byte)(ceil(pow(2,j)))){
                statusRelee[j]=statusRelee[j] | 1;
              }
           }

           itoa((i+1),cAl,10);
           if(strlen(linia)<11){
             strcat(linia,cAl);
           }
         }
       }
    }
    for(int i=0;i<7;i++){
      if(statusRelee[i]){
        digitalWrite(relee[i],INCHIS);
      }else{
        digitalWrite(relee[i],DESCHIS);
      }
    }
    if(bAlarma){
      itoa(timeToFinish,cAl,10);
      strcat(linia," ");
      strcat(linia,cAl);
      if(tipTimp==1){
        strcat(linia,"m");
      }else if(tipTimp==0){
        strcat(linia,"s");
      }
      else if(tipTimp==2){
         strcat(linia,"h");}
    }
}

void loop(){
   //daca avem meniul apasat aratam ceea ce ne trebuie si nu mai aratam data si ora curente
   if(menuPos>0){
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print(linia1);
     lcd.setCursor(0,1);
     lcd.print(linia2);
     if(analogRead(AnalogPin)>/*380*/430 && analogRead(AnalogPin)</*555*/ 530 && menuPos){ //avem apasat minus
       delay(100);
       if(subMenuPos){
           if(menuPos==1){ //data
             minusData();
           }else if(menuPos==2){
             minusOra();
           }else if((menuPos+1)%4==0){
             minusAlarma((menuPos+1)/4);
           }else if((menuPos)%4==0){ //zile alarma
             minusZileAlarma((menuPos)/4);
           }else if((menuPos-1)%4==0){ //durata alarma
             minusDurataAlarma((menuPos-1)/4);
           }else if((menuPos-2)%4==0){ //relee alarma
             minusReleeAlarma((menuPos-2)/4);
           }
       }
       else{
          if(menuPos==1){
            applyNewDateHour();
          }
          menuPos--;
          showMenu();
       }
     }else if(analogRead(AnalogPin)<50/*50*/ && menuPos){ //avem apasat plus
       delay(200);
       if(subMenuPos){
           if(menuPos==1){ //data
             plusData();
           }else if(menuPos==2){
             plusOra();
           }else if((menuPos+1)%4==0){  //ora alarma
             plusAlarma((menuPos+1)/4);
           }else if((menuPos)%4==0){ //zile alarma
             plusZileAlarma((menuPos)/4);
           }else if((menuPos-1)%4==0){ //durata alarma
             plusDurataAlarma((menuPos-1)/4);
           }else if((menuPos-2)%4==0){ //durata alarma
             plusReleeAlarma((menuPos-2)/4);
           }
       }
       else if(menuPos<MAX_MENU_POS-1){
          ++menuPos;
          Serial.print("Menu pos: ");
          Serial.print(menuPos);
          showMenu();
       }else{
          applyNewDateHour();
          menuPos=0;
       }
     }else if(subMenuPos>0 && cursorPos>-1){

//       Serial.println(cursorPos);

       lcd.cursor();
       lcd.setCursor(cursorPos,1);
     }else{

       lcd.noCursor();

     }

     delay(100);
     return;

   }

   lcd.clear();

   char buf[3];
   getDateDs1307(&time);
   Serial.print(getTime(time));
   Serial.print(" ");
   Serial.print(getDate(time));
   Serial.print(":");
   Serial.print("Ziua saptamanii: ");
   Serial.println(time.dayOfWeek, DEC);
     Serial.print("   ");
        Serial.print("  unitAlarma ");
    Serial.println(alarme[(menuPos-1)/4-1].unitAlarma);


   lcd.setCursor(0, 0);
   lcd.print(getZiuaSaptamanii(time.dayOfWeek));
   lcd.setCursor(8,0);
   lcd.print(getDate(time));
   if(bAlarma){
     lcd.setCursor(0,1);
     lcd.print(linia);
   }else{
     lcd.setCursor(4,1);
     lcd.print(getTime(time));
     lcd.setCursor(12,1);
     lcd.print("    ");
     lcd.noCursor();
   }
   checkAlarms();
   
   delay(250);

}
