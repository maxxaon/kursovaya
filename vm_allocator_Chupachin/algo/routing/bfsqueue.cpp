#include "bfsqueue.h"

#include "element.h"
#include "edge.h"
#include "link.h"
#include <stdio.h>
#include <iostream>
#include "operation.h"

BFSQueue::BFSQueue(Element * s, Element * t) 
: 
    start(s)
{
    if ( !t->isLink() ) throw;
    tunnel = t;

    unvisited.push(s);
    ancestors[s] = 0;
}

Element * BFSQueue::processNextItem() {
    if ( isExhausted() ) {
        return 0;
    }

    Element * next = unvisited.front();
    unvisited.pop();

    if ( next == start || next->isNetwork() ) {
        //if next == link then check throughput
        //through this link path is not exist
        //this link returned but in BFSRouter::search() canHostAssignment will return false,
        // because next has type Link and we search storage or server
        if (next->isLink()) {
            if (!next->canHostAssignment(tunnel)) {
                return next;
            }
        }

        if (next->isNode()) {

            Elements edges = next->adjacentEdges();
            std::vector<Element *> tunnels;
            tunnels.insert(tunnels.end(), edges.begin(), edges.end());
            std::sort(tunnels.begin(), tunnels.end(), Criteria::elementWeightDescending);

            for (std::vector<Element *>::iterator j = tunnels.begin(); j != tunnels.end(); j++) {
                Element * node = (*j)->toEdge()->getAdjacent(next);

                if (ancestors.count(node) != 0)
                    continue;

                unvisited.push(node);
                ancestors[node] = next;
            }



        } else if (next->isLink()) {
            if (!next->canHostAssignment(tunnel)) {
                return next;
            }
            Elements adjacentNodes = next->adjacentNodes();
            for (Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++) {
                Element *node = *i;
                if (ancestors.count(node) != 0)
                    continue;
                unvisited.push(node);
                ancestors[node] = next;
            }
        }

    }

    return next;
}

Element * BFSQueue::getNextCandidate() {
    while ( !isExhausted() ) {
        return processNextItem();
    }

    return 0;
}

Path BFSQueue::getPath(Element * target, bool & correct) const {
    if ( ancestors.count(target) == 0 ) {
        correct = true;
        return Path();
    }

    Path result(target, start);
    Element * next = target;
    while ( next != start ) {
        Element * ancestor = ancestors.at(next);
        Elements ancestorEdges = ancestor->adjacentEdges();
        Elements::iterator i;
        for ( i = ancestorEdges.begin(); i != ancestorEdges.end(); i++) {
            Edge * edge = (*i)->toEdge();
            if ( edge->connects(next) ) {
                //We must assign tunnel on this physical edge
                if (!edge->canHostAssignment(tunnel)) {
                    //delete assignment from current path
                    tunnel->toLink()->setRoute(result);
                    Operation::unassignLink(tunnel);
                    correct = false;
                    return Path();
                }

                edge->toLink()->assignLink(tunnel);

                result.addElement(edge);
                break;
            }
        }
        if ( ancestor != start )
            result.addElement(ancestor);
        next = ancestor;
    }

    correct = true;
    result.revert();
    return result;
}
