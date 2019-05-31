#pragma once

#include "defs.h"


class NumaBlock {
public:
    NumaBlock(){}
    NumaBlock(Parameters params):parameters(params){}
    ~NumaBlock();

    double weight() const;
    bool assign(Element * other);
    void unassign(Element * other);
    bool canAssign(Element * other);
    void decreaseResources(const Element * other);
    void removeAssignment(Element * other);
    void restoreResources(const Element * other);
    void printNumaInfo();

    std::map<std::string, double> getAvailableParameters();
    std::map<std::string, double> getParametersTotal();
    std::map<std::string, double> getParametersUsed();
public:
    Parameters parameters;
    Elements assignments;
};
