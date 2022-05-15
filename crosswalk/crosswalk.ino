/*
설정된 시간에 따라 회안보도 빨간불 파란불 동작       ok
그 동작에 따라 서보모터 동작                        ok
미세먼지 센서값을 받아 네오픽셀에 표시               ok
lcd에 시간표시                                     ok
서보모터가 동작할때 부저 동작                       ok
*/
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <RTClib_Johnny.h>
#include <Adafruit_NeoPixel.h>
#include <pm2008_i2c.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27,16,2);
DS3231 rtc;
Adafruit_NeoPixel strip(8, 7, NEO_GRB + NEO_KHZ800);                //네오픽셀 8개 7번핀 설정
PM2008_I2C pm2008_i2c;                                              //빨간선 5V 검은선 GND 노란선을 GND에 연결하여 I2C통신 설정 흰선 초록선을 각각 SDA SCL 연결
Servo myservo;

int red = 2;                                                        //RGB led 붉은색 2번
int green = 3;                                                      //RGB led 녹색 3번
int blue = 4;                                                       //RGB led 푸른색 4번
int servopin = 5;                                                   //서보모터 5번
int speaker = 6;                                                    //부저 6번

int tones[] = {261, 277, 294, 311, 330, 349, 370, 392};             //4옥타브 음계 헤르츠 좌측부터 도,도#,레,레#,미,파,파#,솔 까지

boolean state = false;

char buf[20];
char timebuf[6];

void setup()
{
    Serial.begin(115200);
    lcd.init();                 //lcd 초기화
    pm2008_i2c.begin();
    rtc.begin();                //RTC 초기화
    strip.begin();              //네오픽셀 초기화
    myservo.attach(servopin);   //서보모터 설정
    for(int j = 180; j >= 0; j -= 1)
    {
        myservo.write(j);
        delay(15);
    }
    myservo.detach();           //서보모터 해제
    lcd.backlight();            //lcd 백라이트설정
    lcd.setCursor(4,0);         //lcd문자 커서 0,0
    lcd.print("Barricade");     //문자 출력
    strip.show();               //네오픽셀 표현
    strip.setBrightness(20);    // 네오픽셀의 밝기 설정(최대 255까지)
    pm2008_i2c.command();

    if (rtc.isrunning())
    {
        Serial.println("RTC is running!");
        rtc.adjust(DateTime(__DATE__, __TIME__));
    }

    pinMode(red, OUTPUT);
    pinMode(green, OUTPUT);
    pinMode(blue, OUTPUT);

    delay(1000);
}

void loop()
{
    RTCMOD();
    PMSENSOR();
}

void PMSENSOR()
{
    uint8_t ret = pm2008_i2c.read();
    uint8_t pm1p0_grade = 0;
    uint8_t pm2p5_grade = 0;
    uint8_t pm10p_grade = 0;
    uint8_t total_grade = 0;

    if (ret == 0)
    {
        //PM 1.0 수치 코드
        if(pm2008_i2c.pm1p0_grimm < 10)
        {
            pm1p0_grade = 1;
        }
        else if(pm2008_i2c.pm1p0_grimm < 15)
        {
            pm1p0_grade = 2;
        }
        else if(pm2008_i2c.pm1p0_grimm < 20)
        {
            pm1p0_grade = 3;
        }
        else
        {
            pm1p0_grade = 4;
        }

        //PM 2.5 수치 코드
        if(pm2008_i2c.pm2p5_grimm < 10)
        {
            pm2p5_grade = 1;
        }else if(pm2008_i2c.pm2p5_grimm < 15)
        {
            pm2p5_grade = 2;
        }
        else if(pm2008_i2c.pm2p5_grimm < 20)
        {
            pm2p5_grade = 3;
        }
        else
        {
            pm2p5_grade = 4;
        }

        //PM 10 수치 코드
        if(pm2008_i2c.pm10_grimm < 20)
        {
            pm10p_grade = 1;
        }
        else if(pm2008_i2c.pm10_grimm < 30)
        {
            pm10p_grade = 2;
        }
        else if(pm2008_i2c.pm10_grimm < 40)
        {
            pm10p_grade = 3;
        }
        else
        {
            pm10p_grade = 4;
        }

        //최종 값을 확인 하는 코드
        total_grade = max(pm1p0_grade, pm2p5_grade);
        total_grade = max(total_grade, pm10p_grade);

        //확인한 값으로 표현하는 코드
        switch(total_grade)
        {
            case 1:
            {
                Serial.println("Good!");
                NEORGB(8,0,125,0);                  //초록색 출력 8칸
                break;
            }
            case 2:
            {
                Serial.println("Normal!");
                NEORGB(5,0,0,125);                  //파란색 출력 5칸
                break;
            }
            case 3:
            {
                Serial.println("Bad!");
                NEORGB(2,125,0,0);                  //붉은색 출력 두칸
                break;
            }
            case 4:
            {
                Serial.println("Worst!");
                NEORGB(2,125,0,125);                //보라색 출력 두칸
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

void NEORGB(int j, int RED, int GREEN, int BLUE)        //미세먼지 led 함수
{
    strip.clear();                                      //네오픽셀 초기화
    for(int i = 0; i < j; i++)                          //몇 번 네오픽셀 색을 설정하는 부분
    {
        strip.setPixelColor(i,RED,GREEN,BLUE);          //픽셀 번호, 설정 색, 설정 색, 설정 색
    }
    strip.show();                                       //네오픽셀 표현
}

void RTCMOD()
{
    DateTime now = rtc.now();                   //현재 시간을 가져옴
    Serial.println(now.tostr(buf));             //시리얼 모니터 현재시간 확인
    sprintf(timebuf, "%02d",now.hour());        //시간 값을 가져올때 1의 자리 값은 앞에 0을 붙여서 두자리로 만듬 / 시 값을 timebuf에 저장
    lcd.setCursor(2,1);                         //두번째 자리에 커서 옮김
    lcd.print(timebuf);                         //lcd 시 표시
    lcd.setCursor(5,1);
    lcd.print(":");
    sprintf(timebuf, "%02d",now.minute());
    lcd.setCursor(7,1);
    lcd.print(timebuf);                         //lcd 분 표시
    lcd.setCursor(10,1);
    lcd.print(":");
    sprintf(timebuf, "%02d",now.second());
    lcd.setCursor(12,1);
    lcd.print(timebuf);                         //lcd 분 표시

    if(now.minute() == 0 || now.minute() == 1 || now.minute() == 2 || now.minute() == 3)                //0~3분 빨간불 신호
    {
        if(now.minute() == 0 && now.second() == 0)                  //0분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 5 || now.minute() == 6 || now.minute() == 7 || now.minute() == 8)            //5~8분 빨간불 신호
    {
        if(now.minute() == 5 && now.second() == 0)                  //5분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 10 || now.minute() == 11 || now.minute() == 12 || now.minute() == 13)            //10~13분 빨간불 신호
    {
        if(now.minute() == 10 && now.second() == 0)                  //10분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 15 || now.minute() == 16 || now.minute() == 17 || now.minute() == 18)            //15~18분 빨간불 신호
    {
        if(now.minute() == 15 && now.second() == 0)                  //15분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 20 || now.minute() == 21 || now.minute() == 22 || now.minute() == 23)            //20~23분 빨간불 신호
    {
        if(now.minute() == 20 && now.second() == 0)                  //20분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 25 || now.minute() == 26 || now.minute() == 27 || now.minute() == 28)            //25~28분 빨간불 신호
    {
        if(now.minute() == 25 && now.second() == 0)                  //25분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 30 || now.minute() == 31 || now.minute() == 32 || now.minute() == 33)            //30~33분 빨간불 신호
    {
        if(now.minute() == 30 && now.second() == 0)                  //30분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 35 || now.minute() == 36 || now.minute() == 37 || now.minute() == 38)            //35~38분 빨간불 신호
    {
        if(now.minute() == 35 && now.second() == 0)                  //35분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 40 || now.minute() == 41 || now.minute() == 42 || now.minute() == 43)            //40~43분 빨간불 신호
    {
        if(now.minute() == 40 && now.second() == 0)                  //40분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 45 || now.minute() == 46 || now.minute() == 47 || now.minute() == 48)            //45~48분 빨간불 신호
    {
        if(now.minute() == 45 && now.second() == 0)                  //45분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 50 || now.minute() == 51 || now.minute() == 52 || now.minute() == 53)            //50~53분 빨간불 신호
    {
        if(now.minute() == 50 && now.second() == 0)                  //50분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else if(now.minute() == 55 || now.minute() == 56 || now.minute() == 57 || now.minute() == 58)            //50~58분 빨간불 신호
    {
        if(now.minute() == 55 && now.second() == 0)                  //55분0초 일때 바리게이트 올림
            GATEREVERSE();
        RGBLED(1,0,0);
    }
    else
    {
        GATE();                     //바리게이트 내림
        RGBLED(0,0,1);              //사이 1분 파란불 신호 
    }
}

void GATE()                         //바리게이트 내리는 함수
{
    if(state == true)
    {
        myservo.attach(servopin);               //서보모터 설정
        for(int i = 0; i <= 180; i += 1)        //정방향으로 180도 회전 부분
        {
            myservo.write(i);
            BUZZ();                             //부저 동작
            delay(15);
        }
        myservo.detach();                       //서보모터 해제
        state = false;
    }
}

void GATEREVERSE()                              //바리게이트 올리는 함수
{
    if(state == false)
    {
        myservo.attach(servopin);               //서보모터 설정
        for(int j = 180; j >= 0; j -= 1)        //역방향으로 180도 회전
        {
            myservo.write(j);
            BUZZ();                             //부저 동작
            delay(15);
        }
        myservo.detach();                       //서보모터 해제
        state = true;
    }
}

void BUZZ()                                     //부저 함수
{
    tone(speaker, tones[7],500);                //설정된 핀을 지정한 진동수로 500ms동안 지속합니다.
}

void RGBLED(int RED, int GREEN, int BLUE)       //신호등 설정 함수
{
    digitalWrite(red, RED);
    digitalWrite(green, GREEN);
    digitalWrite(blue, BLUE);
}