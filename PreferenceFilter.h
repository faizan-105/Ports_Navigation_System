
#pragma once
#ifndef PREFERENCEFILTER_H
#define PREFERENCEFILTER_H

#include "LinkedList.h"
#include "Route.h"
#include <string>
using namespace std;
// Structure to hold user preferences for route booking
struct PreferenceFilter
{
    LinkedList<string> preferredCompanies; // Shipping companies to prefer
    LinkedList<string> requiredPorts;      // Ports that must be included in path
    LinkedList<string> excludedPorts;      // Ports to avoid
    int maxVoyageTime;                     // Maximum total voyage time in hours (-1 if no limit)
    bool hasCompanyPreference;
    bool hasPortPreference;
    bool hasTimeLimit;

    PreferenceFilter()
        : maxVoyageTime(-1), hasCompanyPreference(false),
          hasPortPreference(false), hasTimeLimit(false)
    {
    }

    // Check if a route matches the preferences
    bool matchesRoute(const Route &route) const
    {
        // Check company preference
        if (hasCompanyPreference && preferredCompanies.getSize() > 0)
        {
            bool companyMatch = false;
            for (int i = 0; i < preferredCompanies.getSize(); i++)
            {
                if (preferredCompanies.get(i) == route.shippingCompany)
                {
                    companyMatch = true;
                    break;
                }
            }
            if (!companyMatch)
            {
                return false; // Route doesn't match preferred companies
            }
        }

        // Check excluded ports
        if (hasPortPreference && excludedPorts.getSize() > 0)
        {
            for (int i = 0; i < excludedPorts.getSize(); i++)
            {
                if (excludedPorts.get(i) == route.origin ||
                    excludedPorts.get(i) == route.destination)
                {
                    return false; // Route uses excluded port
                }
            }
        }

        return true;
    }

    // Check if a path (list of ports) includes all required ports
    bool pathMatchesPorts(const LinkedList<string> &path) const
    {
        if (!hasPortPreference || requiredPorts.getSize() == 0)
        {
            return true;
        }

        // Check if all required ports are in the path
        for (int i = 0; i < requiredPorts.getSize(); i++)
        {
            bool found = false;
            for (int j = 0; j < path.getSize(); j++)
            {
                if (path.get(j) == requiredPorts.get(i))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return false; // Required port not in path
            }
        }
        return true;
    }

    // Calculate total voyage time for a path (in hours)
    // This is a simplified calculation - you may need to enhance it
    int calculateVoyageTime(const LinkedList<Route> &routes) const
    {
        if (routes.getSize() == 0)
            return 0;

        // For simplicity, calculate time difference between first departure and last arrival
        // In a real system, you'd parse dates and times properly
        const Route &firstRoute = routes.get(0);
        const Route &lastRoute = routes.get(routes.getSize() - 1);

        // Parse times (HH:MM format)
        int firstHour = (firstRoute.departureTime[0] - '0') * 10 + (firstRoute.departureTime[1] - '0');
        int firstMin = (firstRoute.departureTime[3] - '0') * 10 + (firstRoute.departureTime[4] - '0');
        int lastHour = (lastRoute.arrivalTime[0] - '0') * 10 + (lastRoute.arrivalTime[1] - '0');
        int lastMin = (lastRoute.arrivalTime[3] - '0') * 10 + (lastRoute.arrivalTime[4] - '0');

        // Simple calculation (assumes same day for now)
        int totalMinutes = (lastHour * 60 + lastMin) - (firstHour * 60 + firstMin);
        if (totalMinutes < 0)
            totalMinutes += 24 * 60; // Wrap around if needed

        return totalMinutes / 60; // Return hours
    }

    // Check if voyage time is within limit
    bool isVoyageTimeValid(const LinkedList<Route> &routes) const
    {
        if (!hasTimeLimit || maxVoyageTime < 0)
        {
            return true;
        }
        int voyageTime = calculateVoyageTime(routes);
        return voyageTime <= maxVoyageTime;
    }
};

#endif
