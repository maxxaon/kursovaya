#include "bfsrouter.h"

#include "operation.h"
#include "request.h"
#include "criteria.h"
#include "path.h"
#include "link.h"

#include <stdio.h>

BFSRouter::BFSRouter(Request & r, Element * t)
:
    target(t)    
{

    Elements adjacentTunnels = target->adjacentEdges();

    for(Elements::iterator i = adjacentTunnels.begin(); i != adjacentTunnels.end(); i++) {
        Element * tunnel = *i;
        Element * adjacent = tunnel->toEdge()->getAdjacent(target);
        if ( !adjacent->isAssigned() )
            continue;

        Element * assignee = adjacent->getAssignee();
        
        candidates[adjacent] = new Elements();
        searchers[adjacent] = new BFSQueue(assignee, tunnel);
    }
        
}

BFSRouter::~BFSRouter() {
    for(Candidates::iterator i = candidates.begin(); i != candidates.end(); i++)
        delete i->second;
    for(Searchers::iterator i = searchers.begin(); i != searchers.end(); i++)
        delete i->second;
}

bool BFSRouter::isExhausted() const {
    for(Searchers::const_iterator i = searchers.begin(); i != searchers.end(); i++) {
        BFSQueue * searcher = i->second;
        if ( !searcher->isExhausted() )
            return false;
    }

    return true;
}

bool BFSRouter::search() {
    Searchers::iterator i = searchers.begin();

    while( !isExhausted() ) {
        Element * start = i->first;
        BFSQueue * queue = i->second;
        Element * candidate = 0;

        while ( !queue->isExhausted()  ) {
            candidate = queue->getNextCandidate();
            if ( candidate->canHostAssignment(target) )
                break;
        }

        if ( candidate != 0 && candidate->canHostAssignment(target) ) {
            Elements * c = candidates.at(start);
            c->insert(candidate);
            Elements intersection = intersectCandidates();
            if ( !intersection.empty() )
                if ( commit(intersection) ) {
                    return true;
                }
                else
                    discard(intersection);
        }

        i = findNextNonExhausted(i);

    }

    return false;
}

Elements BFSRouter::intersectCandidates() {
    Elements result = *(candidates.begin()->second);
    for ( Candidates::iterator i = candidates.begin(); i != candidates.end(); i++) {
        Elements * c = i->second;
        result = Operation::intersect(result, *c);
    }
    return result;
}

bool BFSRouter::commit(Elements & c) {
    bool flag;
    for(Elements::const_iterator j = c.begin(); j != c.end(); j++) {
        Element * host = *j;
        if ( !host->canHostAssignment(target) )
           continue;

        flag = true;

        if (flag) {
            host->assign(target);
            return true;
        }

    }
    return false;
}

void BFSRouter::discard(Elements & d) {
    for ( Candidates::iterator i = candidates.begin(); i != candidates.end(); i ++) {
        Elements * c = i->second;
        Elements newElements = Operation::minus(*c, d);
        *c = newElements;
    }

}
