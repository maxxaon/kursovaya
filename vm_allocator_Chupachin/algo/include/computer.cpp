#include <iostream>
#include "computer.h"
#include "numa_block.h"


int Computer::maxAttempts;
int Computer::attempt;
int Computer::depth;
std::vector<NumaBlock *> Computer::candidates;
int* Computer::indices;


bool Computer::elementWeightDescending(const NumaBlock * first, const NumaBlock * second) {
    return first->weight() < second->weight();
}

bool Computer::elementWeightAscending(const NumaBlock * first, const NumaBlock * second) {
    return first->weight() < second->weight();
}


bool Computer::isNumaAssigned() const {
    return isVirtual() && nm != nullptr;
}


int Computer::getNumaPart(const std::string & param) {
    // If server hasn't numa blocks or
    // hasn't parameter 'param' then return 0.
    
    int numa_part = 0;
    if (numa == 0)
        return numa_part;

    NumaBlock * nb = *numaBlocks.begin();

    std::map<std::string, double> numaBlockParameters = nb->getParametersTotal();
    std::map<std::string, double>::iterator first = numaBlockParameters.begin();
    std::map<std::string, double>::iterator last = numaBlockParameters.end();

    for (std::map<std::string, double>::iterator i = first ; i != last; ++i ) {
        std::string type = i->first;
        int value = static_cast<int>(i->second);
        if (type == param) {
            numa_part = value;
            break;
        }
    }

    return numa_part;
}


void Computer::printParametersInfo() {
    // std::cout << std::endl;
    // std::cout << std::endl;

    // std::map<std::string, double> numaTotal = getParametersTotal();
    // std::map<std::string, double> numaUsed = getParametersUsed();
    // std::map<std::string, double> numaAvailable = getAvailableParameters();

    // std::cout << "TOTAL RESOURCES COMPUTER" << this << std::endl;

    // for (std::map<std::string, double>::iterator j = numaTotal.begin(); j != numaTotal.end(); j++) {
    //     std::cout << j->first << ' ' << j->second<< std::endl;
    // }

    // std::cout << std::endl;

    // std::cout << "USED RESOURCES COMPUTER" << this << std::endl;

    // for (std::map<std::string, double>::iterator j = numaUsed.begin(); j != numaUsed.end(); j++) {
    //     std::cout << j->first << ' ' << j->second<< std::endl;
    // }

    // std::cout << std::endl;

    // std::cout << "AVAILABLE RESOURCES COMPUTER" << this << std::endl;

    // for (std::map<std::string, double>::iterator j = numaAvailable.begin(); j != numaAvailable.end(); j++) {
    //     std::cout << j->first << ' ' << j->second<< std::endl;
    // }

    // std::cout << std::endl;
    // std::cout << std::endl;
}

void Computer::printNumaInfo() {
    // std::cout << std::endl;
    // std::cout << std::endl;
    // for (NumaBlocks::iterator i = numaBlocks.begin(); i != numaBlocks.end(); i++) {
    //     std::map<std::string, double> numaTotal = (*i)->getParametersTotal();
    //     std::map<std::string, double> numaUsed = (*i)->getParametersUsed();
    //     std::map<std::string, double> numaAvailable = (*i)->getAvailableParameters();

    //     std::cout << "TOTAL RESOURCES NUMA" << *i << std::endl;

    //     for (std::map<std::string, double>::iterator j = numaTotal.begin(); j != numaTotal.end(); j++) {
    //         std::cout << j->first << ' ' << j->second<< std::endl;
    //     }

    //     std::cout << std::endl;

    //     std::cout << "USED RESOURCES NUMA" << *i << std::endl;

    //     for (std::map<std::string, double>::iterator j = numaUsed.begin(); j != numaUsed.end(); j++) {
    //         std::cout << j->first << ' ' << j->second<< std::endl;
    //     }

    //     std::cout << std::endl;

    //     std::cout << "AVAILABLE RESOURCES NUMA" << *i << std::endl;

    //     for (std::map<std::string, double>::iterator j = numaAvailable.begin(); j != numaAvailable.end(); j++) {
    //         std::cout << j->first << ' ' << j->second<< std::endl;
    //     }

    //     std::cout << std::endl;

    // }
    // std::cout << std::endl;
    // std::cout << std::endl;
}

bool Computer::assign(Element * other) {
    //TODO Implement for physical element
    if ( !canHostAssignment(other) )
        return false;

    if (!numa) {
        return LeafNode::assign(other);
    }

    // std::cout << "IN Computer::assign(Element * other)" << std::endl;

    printNumaInfo();
    printParametersInfo();

    std::vector<NumaBlock *> blocks;

    for (NumaBlocks::iterator i = numaBlocks.begin(); i != numaBlocks.end(); i++) {
        NumaBlock * block = *i;
        if (block->canAssign(other))
            blocks.push_back(block);
    }

    if (blocks.empty()) {
        // std::cout << "Exhaustive search" << std::endl;
        if (!exhaustivesearch(other))
            return false;
        return true;
    }

    std::sort(blocks.begin(), blocks.end(), Computer::elementWeightAscending);

    printNumaInfo();

    //Greedy criteria
    bool result;
    result = blocks[0]->assign(other);
    //if element was assigned then unassign it in NUMABLOCK::assign
    //unassign from numa block and server

    // std::cout << "After blocks[0]->assign(other)" << std::endl;

    if (result) {
        decreaseResources(other);
        other->assignee = this;
        assignments.insert(other);
    } else {
        //ExhaustiveSearch
        // std::cout << "Exhaustive search" << std::endl;
        if (!exhaustivesearch(other))
            return false;
    }

    return true;
}


void Computer::unassign() {
    nm->unassign(this);
    LeafNode::unassign();
}

Computer::~Computer() {
    //TODO delete all numaBlocks
    for (NumaBlocks::iterator i = numaBlocks.begin(); i != numaBlocks.end(); i++)
        delete (*i);
}


bool Computer::exhaustivesearch(Element * other) {
    exhaustiveInit(other);

    while( indices[depth - 1] != indices[depth] ) {
        attempt++;
        if ( attempt > maxAttempts ) {
            exhaustiveDel();
            return false;       
        }
        if ( makeAttempt(other) ) {
            exhaustiveDel();
            return true;
        }
    }

    exhaustiveDel();
    return false;
}


void Computer::exhaustiveInit(Element * other) {
    if ( numaBlocks.size() < depth )
        depth = numaBlocks.size();

    candidates = std::vector<NumaBlock *>(numaBlocks.begin(), numaBlocks.end());
    indices = new int[depth + 1];
    for ( int i = 0; i < depth; i++ )
        indices[i] = i;
    indices[depth] = candidates.size();
    // printf("Created NUMA exhaustive search environment to assign element %p, %zu possible candidates\n", other, candidates.size());
}

void Computer::exhaustiveDel() {
    // printf("Exhaustive NUMA search decomposing, attempted %d of %d attempts,", attempt, maxAttempts);
    if ( indices[depth - 1] == indices[depth] )
        // printf(" is exhausted\n");
        ;
    else
        // printf(" not exhausted\n");
        ;

    // set to NULL all variables associated with exhaustive search
    delete[] indices;
    indices = nullptr;
    attempt = 0;
}


bool Computer::makeAttempt(Element * other) {
    NumaBlocks cortege = getNextCortege();
    Assignments cache = getAssignmentsCache(cortege);
    Elements assignmentPack = getAssignmentPack(cache);

    //unassign all elements from numablocks but not from current server
    //unassign
    for (Assignments::iterator i = cache.begin(); i != cache.end(); i++) {
        i->second->unassign(i->first);//{9} unassign in numablock
    }

    assignmentPack.insert(other);
    if ( greedyAlgorithm(assignmentPack, cortege)) {
        // UNASSIGN vm from old location as in NUMABLOCK::ASSIGN
        // ! other element was assigned on numablock in greedyAlgorithm
        // ! now assign it to the server
        decreaseResources(other);
        other->assignee = this;
        assignments.insert(other);
        return true;
    }

    //if greedy algorithm return false
    //Restore old location
    for(Assignments::iterator i = cache.begin(); i != cache.end(); i++ )
        i->second->assign(i->first);

    return false;
}


NumaBlocks Computer::getNextCortege() {
    NumaBlocks result;

    for (size_t i = 0; i < depth; i++)
        result.insert(candidates[indices[i]]);

    advanceCursors();
    return result;
}

void Computer::advanceCursors() {
    int border = 0;
    for ( int i = depth - 1; i >= 0; i-- ) {
        if ( indices[i] != indices[i+1] - 1) {
            border = i;
            break;
        }
    }

    indices[border]++;
    for (int i = border + 1; i < depth; i++) {
        indices[i] = indices[i-1] + 1;
    }
}


Computer::Assignments Computer::getAssignmentsCache(NumaBlocks blocks) {
    Assignments result;

    for (NumaBlocks::iterator i = blocks.begin(); i != blocks.end(); i++) {
        NumaBlock * numaBlock = *i;
        Elements assignments = numaBlock->assignments;
        for (Elements::iterator j = assignments.begin(); j != assignments.end(); j++)
            result[*j] = numaBlock;
    }

    return result;
}

Elements Computer::getAssignmentPack( Computer::Assignments & assignments) {
    Elements result;
    for (Assignments::iterator i = assignments.begin(); i != assignments.end(); i ++)
        result.insert(i->first);
    return result;
}

bool Computer::greedyAlgorithm(Elements &t, NumaBlocks & p) {
    std::vector<Element * > targets(t.begin(), t.end());
    std::vector<NumaBlock * > blocks(p.begin(), p.end());

    std::sort(targets.begin(), targets.end(), Criteria::elementWeightDescending);
    std::sort(blocks.begin(), blocks.end(), Computer::elementWeightAscending);

    for(std::vector<Element *>::iterator i = targets.begin(); i != targets.end(); i++) {
        Element * element = *i;
        bool result = false;
        for (std::vector<NumaBlock *>::iterator j = blocks.begin(); j != blocks.end(); j++) {
            NumaBlock * assignee = *j;
            result = assignee->assign(element);
            if ( result ) {
                break;
            }
        }

        if ( !result ) {
            for (std::vector<Element *>::iterator k = targets.begin(); k != targets.end(); k++) {
                if ((*k)->toComputer()->nm == 0)
                    continue;

                Computer * vm = (*k)->toComputer();
                NumaBlock * nm = vm->nm;
                nm->unassign(vm);
            }

            return false;
        }

        std::sort(blocks.begin(), blocks.end(), Computer::elementWeightAscending);
    }

    return true;
}
