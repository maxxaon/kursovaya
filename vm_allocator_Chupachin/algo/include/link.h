#pragma once

#include "edge.h"
#include "criteria.h"
#include "path.h"

#include "element.h"


class Link : public Edge {
    friend class ElementFactory;
    friend class Switch;
    friend class Network;
public:
    enum Attributes {
        NONE = 0
    };

    enum Latencies {
        DUMMY = 0,
        NORMAL = 1,
        AFFINITY = NORMAL << 1
    };

    Link();
    void setThroughput(const unsigned & throughput);

    virtual bool setRoute(Path& route);

    uint getThroughput();
    uint getFullThroughput();
    uint linkCost();
    
    bool assignLink(Element * other);
    void unassignLink(Element * other);

    virtual Path getRoute() const;

    virtual bool isAssigned() const;

    void setAssignedFlag(const bool & value);

    virtual void unassign();

    virtual Latencies getLatency() const;
    virtual void setLatency(Latencies l);

    virtual bool isDummy() const;
    virtual bool isAffine() const;

private:
    virtual bool typeCheck(const Element * other) const;

    virtual bool physicalCheck(const Element * other) const;

    virtual void decreaseResources(const Element * other);

    virtual void restoreResources(const Element * other);

    virtual double weight() const;

private:
    unsigned throughput;
    Latencies latency;
    Path route;
    bool assignedFlag;
};
