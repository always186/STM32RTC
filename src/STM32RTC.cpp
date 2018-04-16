/**
  ******************************************************************************
  * @file    STM32RTC.cpp
  * @author  WI6LABS
  * @version V1.0.0
  * @date    12-December-2017
  * @brief   Provides a RTC interface for Arduino
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include <time.h>

#include "STM32RTC.h"

#define EPOCH_TIME_OFF      946684800  // This is 1st January 2000, 00:00:00 in epoch time
#define EPOCH_TIME_YEAR_OFF 100        // years since 1900

// Initialize static variable
bool STM32RTC::_configured = false;

STM32RTC::STM32RTC(void): _clockSource(RTC_LSI_CLOCK)
{

}

/**
  * @brief initializes the RTC
  * @param resetTime: if true reconfigures the RTC
  * @param format: hour format: RTC_HOUR_12 or RTC_HOUR_24(default)
  * @retval None
  */
void STM32RTC::begin(bool resetTime, RTC_Hour_Format format)
{
  if(resetTime == true) {
    _configured = false;
  }
  begin(format);
}

/**
  * @brief initializes the RTC
  * @param format: hour format: RTC_HOUR_12 or RTC_HOUR_24(default)
  * @retval None
  */
void STM32RTC::begin(RTC_Hour_Format format)
{
  if(_configured == false) {
    RTC_init((format == RTC_HOUR_12)? HOUR_FORMAT_12: HOUR_FORMAT_24,
             (_clockSource == RTC_LSE_CLOCK)? LSE_CLOCK:
             (_clockSource == RTC_HSE_CLOCK)? HSE_CLOCK : LSI_CLOCK);
    // Must be set before call of sync methods
    _configured = true;
    syncTime();
    syncDate();
    // Use current time to init alarm members
    _alarmDate  = _date;
    _alarmHours = _hours;
    _alarmMinutes = _minutes;
    _alarmSeconds = _seconds;
    _alarmSubSeconds = _subSeconds;
    _alarmPeriod = _hoursPeriod;
  } else {
    syncTime();
    syncDate();
    syncAlarmTime();
  }
}

/**
  * @brief Deinitialize and stop the RTC
  * @param None
  * @retval None
  */
void STM32RTC::end(void)
{
  if(_configured == true) {
    RTC_DeInit();
    _configured = false;
  }
}

/**
  * @brief set the RTC clock source. By default LSI clock is selected. This
  * method must be called before begin().
  * @param source: clock source: RTC_LSI_CLOCK, RTC_LSE_CLOCK or RTC_HSE_CLOCK
  * @retval None
  */
void STM32RTC::setClockSource(RTC_Source_Clock source)
{
  if(IS_CLOCK_SOURCE(source)) {
    _clockSource = source;
  }
}

/**
  * @brief enable the RTC alarm.
  * @param match: Alarm_Match configuration
  * @retval None
  */
void STM32RTC::enableAlarm(Alarm_Match match)
{
  hourAM_PM_t period;
  uint8_t date, hours, minutes, seconds;
  uint32_t subSeconds;

  if(_configured) {
    RTC_GetTime(&hours, &minutes, &seconds, &subSeconds, &period);
    date = getDay();
    switch (match) {
      case MATCH_OFF:
        RTC_StopAlarm();
        break;
      case MATCH_YYMMDDHHMMSS://kept for compatibility
      case MATCH_MMDDHHMMSS:  //kept for compatibility
      case MATCH_DHHMMSS:
        date = _alarmDate;
      case MATCH_HHMMSS:
        hours = _alarmHours;
        period = (_alarmPeriod == RTC_AM)? AM: PM;
      case MATCH_MMSS:
        minutes = _alarmMinutes;
      case MATCH_SS:
        seconds = _alarmSeconds;
        RTC_StartAlarm(date, hours, minutes, seconds, subSeconds, period);
        break;
      default:
      break;
    }
  }
}

/**
  * @brief disable the RTC alarm.
  * @retval None
  */
void STM32RTC::disableAlarm(void)
{
  if(_configured) {
    RTC_StopAlarm();
  }
}

/**
  * @brief attach a callback to the RTC alarm interrupt.
  * @param callback: pointer to the callback
  * @retval None
  */
void STM32RTC::attachInterrupt(voidFuncPtr callback, void *data)
{
  attachAlarmCallback(callback, data);
}

/**
  * @brief detach the RTC alarm callback.
  * @retval None
  */
void STM32RTC::detachInterrupt(void)
{
  detachAlarmCallback();
}

// Kept for compatibility. Use STM32LowPower library.
void STM32RTC::standbyMode(void)
{

}

/*
 * Get Functions
 */

/**
  * @brief  get RTC subseconds.
  * @retval return the current subseconds from the RTC.
  */
uint32_t STM32RTC::getSubSeconds(void)
{
  syncTime();
  return _subSeconds;
}

/**
  * @brief  get RTC seconds.
  * @retval return the current seconds from the RTC.
  */
uint8_t STM32RTC::getSeconds(void)
{
  syncTime();
  return _seconds;
}

/**
  * @brief  get RTC minutes.
  * @retval return the current minutes from the RTC.
  */
uint8_t STM32RTC::getMinutes(void)
{
  syncTime();
  return _minutes;
}

/**
  * @brief  get RTC hours.
  * @param  format: optional (default: NULL)
  *         pointer to the current hour format set in the RTC: AM or PM
  * @retval return the current hours from the RTC.
  */
uint8_t STM32RTC::getHours(RTC_AM_PM *period)
{
  syncTime();
  if(period != NULL) {
    *period = _hoursPeriod;
  }
  return _hours;
}

/**
  * @brief  get RTC week day.
  * @retval return the current week day from the RTC.
  */
uint8_t STM32RTC::getWeekDay(void)
{
  syncDate();
  return _day;
}

/**
  * @brief  get RTC date.
  * @retval return the current date from the RTC.
  */
uint8_t STM32RTC::getDay(void)
{
  syncDate();
  return _date;
}

/**
  * @brief  get RTC month.
  * @retval return the current month from the RTC.
  */
uint8_t STM32RTC::getMonth(void)
{
  syncDate();
  return _month;
}

/**
  * @brief  get RTC year.
  * @retval return the current year from the RTC.
  */
uint8_t STM32RTC::getYear(void)
{
  syncDate();
  return _year;
}

/**
  * @brief  get RTC alarm subsecond.
  * @retval return the current alarm subsecond.
  */
uint32_t STM32RTC::getAlarmSubSeconds(void)
{
  syncAlarmTime();
  return _alarmSubSeconds;
}

/**
  * @brief  get RTC alarm second.
  * @retval return the current alarm second.
  */
uint8_t STM32RTC::getAlarmSeconds(void)
{
  syncAlarmTime();
  return _alarmSeconds;
}

/**
  * @brief  get RTC alarm minute.
  * @retval return the current alarm minute.
  */
uint8_t STM32RTC::getAlarmMinutes(void)
{
  syncAlarmTime();
  return _alarmMinutes;
}

/**
  * @brief  get RTC alarm hour.
  * @param  format: optional (default: NULL)
  *         pointer to the current hour format set in the RTC: AM or PM
  * @retval return the current alarm hour.
  */
uint8_t STM32RTC::getAlarmHours(RTC_AM_PM *period)
{
  syncAlarmTime();
  if(period != NULL) {
    *period = _alarmPeriod;
  }
  return _alarmHours;
}

/**
  * @brief  get RTC alarm date.
  * @retval return the current alarm date.
  */
uint8_t STM32RTC::getAlarmDay(void)
{
  syncAlarmTime();
  return _alarmDate;
}

/**
  * @brief  get RTC alarm month.
  * @NOTE   This function is kept for compatibility but the STM32 RTC
  *         can't assign a month to an alarm. See board datasheet.
  * @retval always returns 0
  */
uint8_t STM32RTC::getAlarmMonth(void)
{
  return 0;
}

/**
  * @brief  get RTC alarm year.
  * @NOTE   This function is kept for compatibility but the STM32 RTC
  *         can't assign a year to an alarm. See board datasheet.
  * @retval always returns 0
  */
uint8_t STM32RTC::getAlarmYear(void)
{
  return 0;
}

/*
 * Set Functions
 */

/**
  * @brief  set RTC subseconds.
  * @param  subseconds: 0-999
  * @retval none
  */
void STM32RTC::setSubSeconds(uint32_t subSeconds)
{
 if (_configured) {
    syncTime();
    if(subSeconds < 1000) {
      _subSeconds = subSeconds;
    }
    RTC_SetTime(_hours, _minutes, _seconds, _subSeconds, (_hoursPeriod == RTC_AM)? AM : PM);
 }
}

/**
  * @brief  set RTC seconds.
  * @param  seconds: 0-59
  * @retval none
  */
void STM32RTC::setSeconds(uint8_t seconds)
{
  if (_configured) {
    syncTime();
    if(seconds < 60) {
      _seconds = seconds;
    }
    RTC_SetTime(_hours, _minutes, _seconds, _subSeconds, (_hoursPeriod == RTC_AM)? AM : PM);
  }
}

/**
  * @brief  set RTC minutes.
  * @param  minutes: 0-59
  * @retval none
  */
void STM32RTC::setMinutes(uint8_t minutes)
{
  if (_configured) {
    syncTime();
    if(minutes < 60) {
      _minutes = minutes;
    }
    RTC_SetTime(_hours, _minutes, _seconds, _subSeconds, (_hoursPeriod == RTC_AM)? AM : PM);
  }
}

/**
  * @brief  set RTC hours.
  * @param  hours: 0-23
  * @retval none
  */
void STM32RTC::setHours(uint8_t hours)
{
  if (_configured) {
    syncTime();
    if(hours < 24) {
      _hours = hours;
    }
    RTC_SetTime(_hours, _minutes, _seconds, _subSeconds, (_hoursPeriod == RTC_AM)? AM : PM);
  }
}

/**
  * @brief  set RTC hours.
  * @param  hours: 0-23 or 0-12
  * @param  hours format: AM or PM
  * @retval none
  */
void STM32RTC::setHours(uint8_t hours, RTC_AM_PM period)
{
  if (_configured) {
    syncTime();
    if(hours < 24) {
      _hours = hours;
    }
    _hoursPeriod = period;
    RTC_SetTime(_hours, _minutes, _seconds, _subSeconds, (_hoursPeriod == RTC_AM)? AM : PM);
  }
}

/**
  * @brief  set RTC time.
  * @param  hours: 0-23
  * @param  minutes: 0-59
  * @param  seconds: 0-59
  * @retval none
  */
void STM32RTC::setTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
  if (_configured) {
    setSeconds(seconds);
    setMinutes(minutes);
    setHours(hours);
  }
}

/**
  * @brief  set RTC time.
  * @param  hours: 0-23 or 0-12
  * @param  minutes: 0-59
  * @param  seconds: 0-59
  * @param  hour format: AM or PM
  * @retval none
  */
void STM32RTC::setTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds, RTC_AM_PM period)
{
  if (_configured) {
    setSubSeconds(subSeconds);
    setSeconds(seconds);
    setMinutes(minutes);
    setHours(hours, period);
  }
}

/**
  * @brief  set RTC week day.
  * @param  week day: 1-7 (Monday first)
  * @retval none
  */
void STM32RTC::setWeekDay(uint8_t weekDay)
{
  if (_configured) {
    RTC_GetDate(&_year, &_month, &_date, &_day);
    if((weekDay >= 1) && (weekDay <= 7)) {
      _day = weekDay;
    }
    RTC_SetDate(_year, _month, _date, _day);
  }
}

/**
  * @brief  set RTC date.
  * @param  date: 1-31
  * @retval none
  */
void STM32RTC::setDay(uint8_t day)
{
  if (_configured) {
    RTC_GetDate(&_year, &_month, &_date, &_day);
    if((day >= 1) && (day <= 31)) {
      _date = day;
    }
    RTC_SetDate(_year, _month, _date, _day);
  }
}

/**
  * @brief  set RTC month.
  * @param  month: 1-12
  * @retval none
  */
void STM32RTC::setMonth(uint8_t month)
{
  if (_configured) {
    RTC_GetDate(&_year, &_month, &_date, &_day);
    if((month >= 1) && (month <= 12)) {
      _month = month;
    }
    RTC_SetDate(_year, _month, _date, _day);
  }
}

/**
  * @brief  set RTC year.
  * @param  year: 0-99
  * @retval none
  */
void STM32RTC::setYear(uint8_t year)
{
  if (_configured) {
    RTC_GetDate(&_year, &_month, &_date, &_day);
    if(year < 100) {
      _year = year;
    }
    RTC_SetDate(_year, _month, _date, _day);
  }
}

/**
  * @brief  set RTC calendar.
  * @param  day: 1-31
  * @param  month: 1-12
  * @param  year: 0-99
  * @retval none
  */
void STM32RTC::setDate(uint8_t day, uint8_t month, uint8_t year)
{
  if (_configured) {
    setDay(day);
    setMonth(month);
    setYear(year);
  }
}

/**
  * @brief  set RTC calendar.
  * @param  weekDay: 1-7 (Monday first)
  * @param  day: 1-31
  * @param  month: 1-12
  * @param  year: 0-99
  * @retval none
  */
void STM32RTC::setDate(uint8_t weekDay, uint8_t day, uint8_t month, uint8_t year)
{
  if (_configured) {
    setWeekDay(weekDay);
    setDay(day);
    setMonth(month);
    setYear(year);
  }
}

/**
  * @brief  set RTC alarm second.
  * @param  seconds: 0-59
  * @retval none
  */
void STM32RTC::setAlarmSeconds(uint8_t seconds)
{
  if (_configured) {
    if(seconds < 60) {
      _alarmSeconds = seconds;
    }
  }
}

/**
  * @brief  set RTC alarm minute.
  * @param  minutes: 0-59
  * @retval none
  */
void STM32RTC::setAlarmMinutes(uint8_t minutes)
{
  if (_configured) {
    if(minutes < 60) {
      _alarmMinutes = minutes;
    }
  }
}

/**
  * @brief  set RTC alarm hour.
  * @param  hour: 0-23
  * @retval none
  */
void STM32RTC::setAlarmHours(uint8_t hours)
{
  if (_configured) {
    if(hours < 24) {
      _alarmHours = hours;
    }
  }
}

/**
  * @brief  set RTC alarm hour.
  * @param  hour: 0-23 or 0-12
  * @param  hour format: AM or PM
  * @retval none
  */
void STM32RTC::setAlarmHours(uint8_t hours, RTC_AM_PM period)
{
  if (_configured) {
    if(hours < 24) {
      _alarmHours = hours;
    }
    _alarmPeriod = period;
  }
}

/**
  * @brief  set RTC alarm time.
  * @param  hours: 0-23
  * @param  minutes: 0-59
  * @param  seconds: 0-59
  * @retval none
  */
void STM32RTC::setAlarmTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
  if (_configured) {
    setAlarmHours(hours);
    setAlarmMinutes(minutes);
    setAlarmSeconds(seconds);
  }
}

/**
  * @brief  set RTC alarm time.
  * @param  hours: 0-23
  * @param  minutes: 0-59
  * @param  seconds: 0-59
  * @param  hour format: AM or PM
  * @retval none
  */
void STM32RTC::setAlarmTime(uint8_t hours, uint8_t minutes, uint8_t seconds, RTC_AM_PM period)
{
  if (_configured) {
    setAlarmHours(hours, period);
    setAlarmMinutes(minutes);
    setAlarmSeconds(seconds);
  }
}

/**
  * @brief  set RTC alarm date.
  * @param  day: 1-31
  * @retval none
  */
void STM32RTC::setAlarmDay(uint8_t day)
{
  if (_configured) {
    if((day >= 1) && (day <= 31)) {
      _alarmDate = day;
    }
  }
}

/**
  * @brief  set RTC alarm month.
  * @NOTE   This function is kept for compatibility but the STM32 RTC
  *         can't assign a month to an alarm. See board datasheet.
  * @param  month is ignored.
  */
void STM32RTC::setAlarmMonth(uint8_t month)
{
  UNUSED(month);
}

/**
  * @brief  set RTC alarm year.
  * @NOTE   This function is kept for compatibility but the STM32 RTC
  *         can't assign a year to an alarm. See board datasheet.
  * @param  year is ignored.
  */
void STM32RTC::setAlarmYear(uint8_t year)
{
  UNUSED(year);
}

/**
  * @brief  set RTC alarm date.
  * @NOTE   Parameters month and year are ingored because the STM32 RTC can't
  *         assign a month or year to an alarm. See board datasheet.
  * @param  day: 1-31
  * @param  month is ignored
  * @param  year is ignored
  */
void STM32RTC::setAlarmDate(uint8_t day, uint8_t month, uint8_t year)
{
  UNUSED(month);
  UNUSED(year);

  setAlarmDay(day);
}

/**
  * @brief  get epoch time
  * @retval epoch time in seconds
  */
uint32_t STM32RTC::getEpoch(void)
{
  struct tm tm;

  syncDate();
  syncTime();

  tm.tm_isdst = -1;
  tm.tm_yday = 0;
  tm.tm_wday = _day - 1;
  tm.tm_year = _year + EPOCH_TIME_YEAR_OFF;
  tm.tm_mon = _month - 1;
  tm.tm_mday = _date;
  tm.tm_hour = _hours;
  tm.tm_min = _minutes;
  tm.tm_sec = _seconds;

  return mktime(&tm);
}

/**
  * @brief  get epoch time since 1st January 2000, 00:00:00
  * @retval epoch time in seconds
  */
uint32_t STM32RTC::getY2kEpoch(void)
{
  return (getEpoch() - EPOCH_TIME_OFF);
}

/**
  * @brief  set RTC alarm from epoch time
  * @param  epoch time in seconds
  */
void STM32RTC::setAlarmEpoch(uint32_t ts)
{
  if (_configured) {
    if (ts < EPOCH_TIME_OFF) {
      ts = EPOCH_TIME_OFF;
    }

    time_t t = ts;
    struct tm* tmp = gmtime(&t);

    setAlarmDay(tmp->tm_mday);
    setAlarmHours(tmp->tm_hour);
    setAlarmMinutes(tmp->tm_min);
    setAlarmSeconds(tmp->tm_sec);
    enableAlarm(MATCH_DHHMMSS);
  }
}

/**
  * @brief  set RTC time from epoch time
  * @param  epoch time in seconds
  */
void STM32RTC::setEpoch(uint32_t ts)
{
  if (_configured) {
    if (ts < EPOCH_TIME_OFF) {
      ts = EPOCH_TIME_OFF;
    }

    time_t t = ts;
    struct tm* tmp = gmtime(&t);

    _year = tmp->tm_year - EPOCH_TIME_YEAR_OFF;
    _month = tmp->tm_mon + 1;
    _date = tmp->tm_mday;
    _day = tmp->tm_wday + 1;
    _hours = tmp->tm_hour;
    _minutes = tmp->tm_min;
    _seconds = tmp->tm_sec;

    RTC_SetDate(_year, _month, _date, _day);
    RTC_SetTime(_hours, _minutes, _seconds, _subSeconds, (_hoursPeriod == RTC_AM)? AM : PM);
  }
}

/**
  * @brief  set RTC time from epoch time since 1st January 2000, 00:00:00
  * @param  epoch time in seconds
  */
void STM32RTC::setY2kEpoch(uint32_t ts)
{
  if (_configured) {
    setEpoch(ts + EPOCH_TIME_OFF);
  }
}

/**
  * @brief  synchronise the time from the current RTC one
  * @param  none
  */
void STM32RTC::syncTime(void)
{
  if(_configured) {
    hourAM_PM_t p = AM;
    RTC_GetTime(&_hours, &_minutes, &_seconds, &_subSeconds, &p);
    _hoursPeriod = (p == AM)? RTC_AM : RTC_PM;
  }
}

/**
  * @brief  synchronise the time from the current RTC one
  * @param  none
  */
void STM32RTC::syncDate(void)
{
  if(_configured) {
    RTC_GetDate(&_year, &_month, &_date, &_day);
  }
}

/**
  * @brief  synchronise the alarm time from the current RTC one
  * @param  none
  */
void STM32RTC::syncAlarmTime(void)
{
  if(_configured) {
    hourAM_PM_t p = AM;
    RTC_GetAlarm(&_alarmDate, &_alarmHours, &_alarmMinutes, &_alarmSeconds, &_alarmSubSeconds, &p);
    _alarmPeriod = (p == AM)? RTC_AM : RTC_PM;
  }
}
