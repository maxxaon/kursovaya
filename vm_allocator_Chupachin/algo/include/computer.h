#pragma once

#include "defs.h"
#include "leafnode.h"


class Computer : public LeafNode {
    typedef std::map<Element *, NumaBlock *> Assignments;
    friend class ElementFactory;
public:
    enum Type {
        NONE = 0,
        VNF = 1
    };

    Computer(bool vnf = false) : LeafNode(), computerType(0), numa(0) {
        type = COMPUTER;
        if (vnf)
            computerType |= VNF;
        // nm = new NumaBlock();
        nm = nullptr;
        numaBlocks = std::set<NumaBlock *>();

        maxAttempts = 100;
        attempt = 0;
        depth = 3;
        indices = nullptr;
        candidates = std::vector<NumaBlock *>();
    }
    ~Computer();

    bool isVnf() const {
        return computerType & VNF == VNF;
    }

    virtual bool assign(Element * other);
    virtual void unassign();
    bool exhaustivesearch(Element * other);
    void exhaustiveInit(Element * other);
    void exhaustiveDel();
    bool makeAttempt(Element * other);

    NumaBlocks getNextCortege();
    void advanceCursors();

    Assignments getAssignmentsCache(NumaBlocks blocks);
    Elements getAssignmentPack(Assignments & assignments);
    bool greedyAlgorithm(Elements &t, NumaBlocks & p);

    static bool elementWeightDescending(const NumaBlock * first, const NumaBlock * second);

    static bool elementWeightAscending(const NumaBlock * first, const NumaBlock * second);

    void printNumaInfo();
    void printParametersInfo();
    bool isNumaAssigned() const;

    int getNumaPart(const std::string & param);

private:
    virtual bool typeCheck(const Element * other) const {
        return other->isComputer();
    }

private:
    int computerType; // vnf or not

public:
    //For physical elements
    NumaBlocks numaBlocks;
    int numa;


    //FOR EXHAUSTIVE SEARCH
    static int maxAttempts;
    static int attempt;
    static int depth;
    static std::vector<NumaBlock *> candidates;
    static int* indices;

    //For virtual elements
    NumaBlock * nm;
};
