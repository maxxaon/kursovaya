#include "link.h"


Link::Link() : Edge(), throughput(0), latency(DUMMY), assignedFlag(false) {
    type = LINK;
}

void Link::setThroughput(const unsigned & throughput) {
    this->throughput = throughput;
}

bool Link::setRoute(Path& route) {
    this->route = route;
    return true;
}

uint Link::getThroughput() {
    return throughput;
}

uint Link::getFullThroughput() {
    uint result=0;
    for(Elements::iterator i=assignments.begin(); i!=assignments.end(); i++) {
        result+=(*i)->toLink()->getThroughput();
    }
    return ( result + throughput );
}

uint Link::linkCost() {
    //calculate busy throughput
    return (getFullThroughput() - throughput);
}

bool Link::assignLink(Element * other) {
    if ( !canHostAssignment(other) )
        return false;


    decreaseResources(other);

    assignments.insert(other);
    return true;
}

void Link::unassignLink(Element * other) {
    Elements::iterator a = assignments.find(other);
    if ( a == assignments.end() )
        return;

    restoreResources(other);

    assignments.erase(a);
}

Path Link::getRoute() const {
    return route;
}

bool Link::isAssigned() const {
    return assignedFlag;
}

void Link::setAssignedFlag(const bool & value) {
    assignedFlag = value;
}

void Link::unassign() {

    if ( !isAssigned() )
        return;

    route = Path();
    assignedFlag = false;


}

Link::Latencies Link::getLatency() const {
    return latency;
}

void Link::setLatency(Latencies l) {
    latency = l;
}

bool Link::isDummy() const {
    return latency == DUMMY;
}

bool Link::isAffine() const {
    return latency == AFFINITY;
}


bool Link::typeCheck(const Element * other) const {
    return Criteria::isLink(other);
}

bool Link::physicalCheck(const Element * other) const {
    Link * link = other->toLink();
    if ( throughput < link->throughput ) return false;
    return true;
}

void Link::decreaseResources(const Element * other) {
    Link * link = other->toLink();
    throughput -= link->throughput;
}

void Link::restoreResources(const Element * other) {
    Link * link = other->toLink();
    throughput += link->throughput;
}

double Link::weight() const {
    return const_cast<Link *>(this)->getThroughput();
}
