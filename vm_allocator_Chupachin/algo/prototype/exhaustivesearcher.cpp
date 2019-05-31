#include "exhaustivesearcher.h"

#include "element.h"
#include "network.h"
#include "criteria.h"
#include "operation.h"
#include "path.h"
#include "link.h"
#include "port.h"
#include "routing/bsearcher.h"
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include "leafnode.h"
#include "prototype.h"

#include "computer.h"

using std::vector;




ExhaustiveSearcher::ExhaustiveSearcher(Network * n, const Resources & res, const TenantsElements & tens,
                                       Elements & pool, Element * t, const Elements & assignedElements,
                                       const bool & twoStage, int d, int ma)
:
    network(n),
    resources(res),
    tenantsElements(tens),
    assignedElements(assignedElements),
    twoStage(twoStage),
    target(t),
    maxAttempts(ma),
    attempt(0),
    depth(d)
{
    Elements cand = Operation::filter(pool, target, Criteria::isExhaustiveCandidate);
    if ( cand.size() < depth )
        depth = cand.size();

    candidates = vector<Element *>(cand.begin(), cand.end());
    indices = new int[depth + 1];
    for ( int i = 0; i < depth; i++ )
        indices[i] = i;
    indices[depth] = candidates.size();
    printf("Created exhaustive search environment to assign element %p, %zu possible candidates\n", t, candidates.size());
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    printf("Exhaustive search decomposing, attempted %d of %d attempts,", attempt, maxAttempts);
    if ( isExhausted() )
        printf(" is exhausted\n");
    else
        printf(" not exhausted\n");

    delete[](indices);
}

bool ExhaustiveSearcher::isValid() const {
    if ( !target ) return false;
    if ( !target->isVirtual() ) return false;
    if ( !target->isNode() ) return false;
    return true;
}

bool ExhaustiveSearcher::isExhausted() const {
    return indices[depth - 1] == indices[depth];
}

bool ExhaustiveSearcher::search() {
    if ( !isValid() )
       return false;

    while( !isExhausted() ) {
        attempt++;
        if ( attempt > maxAttempts )
            return false;
        if ( makeAttempt() )
            return true;
    }

    return false;
}

bool ExhaustiveSearcher::makeAttempt() {
    if ( isExhausted() )
        return false;

    Elements cortege = getNextCortege();

    if (!checkAssignmentAvailability(target, cortege))
        return false;
    
    // save vm-server assignment
    Assignments cache = getAssignmentsCache(cortege);
    Elements assignmentPack = getAssignmentPack(cache);

    // save vm-numablock assignment if target is computer
    NumaAssignments numaCache = getNumaAssignmentsCache(assignmentPack);

    //Save old pathes
    std::map<Link *, Path> oldPathes;
    for(Elements::iterator i = assignmentPack.begin(); i != assignmentPack.end(); i++ ) {
        Element *assignment = *i;
        Elements edges = assignment->adjacentEdges();
        for (Elements::iterator e = edges.begin(); e != edges.end(); e++) {
            Element *edge = *e;
            Link *tunnel = edge->toLink();
            if (!tunnel->isAssigned())
                continue;

            Path oldPath = tunnel->getRoute();
            oldPathes[tunnel] = oldPath;
        }
    }
    //

    // Unassign virtual elements.
    // If virtual elements are virtual machines than
    // unassign from servers and from NumaBlocks.
    Operation::forEach(assignmentPack, Operation::unassign);

    assignmentPack.insert(target);
    if ( performGreedyAssignment(assignmentPack, cortege) ) {

        //Work with channel
        //assignmentPack = oldFixed + currFixed + oldMoved + currMoved + target
        //oldFixed - restore channel
        //currFixed - do nothing
        //oldMoved - find pathes in UpdatePathes
        //currMoved - do nothing

        Elements oldElements = Operation::minus(assignmentPack, assignedElements);
        oldElements.erase(target);

        Elements needUpdatePathes;


        if (twoStage) {
            needUpdatePathes = oldElements;
        } else {
            needUpdatePathes = assignmentPack;
        }


        if ( updatePathes(needUpdatePathes) ) {
            return true;
        }

        //If updatePathes return false then roll back to previous assignments
        Operation::forEach(assignmentPack, Operation::unassign);
	
    }

    //Restore old location and old Pathes
    // Restore numablock location if exhaustive 
    // searcher was launched for virtual machines.

    if (numaCache.empty()) {
        for (Assignments::iterator i = cache.begin(); i != cache.end(); i++ ) 
            i->second->assign(i->first);
    } else {
        for (NumaAssignments::iterator i = numaCache.begin(); i != numaCache.end(); i++) {
            Element * vm = i->first;
            Computer * pm =  cache[vm]->toComputer();
            NumaBlock * nb = i->second;
            
            // Simple assign vm without NumaBlocks from Computer::assign 
            pm->LeafNode::assign(vm);

            // Assign to NumaBlock
            nb->assign(vm);
        }
    }


    for (std::map<Link *, Path>::iterator i = oldPathes.begin(); i != oldPathes.end(); i++) {
        Link * link = i->first;
        Path route = i->second;
        std::vector<Element *> path = route.getPath();
        for (size_t j = 0; j < path.size(); j++) {
            if (path[j]->isLink())
                path[j]->toLink()->assignLink(link);
        }
        link->setRoute(route);
        link->setAssignedFlag(true);
    }

    return false;
}

Elements ExhaustiveSearcher::getNextCortege() {
    Elements result;
    for ( int i = 0; i < depth; i++ )
        result.insert(candidates[indices[i]]);

    advanceCursors();
    return result;
}

void ExhaustiveSearcher::advanceCursors() {
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

ExhaustiveSearcher::Assignments ExhaustiveSearcher::getAssignmentsCache(Elements & resources) {
    Assignments result;
    for (Elements::iterator i = resources.begin(); i != resources.end(); i++) {
        Element * resource = *i;
        Elements assignments = resource->getAssignments();
        for(Elements::iterator a = assignments.begin(); a != assignments.end(); a++ ) {
            Element * assignment = *a;
            //check the elements for which migration is not allowed
            if ( assignment->isComputer() || assignment->isStore() ) {
                LeafNode * node = (LeafNode *)assignment;
                if ( node->getMigration() == 0 )
                    continue;
            }
            //
            result[assignment] = resource;
        }
    }
    return result;
}

Elements ExhaustiveSearcher::getAssignmentPack(ExhaustiveSearcher::Assignments & assignments) {
    Elements result;
    for (Assignments::iterator i = assignments.begin(); i != assignments.end(); i++) {
        result.insert(i->first);
    }
    return result;
}


ExhaustiveSearcher::NumaAssignments ExhaustiveSearcher::getNumaAssignmentsCache(Elements & virtualElements) {
    NumaAssignments result;
    if (virtualElements.size() == 0)
        return result;

    if (!(*virtualElements.begin())->isComputer())
        return result;


    for (Elements::iterator i = virtualElements.begin(); i != virtualElements.end(); i++) {
        Computer * vm = (*i)->toComputer();
        if (vm->nm == nullptr)
            continue;
        result[(*i)] = vm->nm;
    }
    
    return result;
}


bool ExhaustiveSearcher::performGreedyAssignment(Elements & t, Elements & p) {
    std::vector<Element *> targets(t.begin(), t.end());
    std::vector<Element *> physical(p.begin(), p.end());

    std::sort(targets.begin(), targets.end(), Criteria::elementWeightDescending);
    std::sort(physical.begin(), physical.end(), Criteria::elementWeightAscending);

    for(std::vector<Element *>::iterator i = targets.begin(); i != targets.end(); i++) {
        Element * element = *i;
        bool result = false;
        for (std::vector<Element *>::iterator j = physical.begin(); j != physical.end(); j++) {
            Element * assignee = *j;
            result = assignee->assign(element);
            if ( result ) {
                break;
            }
        }

        if ( !result ) {
            Operation::forEach(t, Operation::unassign);
            return false;
        }

        std::sort(physical.begin(), physical.end(), Criteria::elementWeightAscending);
    }

    return true;
}

bool ExhaustiveSearcher::updatePathes(Elements & assignments) {
    Elements edges;
    std::vector<Element *> tunnels;

    for(Elements::iterator i = assignments.begin(); i != assignments.end(); i++ ) {
        Element *assignment = *i;
        edges = Operation::join(edges, assignment->adjacentEdges());
    }

    tunnels.insert(tunnels.end(), edges.begin(), edges.end());
    std::sort(tunnels.begin(), tunnels.end(), Criteria::elementWeightDescending);

    for(size_t e = 0; e < tunnels.size(); e ++) {
        Element * edge = tunnels[e];
        Link * tunnel = edge->toLink();
        if (tunnel->isAssigned())
            continue;

        Element * start = tunnel->getFirst()->getParentNode()->getAssignee();
        Element * end = tunnel->getSecond()->getParentNode()->getAssignee();
        BSearcher searcher(start, end, tunnel);

        if ( start == end ) {
            Path emptyPath = Path(end, start);
            tunnel->setRoute(emptyPath);
            tunnel->setAssignedFlag(true);
            continue;
        }

        if ( !searcher.isValid() ) {
            continue;
        }
        if ( !searcher.search() ) {
            return false;
        }
        Path route = searcher.getPath();
        tunnel->setRoute(route);
        tunnel->setAssignedFlag(true);
    }

    return true;
}


bool ExhaustiveSearcher::checkAssignmentAvailability(Element * target, Elements nodes) {
    if (nodes.empty())
        return false;

    Parameters parameters = (*nodes.begin())->getParameters();
    std::map<std::string, double> availableResources;
    Parameters::iterator first = parameters.begin();
    Parameters::iterator last = parameters.end();

    for (Parameters::iterator param = first; param != last; param++ ) {
        if (param->first->getName() == std::string("links"))
                continue;
        availableResources[param->first->getName()] = 0.0;
    }

    for ( Elements::iterator node = nodes.begin(); node != nodes.end(); node++ ) {
        std::map<std::string, double> nodeParameters = (*node)->getAvailableParameters();
        std::map<std::string, double>::iterator firstParam = nodeParameters.begin();
        std::map<std::string, double>::iterator lastParam = nodeParameters.end();
        for ( std::map<std::string, double>::iterator param = firstParam; param != lastParam; param++ ) {
            std::string paramName = param->first;
            double paramValue = param->second;
            availableResources[paramName] += paramValue;
        }
    }

    std::map<std::string, double> targetResources = target->getAvailableParameters();

    std::map<std::string, double>::iterator v;
    for ( v = targetResources.begin(); v != targetResources.end(); v++) {
        std::string paramName = v->first;
        double paramValue = v->second;
        if (paramValue > availableResources[paramName])
            return false;
    }

    return true;
}
