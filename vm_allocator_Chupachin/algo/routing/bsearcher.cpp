#include "bsearcher.h"

#include "element.h"
#include "edge.h"
#include "link.h"
#include <iostream>

BSearcher::BSearcher(Element * s, Element * e, Element * t)
:
    start(s),
    end(e),
    tunnel(t)
{
}

bool BSearcher::isValid() const {
    if ( !start ) return false;
    if ( !start->isPhysical() ) return false;
    if ( !end ) return false;
    if ( !end->isPhysical() ) return false;
    if ( tunnel != 0 && !tunnel->isVirtual() && !tunnel->isLink() ) return false;
    return true;
}

bool BSearcher::search() {
    unvisited.push(start);
    ancestors[start] = 0;

    while ( !unvisited.empty() ) {
        Element * next = unvisited.front();
        unvisited.pop();

        if ( next == end ) {
            return true;
        }

        if ( !next->isNetwork() && next != start )
           continue;

        if (next->isLink()) {
            if (!next->toLink()->canHostAssignment(tunnel)) {
                std::cout <<  "BSearcher tiny link" << std::endl;
                continue;
            }
        }

        if (next->isNode()) {

            Elements adjacentEdges = next->adjacentEdges();
            Elements edges;
            for (Elements::iterator j = adjacentEdges.begin(); j != adjacentEdges.end(); j++) {
                Edge * edge = (*j)->toEdge();
                if (edge->toLink()->canHostAssignment(tunnel))
                    edges.insert(edge);
            }

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

        } else {
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

    return false;
}

Path BSearcher::getPath() const {
    if ( ancestors.count(end) == 0 )
        return Path();

    Path result(end, start);
    Element * next = end;
    while ( next != start ) {
        Element * ancestor = ancestors.at(next);
        Elements ancestorEdges = ancestor->adjacentEdges();
        std::vector<Element *> connectingEdges;
        Elements::iterator i;
        for ( i = ancestorEdges.begin(); i != ancestorEdges.end(); i++) {
            Edge * edge = (*i)->toEdge();
            if ( edge->connects(next) )
                connectingEdges.push_back(edge);
        }

        // Sort all physical links and choose the largest link (the least loaded)
        std::sort(connectingEdges.begin(), connectingEdges.end(), Criteria::elementWeightDescending);
        Element * edge = connectingEdges[0];

        // Assign tunnel on this physical edge
        if (!edge->canHostAssignment(tunnel))
            printf("[ERROR] In BSearcher tunnel cannot assigned on found path.\n");
        edge->toLink()->assignLink(tunnel);
        result.addElement(edge);

        if ( ancestor != start )
            result.addElement(ancestor);
        next = ancestor;
    }

    result.revert();
    return result;
}
