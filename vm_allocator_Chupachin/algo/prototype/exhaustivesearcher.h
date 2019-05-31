#pragma once

#include <defs.h>
#include <map>
#include <vector>
#include <string>
#include "numa_block.h"


class ExhaustiveSearcher {
public:
    typedef std::map<Element *, Element *> Assignments; 
    typedef std::map<Element *, NumaBlock *> NumaAssignments; 
    ExhaustiveSearcher(Network * n, const Resources & res, const TenantsElements & tens,
                       Elements & pool, Element * target, const Elements & assignedElements,
                       const bool & twoStage, int depth = 3, int maxAttempts = 1000);
    ~ExhaustiveSearcher();
    

    bool isValid() const;
    bool isExhausted() const;
    bool search();
private:
    bool makeAttempt();
    Elements getNextCortege();
    void advanceCursors();
    Assignments getAssignmentsCache(Elements &);
    NumaAssignments getNumaAssignmentsCache(Elements &);
    Elements getAssignmentPack(Assignments &);

    bool checkAssignmentAvailability(Element * target, Elements nodes);
    bool performGreedyAssignment(Elements & targets, Elements & physical);
    bool updatePathes(Elements & assignments);

private:
    Element * target;
    int maxAttempts;
    int attempt;
    int depth;

    std::vector<Element *> candidates;
    int* indices;

    Network * network;
    Resources resources;
    TenantsElements tenantsElements;

    Elements assignedElements;
    bool twoStage;
};
