// Code by JeeLabs http://news.jeelabs.org/code/
#pragma once

#if (ARDUINO >= 100)
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif

#include <Wire.h>

#ifdef __AVR__
  #include <avr/pgmspace.h>
  #define WIRE Wire
#elif defined ESP8266
  #include <pgmspace.h>
  #define WIRE Wire
#elif defined ARDUINO_RASPBERRY_PI_PICO
  #include <pgmspace.h>
  #define WIRE Wire
  #define BUFFER_LENGTH WIRE_BUFFER_SIZE
#else
  #define PROGMEM
  #define pgm_read_byte(addr) (*(const unsigned char*)(addr))
  #define WIRE Wire1
#endif

#define DS1302_RAMSIZE 31 // bytes
#define DS1307_RAMSIZE 56 // bytes

#define SECONDS_PER_DAY 86400L
#define SECONDS_FROM_1970_TO_2000 946684800L


struct alarm_flags {
    char minute;
    char hour;
    char day;
    char wday;
};

// TimeDelta which can represent changes in time with seconds accuracy.
// TODO: handle negative delta
class TimeDelta {
public:
    TimeDelta(uint32_t seconds = 0, bool neg = false);
    TimeDelta(uint16_t days, int hours, int minutes, int seconds);
    TimeDelta(const TimeDelta& copy);
    uint16_t days() const { return _sec / 86400L; }
    int hours() const { return _sec / 3600 % 24; }
    int minutes() const { return _sec / 60 % 60; }
    int seconds() const { return _sec % 60; }
    int32_t totalseconds() const { return _sec; }

    bool operator==(const TimeDelta& td) const;
    bool operator!=(const TimeDelta& td) const;
    bool operator>(const TimeDelta& td) const;
    bool operator<(const TimeDelta& td) const;
    bool operator>=(const TimeDelta& td) const;
    bool operator<=(const TimeDelta& td) const;

    TimeDelta operator+(uint32_t t) const;
    TimeDelta operator+(const TimeDelta& td) const;
    TimeDelta operator-(uint32_t t) const;
    TimeDelta operator-(const TimeDelta& td) const;

    TimeDelta& operator+=(uint32_t t);
    TimeDelta& operator+=(const TimeDelta& td);
    TimeDelta& operator-=(uint32_t t);
    TimeDelta& operator-=(const TimeDelta& td);

protected:
    uint32_t _sec;
};

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    char* format(char* ret);
    char* tostr(char* charr);
    char* Hours(char* charr);
    char* Minutes(char* charr);
    DateTime(uint32_t t = 0);
    DateTime(uint16_t year, int month, int day,
             int hour = 0, int min = 0, int sec = 0);
    DateTime(const char* date, const char* time);
    DateTime(const __FlashStringHelper* date, const __FlashStringHelper* time);
    DateTime(const char* sdate); // Do we really need this?
    uint16_t year() const { return 2000 + yOff; }
    int month() const { return m; }
    int day() const { return d; }
    int hour() const { return hh; }
    int minute() const { return mm; }
    int second() const { return ss; }
    int dayOfWeek() const;
    void SetTime(const char* time);
    void SetDate(const char* date);
    void setyear(uint16_t year) { yOff = year - (year >= 2000 ? 2000 : 0); }
    void setmonth(int month) { m = month; }
    void setday(int day) { d = day; }
    void sethour(int hour) { hh = hour % 24; }
    void setminute(int minute) { mm = minute % 60; }
    void setsecond(int second) { ss = second % 60; }
    // 32-bit UNIX timestamp
    // An uint32_t should be able to store up to 2106,
    // which is beyond most chip's upper bound 2099
    void setunixtime(uint32_t t);
    uint32_t unixtime() const;

    bool operator==(const DateTime& date) const;
    bool operator==(const char* sdate) const;
    bool operator!=(const DateTime& date) const;
    bool operator!=(const char* sdate) const;
    bool operator<(const DateTime& date) const;
    bool operator>(const DateTime& date) const;
    bool operator<=(const DateTime& date) const;
    bool operator>=(const DateTime& date) const;

    DateTime operator+(uint32_t t) const;
    DateTime operator+(const TimeDelta& delta) const;
    DateTime operator-(uint32_t t) const;
    DateTime operator-(const TimeDelta& delta) const;
    TimeDelta operator-(const DateTime& date) const;

    DateTime& operator+=(uint32_t t);
    DateTime& operator+=(const TimeDelta& delta);
    DateTime& operator-=(uint32_t t);
    DateTime& operator-=(const TimeDelta& delta);

protected:
    int yOff, m, d, hh, mm, ss;
};

// RTC based on the DS1302 chip connected via pins
class DS1302 {
    // RAII class for data transferring
    class TransferHelper {
    public:
        TransferHelper(int ce_pin, int sck_pin);
        ~TransferHelper();

    protected:
        int ce, sck;

        const static int ce_to_sck_setup = 4;
        const static int ce_inactive_time = 4;
    };

    int read();
    void write(int val);

public:
    DS1302(int ce_pin = 4, int sck_pin = 5, int io_pin = 6);
    int read(int addr);
    void write(int addr, int val);

    void begin();
    int isrunning();
    DateTime now();
    void adjust(const DateTime& dt);
    int readram(int addr);
    void writeram(int addr, int val);
    int* getram(int* arr, int len);
    void putram(const int* arr, int len);

protected:
    int ce, sck, io;
};

// RTC based on the DS1307 chip connected via I2C and the Wire library
class DS1307 {
public:
    int begin();
    void adjust(const DateTime& dt);
    int isrunning();
    DateTime now();
    int read(int addr);
    void write(int addr, int val);
    int readram(int addr);
    void writeram(int addr, int val);
    int* getram(int* arr, int len);
    void putram(const int* arr, int len);
};

class DS3231 {
public:
    int begin();
    void adjust(const DateTime& dt);
    int isrunning();
    double getTemp();
    void set_alarm(const DateTime& dt); // TODO: implement DS3231 alarm
    DateTime now();
    int read(int addr);
    void write(int addr, int val);
};

class PCF8583 {
    int address;

public:
    PCF8583();
    PCF8583(int device_address);
    int begin();
    DateTime now();
    int isrunning();
    void adjust(const DateTime& dt);
    void off_alarm();
    DateTime get_alarm();
    void set_alarm(const DateTime& dt);
};

class PCF8563 {
    int address;
    int status1;
    int status2;

public:
    PCF8563();
    PCF8563(int device_address);
    int begin();
    DateTime now();
    int isrunning();
    int isvalid();
    void adjust(const DateTime& dt);
    void off_alarm();
    void on_alarm();
    DateTime get_alarm();
    void set_alarm(const DateTime& dt, alarm_flags flags);
};

// RTC using the internal millis() clock, has to be initialized before use
// NOTE: this clock won't be correct once the millis() timer rolls over (~49.7d)
// TODO: handle millis() overflow (if possible)
class RTC_Millis {
    bool running = false;

public:
    void begin();
    void begin(const DateTime& dt);
    void stop();
    void adjust(const DateTime& dt);
    DateTime now();
    bool isrunning();

protected:
    uint32_t offset;
};
