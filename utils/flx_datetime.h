#ifndef FLX_DATETIME_H
#define FLX_DATETIME_H

#include "flx_string.h"
#include <chrono>
#include <stdexcept>

class flx_datetime_exception : public std::runtime_error {
public:
    flx_datetime_exception(const flx_string& message) : std::runtime_error(message.c_str()) {}
};

class flx_duration {
private:
    std::chrono::milliseconds m_duration;

public:
    flx_duration();
    flx_duration(const std::chrono::milliseconds& ms);
    flx_duration(long long milliseconds);
    
    std::chrono::milliseconds to_chrono() const { return m_duration; }
    
    static flx_duration days(int d);
    static flx_duration hours(int h);
    static flx_duration minutes(int m);
    static flx_duration seconds(int s);
    static flx_duration milliseconds(long long ms);
    
    long long total_milliseconds() const;
    long long total_seconds() const;
    long long total_minutes() const;
    long long total_hours() const;
    long long total_days() const;
    
    flx_duration operator+(const flx_duration& other) const;
    flx_duration operator-(const flx_duration& other) const;
    flx_duration operator*(long long factor) const;
    flx_duration operator/(long long factor) const;
    
    bool operator==(const flx_duration& other) const;
    bool operator<(const flx_duration& other) const;
};

class flx_datetime
{
private:
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> m_timepoint;
    
    static std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> parse_iso_string(const flx_string& str);
    static std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> create_timepoint(int year, int month, int day, int hour, int minute, int second, int millisecond);

public:
    flx_datetime();
    flx_datetime(const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>& tp);
    flx_datetime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int millisecond = 0);
    flx_datetime(const flx_string& iso_string);
    flx_datetime(const char* iso_string);
    
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> to_chrono() const { return m_timepoint; }
    
    operator flx_string() const;
    
    flx_datetime& operator=(const flx_string& iso_string);
    flx_datetime& operator=(const char* iso_string);
    
    bool operator==(const flx_datetime& other) const;
    bool operator!=(const flx_datetime& other) const;
    bool operator<(const flx_datetime& other) const;
    bool operator<=(const flx_datetime& other) const;
    bool operator>(const flx_datetime& other) const;
    bool operator>=(const flx_datetime& other) const;
    
    flx_datetime operator+(const flx_duration& duration) const;
    flx_datetime operator-(const flx_duration& duration) const;
    flx_duration operator-(const flx_datetime& other) const;
    
    static flx_datetime now();
    static flx_datetime utc_now();
    static flx_datetime today();
    static flx_datetime yesterday();
    static flx_datetime tomorrow();
    static flx_datetime from_unix_timestamp(long long timestamp);
    static flx_datetime from_iso(const flx_string& iso_string);
    
    int year() const;
    int month() const;
    int day() const;
    int hour() const;
    int minute() const;
    int second() const;
    int millisecond() const;
    int day_of_week() const;
    int day_of_year() const;
    
    flx_datetime add_years(int years) const;
    flx_datetime add_months(int months) const;
    flx_datetime add_days(int days) const;
    flx_datetime add_hours(int hours) const;
    flx_datetime add_minutes(int minutes) const;
    flx_datetime add_seconds(int seconds) const;
    flx_datetime add_milliseconds(long long ms) const;
    
    flx_datetime start_of_day() const;
    flx_datetime end_of_day() const;
    flx_datetime start_of_month() const;
    flx_datetime end_of_month() const;
    flx_datetime start_of_year() const;
    flx_datetime end_of_year() const;
    flx_datetime start_of_week() const;
    flx_datetime end_of_week() const;
    
    flx_string to_iso() const;
    flx_string to_iso_date() const;
    flx_string to_iso_time() const;
    flx_string to_date_string() const;
    flx_string to_time_string() const;
    flx_string to_datetime_string() const;
    flx_string format(const flx_string& format_string) const;
    
    bool is_leap_year() const;
    bool is_weekend() const;
    bool is_same_day(const flx_datetime& other) const;
    bool is_same_month(const flx_datetime& other) const;
    bool is_same_year(const flx_datetime& other) const;
    
    long long milliseconds_since_epoch() const;
    long long seconds_since_epoch() const;
    long long days_since_epoch() const;
    
    flx_duration duration_since(const flx_datetime& other) const;
    long long days_between(const flx_datetime& other) const;
    long long hours_between(const flx_datetime& other) const;
    long long minutes_between(const flx_datetime& other) const;
    long long seconds_between(const flx_datetime& other) const;
    
    int age_at_date(const flx_datetime& reference_date) const;
    int current_age() const;
    
    int calendar_week() const;
    flx_string weekday_name() const;
    flx_string weekday_short() const;
    flx_string month_name() const;
    flx_string month_short() const;
    
    bool is_valid() const;
    static bool is_valid_date(int year, int month, int day);
    static bool is_valid_time(int hour, int minute, int second, int millisecond = 0);
    
private:
    static bool is_leap_year(int year);
    static int days_in_month(int year, int month);
};

#endif // FLX_DATETIME_H
