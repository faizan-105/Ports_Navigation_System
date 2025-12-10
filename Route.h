
#pragma once
#ifndef ROUTE_H
#define ROUTE_H
#include <string>
using namespace std;
class Route
{
public:
    string origin;
    string destination;
    string date;          // Format: DD/MM/YYYY
    string departureTime; // Format: HH:MM
    string arrivalTime;   // Format: HH:MM
    int cost;             // In USD
    string shippingCompany;
    Route() : origin(""), destination(""), date(""),
              departureTime(""), arrivalTime(""), cost(0), shippingCompany("")
    {
    }
    Route(const string &orig, const string &dest, const string &dt,
          const string &depTime, const string &arrTime, int cst,
          const string &company)
        : origin(orig), destination(dest), date(dt), departureTime(depTime),
          arrivalTime(arrTime), cost(cst), shippingCompany(company)
    {
    }
    // Helper function to compare times (returns true if time1 < time2)
    static bool isTimeBefore(const std::string &time1, const std::string &time2)
    {
        int h1 = (time1[0] - '0') * 10 + (time1[1] - '0');
        int m1 = (time1[3] - '0') * 10 + (time1[4] - '0');
        int h2 = (time2[0] - '0') * 10 + (time2[1] - '0');
        int m2 = (time2[3] - '0') * 10 + (time2[4] - '0');
        if (h1 < h2)
            return true;
        if (h1 > h2)
            return false;
        return m1 < m2;
    }
    // Parse date string (DD/MM/YYYY) and return day, month, year
    static void parseDate(const std::string &dateStr, int &day, int &month, int &year)
    {
        // Format: DD/MM/YYYY
        day = (dateStr[0] - '0') * 10 + (dateStr[1] - '0');
        month = (dateStr[3] - '0') * 10 + (dateStr[4] - '0');
        year = (dateStr[6] - '0') * 1000 + (dateStr[7] - '0') * 100 +
               (dateStr[8] - '0') * 10 + (dateStr[9] - '0');
    }

    // Convert date to string (DD/MM/YYYY)
    static std::string dateToString(int day, int month, int year)
    {
        std::string result = "";
        if (day < 10)
            result += "0";
        result += std::to_string(day) + "/";
        if (month < 10)
            result += "0";
        result += std::to_string(month) + "/";
        result += std::to_string(year);
        return result;
    }

    // Get next day date string
    static std::string getNextDay(const std::string &dateStr)
    {
        int day, month, year;
        parseDate(dateStr, day, month, year);

        // Days in each month (assuming non-leap year for simplicity)
        int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        day++;
        if (day > daysInMonth[month - 1])
        {
            day = 1;
            month++;
            if (month > 12)
            {
                month = 1;
                year++;
            }
        }

        return dateToString(day, month, year);
    }

    // Calculate layover hours between two routes
    static int calculateLayoverHours(const Route &arrivingRoute, const Route &departingRoute)
    {
        if (arrivingRoute.date == departingRoute.date)
        {
            // Same day: calculate time difference
            int arrHour = (arrivingRoute.arrivalTime[0] - '0') * 10 + (arrivingRoute.arrivalTime[1] - '0');
            int arrMin = (arrivingRoute.arrivalTime[3] - '0') * 10 + (arrivingRoute.arrivalTime[4] - '0');
            int depHour = (departingRoute.departureTime[0] - '0') * 10 + (departingRoute.departureTime[1] - '0');
            int depMin = (departingRoute.departureTime[3] - '0') * 10 + (departingRoute.departureTime[4] - '0');

            int arrMinutes = arrHour * 60 + arrMin;
            int depMinutes = depHour * 60 + depMin;

            if (depMinutes >= arrMinutes)
            {
                return (depMinutes - arrMinutes) / 60; // Hours
            }
            else
            {
                // Should not happen if canConnectTo is checked first
                return 24; // Next day
            }
        }
        else
        {
            // Different dates: calculate full layover including waiting days
            int daysDifference = calculateDaysDifference(arrivingRoute.date, departingRoute.date);

            int arrHour = (arrivingRoute.arrivalTime[0] - '0') * 10 + (arrivingRoute.arrivalTime[1] - '0');
            int arrMin = (arrivingRoute.arrivalTime[3] - '0') * 10 + (arrivingRoute.arrivalTime[4] - '0');
            int depHour = (departingRoute.departureTime[0] - '0') * 10 + (departingRoute.departureTime[1] - '0');
            int depMin = (departingRoute.departureTime[3] - '0') * 10 + (departingRoute.departureTime[4] - '0');

            int arrMinutes = arrHour * 60 + arrMin;
            int depMinutes = depHour * 60 + depMin;

            // Hours from arrival to midnight + full days waiting + hours from midnight to departure
            int hoursToMidnight = (24 * 60 - arrMinutes) / 60; // Rest of arrival day
            int fullDaysWaiting = (daysDifference - 1) * 24;   // Complete days between arrival and departure days
            int hoursFromMidnight = depMinutes / 60;           // Partial day of departure

            return hoursToMidnight + fullDaysWaiting + hoursFromMidnight;
        }
    }

    // Helper: Calculate days difference between two dates (DD/MM/YYYY format)
    static int calculateDaysDifference(const std::string &fromDate, const std::string &toDate)
    {
        int days = 0;
        std::string current = fromDate;

        while (current != toDate)
        {
            current = getNextDay(current);
            days++;
            if (days > 365)
                break; // Safety check
        }

        return days;
    }

    // Check if this route can connect to another (arrival before next departure)
    bool canConnectTo(const Route &nextRoute) const
    {
        // Allow any route that departs on or after arrival date
        // (Ship can wait for multiple days if needed for cheaper route)
        return compareDates(nextRoute.date, date) >= 0;
    }

    // Helper: Compare two dates (DD/MM/YYYY format)
    // Returns: positive if date1 > date2, 0 if equal, negative if date1 < date2
    static int compareDates(const std::string &date1, const std::string &date2)
    {
        // Extract day, month, year from DD/MM/YYYY
        int day1 = std::stoi(date1.substr(0, 2));
        int month1 = std::stoi(date1.substr(3, 2));
        int year1 = std::stoi(date1.substr(6, 4));

        int day2 = std::stoi(date2.substr(0, 2));
        int month2 = std::stoi(date2.substr(3, 2));
        int year2 = std::stoi(date2.substr(6, 4));

        // Compare years
        if (year1 != year2)
            return year1 - year2;
        // Compare months
        if (month1 != month2)
            return month1 - month2;
        // Compare days
        return day1 - day2;
    }
};
#endif