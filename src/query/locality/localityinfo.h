#pragma once
#include <string>

struct LocalityInfo
{
    double spatialLocality;
    double temporalLocality;

    LocalityInfo(double spatialLocality, double temporalLocality):
        spatialLocality(spatialLocality),
        temporalLocality(temporalLocality)
    {
    }

    std::string str() const
    {
        return "[spatial = " + std::to_string(spatialLocality) +
             "; temporal = " + std::to_string(temporalLocality) + "]";
    }
};