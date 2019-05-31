#pragma once

#include <string>
#include <vector>
#include <deque>
#include "algorithm.h"
#include "exhaustivesearcher.h"

class PrototypeAlgorithm : public Algorithm {
    friend class ExhaustiveSearcher;
    friend class Snapshot;
public:
    PrototypeAlgorithm(Network * n, const Requests & r)
    : Algorithm(n, r) {}


    virtual void schedule();

    void setResources(const Resources & r) { resources = r; }
    void setTenants(const TenantsElements & t) { tenantsElements = t; }

private:
    std::vector<Request *> prioritizeRequests(Requests & r);
    bool exhaustiveSearch(Element * e, Elements & pool, Elements & assignedElements, const bool & twoStage = false);
    bool assignSeedElement(Request * r, Element * e, Elements & pool);

	static bool increasingByRulesCountAndWeight(Request * first, Request * second);
    static double physicalElementValue(Element * element, Network * network, Elements & assignedVirtElements);
    static bool descendingPhys(const Element * first, const Element * second);

    Element * getSeedElement(Request * r, Elements & e, bool isVirtual = true);

    bool assignNodesAndLinks(Request * r);
	bool slAssignmentWithCompactness(Elements & nodes, Elements & pool, Request * r);
    bool assignNodes(Elements & virtualNodes, Elements & pool, Request * r);
	bool assignLinks(Request * request);
    static double virtualElementValue(Element * element);

private:
	Resources resources;
	TenantsElements tenantsElements;

public:
    Network *getNetwork() {
        return network;
    }
	Requests incorrectRequests;
	static double maxWeight;
	static double maxRules;
};
