#ifndef NTP_h
#define NTP_h

#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#ifndef LEAP_YEAR
#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ) )
#endif

class NTPUtilityClass 
{
    public:
        NTPUtilityClass();
        void begin();
        String getFormattedDate();
        String getISO8601Formatted();
        String getFormattedTime();
        void tick();
        long getEpoch();
    private:
        NTPClient _ntp;
};

extern NTPUtilityClass NTPUtility;

#endif
