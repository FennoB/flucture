#include "flx_datetime.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <array>

flx_duration::flx_duration() : m_duration(0) {}

flx_duration::flx_duration(const std::chrono::milliseconds& ms) : m_duration(ms) {}

flx_duration::flx_duration(long long milliseconds) : m_duration(milliseconds) {}

flx_duration flx_duration::days(int d) {
    return flx_duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(24 * d)));
}

flx_duration flx_duration::hours(int h) {
    return flx_duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(h)));
}

flx_duration flx_duration::minutes(int m) {
    return flx_duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(m)));
}

flx_duration flx_duration::seconds(int s) {
    return flx_duration(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(s)));
}

flx_duration flx_duration::milliseconds(long long ms) {
    return flx_duration(ms);
}

long long flx_duration::total_milliseconds() const {
    return m_duration.count();
}

long long flx_duration::total_seconds() const {
    return std::chrono::duration_cast<std::chrono::seconds>(m_duration).count();
}

long long flx_duration::total_minutes() const {
    return std::chrono::duration_cast<std::chrono::minutes>(m_duration).count();
}

long long flx_duration::total_hours() const {
    return std::chrono::duration_cast<std::chrono::hours>(m_duration).count();
}

long long flx_duration::total_days() const {
    return std::chrono::duration_cast<std::chrono::hours>(m_duration).count() / 24;
}

flx_duration flx_duration::operator+(const flx_duration& other) const {
    return flx_duration(m_duration + other.m_duration);
}

flx_duration flx_duration::operator-(const flx_duration& other) const {
    return flx_duration(m_duration - other.m_duration);
}

flx_duration flx_duration::operator*(long long factor) const {
    return flx_duration(m_duration * factor);
}

flx_duration flx_duration::operator/(long long factor) const {
    return flx_duration(m_duration / factor);
}

bool flx_duration::operator==(const flx_duration& other) const {
    return m_duration == other.m_duration;
}

bool flx_duration::operator<(const flx_duration& other) const {
    return m_duration < other.m_duration;
}

flx_datetime::flx_datetime() : m_timepoint(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())) {}

flx_datetime::flx_datetime(const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>& tp) : m_timepoint(tp) {}

flx_datetime::flx_datetime(int year, int month, int day, int hour, int minute, int second, int millisecond) {
    m_timepoint = create_timepoint(year, month, day, hour, minute, second, millisecond);
}

flx_datetime::flx_datetime(const flx_string& iso_string) {
    m_timepoint = parse_iso_string(iso_string);
}

flx_datetime::flx_datetime(const char* iso_string) {
    m_timepoint = parse_iso_string(flx_string(iso_string));
}

flx_datetime::operator flx_string() const {
    return to_iso();
}

flx_datetime& flx_datetime::operator=(const flx_string& iso_string) {
    m_timepoint = parse_iso_string(iso_string);
    return *this;
}

flx_datetime& flx_datetime::operator=(const char* iso_string) {
    m_timepoint = parse_iso_string(flx_string(iso_string));
    return *this;
}

bool flx_datetime::operator==(const flx_datetime& other) const {
    return m_timepoint == other.m_timepoint;
}

bool flx_datetime::operator!=(const flx_datetime& other) const {
    return m_timepoint != other.m_timepoint;
}

bool flx_datetime::operator<(const flx_datetime& other) const {
    return m_timepoint < other.m_timepoint;
}

bool flx_datetime::operator<=(const flx_datetime& other) const {
    return m_timepoint <= other.m_timepoint;
}

bool flx_datetime::operator>(const flx_datetime& other) const {
    return m_timepoint > other.m_timepoint;
}

bool flx_datetime::operator>=(const flx_datetime& other) const {
    return m_timepoint >= other.m_timepoint;
}

flx_datetime flx_datetime::operator+(const flx_duration& duration) const {
    return flx_datetime(m_timepoint + duration.to_chrono());
}

flx_datetime flx_datetime::operator-(const flx_duration& duration) const {
    return flx_datetime(m_timepoint - duration.to_chrono());
}

flx_duration flx_datetime::operator-(const flx_datetime& other) const {
    return flx_duration(std::chrono::duration_cast<std::chrono::milliseconds>(m_timepoint - other.m_timepoint));
}

flx_datetime flx_datetime::now() {
    return flx_datetime();
}

flx_datetime flx_datetime::utc_now() {
    return flx_datetime();
}

flx_datetime flx_datetime::today() {
    return flx_datetime().start_of_day();
}

flx_datetime flx_datetime::yesterday() {
    return today().add_days(-1);
}

flx_datetime flx_datetime::tomorrow() {
    return today().add_days(1);
}

flx_datetime flx_datetime::from_unix_timestamp(long long timestamp) {
    auto tp = std::chrono::system_clock::from_time_t(timestamp);
    return flx_datetime(std::chrono::time_point_cast<std::chrono::milliseconds>(tp));
}

flx_datetime flx_datetime::from_iso(const flx_string& iso_string) {
    return flx_datetime(iso_string);
}

std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> 
flx_datetime::parse_iso_string(const flx_string& str) {
    std::tm tm = {};
    int milliseconds = 0;
    
    if (str.contains("T")) {
        if (str.contains(".")) {
            auto dot_pos = str.find(".");
            auto z_pos = str.find("Z");
            if (z_pos == flx_string::npos) z_pos = str.size();
            
            flx_string datetime_part = str.substr(0, dot_pos);
            flx_string ms_part = str.substr(dot_pos + 1, z_pos - dot_pos - 1);
            
            std::istringstream ss(datetime_part.c_str());
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            if (ss.fail()) {
                throw flx_datetime_exception("Invalid ISO datetime format: " + str);
            }
            
            milliseconds = ms_part.to_int(0);
            while (ms_part.size() < 3) {
                milliseconds *= 10;
                ms_part = ms_part + "0";
            }
        } else {
            flx_string clean_str = str;
            if (clean_str.contains("Z")) {
                clean_str = clean_str.substr(0, clean_str.find("Z"));
            }
            
            std::istringstream ss(clean_str.c_str());
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            if (ss.fail()) {
                throw flx_datetime_exception("Invalid ISO datetime format: " + str);
            }
        }
    } else {
        std::istringstream ss(str.c_str());
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (ss.fail()) {
            throw flx_datetime_exception("Invalid ISO date format: " + str);
        }
    }
    
    tm.tm_isdst = -1;
    
    auto time_t_val = std::mktime(&tm);
    if (time_t_val == -1) {
        throw flx_datetime_exception("Invalid date/time values");
    }
    
    auto tp = std::chrono::system_clock::from_time_t(time_t_val);
    auto tp_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(tp);
    tp_ms += std::chrono::milliseconds(milliseconds);
    
    return tp_ms;
}

std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> 
flx_datetime::create_timepoint(int year, int month, int day, int hour, int minute, int second, int millisecond) {
    if (!is_valid_date(year, month, day) || !is_valid_time(hour, minute, second, millisecond)) {
        throw flx_datetime_exception("Invalid date or time values");
    }
    
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;
    
    auto time_t_val = std::mktime(&tm);
    if (time_t_val == -1) {
        throw flx_datetime_exception("Invalid date/time values");
    }
    
    auto tp = std::chrono::system_clock::from_time_t(time_t_val);
    auto tp_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(tp);
    tp_ms += std::chrono::milliseconds(millisecond);
    
    return tp_ms;
}

int flx_datetime::year() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_year + 1900;
}

int flx_datetime::month() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_mon + 1;
}

int flx_datetime::day() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_mday;
}

int flx_datetime::hour() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_hour;
}

int flx_datetime::minute() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_min;
}

int flx_datetime::second() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_sec;
}

int flx_datetime::millisecond() const {
    auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(m_timepoint.time_since_epoch());
    return static_cast<int>(ms_since_epoch.count() % 1000);
}

int flx_datetime::day_of_week() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_wday == 0 ? 7 : tm.tm_wday;
}

int flx_datetime::day_of_year() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_yday + 1;
}

flx_datetime flx_datetime::add_years(int years) const {
    return flx_datetime(year() + years, month(), day(), hour(), minute(), second(), millisecond());
}

flx_datetime flx_datetime::add_months(int months) const {
    int new_year = year();
    int new_month = month() + months;
    
    while (new_month > 12) {
        new_year++;
        new_month -= 12;
    }
    while (new_month < 1) {
        new_year--;
        new_month += 12;
    }
    
    int new_day = std::min(day(), days_in_month(new_year, new_month));
    return flx_datetime(new_year, new_month, new_day, hour(), minute(), second(), millisecond());
}

flx_datetime flx_datetime::add_days(int days) const {
    return *this + flx_duration::days(days);
}

flx_datetime flx_datetime::add_hours(int hours) const {
    return *this + flx_duration::hours(hours);
}

flx_datetime flx_datetime::add_minutes(int minutes) const {
    return *this + flx_duration::minutes(minutes);
}

flx_datetime flx_datetime::add_seconds(int seconds) const {
    return *this + flx_duration::seconds(seconds);
}

flx_datetime flx_datetime::add_milliseconds(long long ms) const {
    return *this + flx_duration::milliseconds(ms);
}

flx_datetime flx_datetime::start_of_day() const {
    return flx_datetime(year(), month(), day(), 0, 0, 0, 0);
}

flx_datetime flx_datetime::end_of_day() const {
    return flx_datetime(year(), month(), day(), 23, 59, 59, 999);
}

flx_datetime flx_datetime::start_of_month() const {
    return flx_datetime(year(), month(), 1, 0, 0, 0, 0);
}

flx_datetime flx_datetime::end_of_month() const {
    return flx_datetime(year(), month(), days_in_month(year(), month()), 23, 59, 59, 999);
}

flx_datetime flx_datetime::start_of_year() const {
    return flx_datetime(year(), 1, 1, 0, 0, 0, 0);
}

flx_datetime flx_datetime::end_of_year() const {
    return flx_datetime(year(), 12, 31, 23, 59, 59, 999);
}

flx_datetime flx_datetime::start_of_week() const {
    int days_to_subtract = day_of_week() - 1;
    return add_days(-days_to_subtract).start_of_day();
}

flx_datetime flx_datetime::end_of_week() const {
    int days_to_add = 7 - day_of_week();
    return add_days(days_to_add).end_of_day();
}

flx_string flx_datetime::to_iso() const {
    std::ostringstream oss;
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    int ms = millisecond();
    if (ms > 0) {
        oss << "." << std::setfill('0') << std::setw(3) << ms;
    }
    
    return flx_string(oss.str());
}

flx_string flx_datetime::to_iso_date() const {
    std::ostringstream oss;
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    oss << std::put_time(&tm, "%Y-%m-%d");
    return flx_string(oss.str());
}

flx_string flx_datetime::to_iso_time() const {
    std::ostringstream oss;
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    oss << std::put_time(&tm, "%H:%M:%S");
    
    int ms = millisecond();
    if (ms > 0) {
        oss << "." << std::setfill('0') << std::setw(3) << ms;
    }
    
    return flx_string(oss.str());
}

flx_string flx_datetime::to_date_string() const {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << day() << "."
        << std::setfill('0') << std::setw(2) << month() << "."
        << year();
    return flx_string(oss.str());
}

flx_string flx_datetime::to_time_string() const {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hour() << ":"
        << std::setfill('0') << std::setw(2) << minute() << ":"
        << std::setfill('0') << std::setw(2) << second();
    return flx_string(oss.str());
}

flx_string flx_datetime::to_datetime_string() const {
    return to_date_string() + " " + to_time_string();
}

flx_string flx_datetime::format(const flx_string& format_string) const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, format_string.c_str());
    return flx_string(oss.str());
}

bool flx_datetime::is_leap_year() const {
    return is_leap_year(year());
}

bool flx_datetime::is_weekend() const {
    int dow = day_of_week();
    return dow == 6 || dow == 7;
}

bool flx_datetime::is_same_day(const flx_datetime& other) const {
    return year() == other.year() && month() == other.month() && day() == other.day();
}

bool flx_datetime::is_same_month(const flx_datetime& other) const {
    return year() == other.year() && month() == other.month();
}

bool flx_datetime::is_same_year(const flx_datetime& other) const {
    return year() == other.year();
}

long long flx_datetime::milliseconds_since_epoch() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(m_timepoint.time_since_epoch()).count();
}

long long flx_datetime::seconds_since_epoch() const {
    return std::chrono::duration_cast<std::chrono::seconds>(m_timepoint.time_since_epoch()).count();
}

long long flx_datetime::days_since_epoch() const {
    return std::chrono::duration_cast<std::chrono::hours>(m_timepoint.time_since_epoch()).count() / 24;
}

flx_duration flx_datetime::duration_since(const flx_datetime& other) const {
    return *this - other;
}

long long flx_datetime::days_between(const flx_datetime& other) const {
    auto abs_duration = (*this - other);
    return abs_duration.total_days();
}

long long flx_datetime::hours_between(const flx_datetime& other) const {
    auto abs_duration = (*this - other);
    return abs_duration.total_hours();
}

long long flx_datetime::minutes_between(const flx_datetime& other) const {
    auto abs_duration = (*this - other);
    return abs_duration.total_minutes();
}

long long flx_datetime::seconds_between(const flx_datetime& other) const {
    auto abs_duration = (*this - other);
    return abs_duration.total_seconds();
}

int flx_datetime::age_at_date(const flx_datetime& reference_date) const {
    int age = reference_date.year() - year();
    if (reference_date.month() < month() || 
        (reference_date.month() == month() && reference_date.day() < day())) {
        age--;
    }
    return age;
}

int flx_datetime::current_age() const {
    return age_at_date(flx_datetime::now());
}

int flx_datetime::calendar_week() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(m_timepoint);
    auto tm = *std::localtime(&time_t_val);
    
    char buffer[8];
    std::strftime(buffer, sizeof(buffer), "%V", &tm);
    return std::stoi(buffer);
}

flx_string flx_datetime::weekday_name() const {
    static const std::array<const char*, 7> names = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
    };
    return flx_string(names[day_of_week() - 1]);
}

flx_string flx_datetime::weekday_short() const {
    static const std::array<const char*, 7> names = {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };
    return flx_string(names[day_of_week() - 1]);
}

flx_string flx_datetime::month_name() const {
    static const std::array<const char*, 12> names = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    return flx_string(names[month() - 1]);
}

flx_string flx_datetime::month_short() const {
    static const std::array<const char*, 12> names = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return flx_string(names[month() - 1]);
}

bool flx_datetime::is_valid() const {
    return true;
}

bool flx_datetime::is_valid_date(int year, int month, int day) {
    if (year < 1900 || year > 9999) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > days_in_month(year, month)) return false;
    return true;
}

bool flx_datetime::is_valid_time(int hour, int minute, int second, int millisecond) {
    if (hour < 0 || hour > 23) return false;
    if (minute < 0 || minute > 59) return false;
    if (second < 0 || second > 59) return false;
    if (millisecond < 0 || millisecond > 999) return false;
    return true;
}

bool flx_datetime::is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int flx_datetime::days_in_month(int year, int month) {
    static const std::array<int, 12> days = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days[month - 1];
}
