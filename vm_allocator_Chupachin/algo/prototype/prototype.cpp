#include "prototype.h"

#include "request.h"
#include "element.h"
#include "network.h"
#include "criteria.h"
#include "operation.h"
#include "link.h"
#include "leafnode.h"
#include "routing/bfsrouter.h"
#include "exhaustivesearcher.h"

#include "routing/bsearcher.h"
#include "routing/dijkstrarouter.h"
#include <stdio.h>

#include <vector>
#include <algorithm>
#include <queue>
#include <deque>
#include <map>
#include <string>
#include <math.h>

#define EPS 0.000001

double PrototypeAlgorithm::maxWeight;
double PrototypeAlgorithm::maxRules;

std::vector<Request *> PrototypeAlgorithm::prioritizeRequests(Requests & r) {
    std::vector<Request *> res;
    res.insert(res.end(), r.begin(), r.end());
    maxRules = -1;
    maxWeight = -1;
    for (Requests::iterator i = requests.begin(); i != requests.end(); i++) {
        Request * request = *i;
        if (request->weight() > maxWeight)
            maxWeight = request->weight();
        if (request->rules > maxRules)
            maxRules = request->rules;
    }

    //std::sort(res.begin(), res.end(), increasingByRulesCountAndWeight);

    return res;
}

bool PrototypeAlgorithm::increasingByRulesCountAndWeight(Request * first, Request * second) {
    double w1 = 0.2;
    double w2 = 0.8;
    return w1* first->weight() + w2 * first->rules > w1 * second->weight() + w2 * second->rules;
}


bool PrototypeAlgorithm::exhaustiveSearch(Element * e, Elements & pool,
                                          Elements & assignedElements, const bool & twoStage) {
    ExhaustiveSearcher searcher(network, resources, tenantsElements, pool, e, assignedElements, twoStage, 2);
    return searcher.search();
}

bool PrototypeAlgorithm::assignSeedElement(Request * r, Element * e, Elements & pool) {
    Elements candidates = Operation::filter(pool, e, Criteria::canHostAssignment);
    if (candidates.empty())
        return false;

    Element *candidate = getSeedElement(r, candidates, false);
    return candidate->assign(e);
}


Element * PrototypeAlgorithm::getSeedElement(Request * r, Elements & e, bool isVirtual) {
    std::vector<Element *> tmp;
    Elements stores = Operation::filter(e, Criteria::isStore);
    if ( !stores.empty() ) {
        tmp = std::vector<Element *>(stores.begin(), stores.end());
    } else {
        tmp = std::vector<Element *>(e.begin(), e.end());
    }

    if ( isVirtual ) {
        std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightDescending);
    } else {
        Elements assignedElements = Operation::filter(r->getNodes(), Criteria::isAssigned);
        for (size_t n = 0; n < tmp.size(); n++) {
            Element * elem = tmp[n];
            elem->value = physicalElementValue(tmp[n], network, assignedElements);
        }

        std::sort(tmp.begin(), tmp.end(), descendingPhys);
    }
    Element * result = tmp[0];
    return result;
} 


bool PrototypeAlgorithm::descendingPhys(const Element *first, const Element *second) {
    return first->value > second->value;
}

double PrototypeAlgorithm::virtualElementValue(Element * element) {
    double result = 0.0;
    Elements edges = element->adjacentEdges();
    for (Elements::iterator i = edges.begin(); i != edges.end(); i++) {
        result += (*i)->toLink()->getFullThroughput();
    }

    return result * element->weight() * element->value;
}

double PrototypeAlgorithm::physicalElementValue(Element * element, Network * network, Elements & assignedVirtElements) {
    double result = 0.0;
    double links = 0.0;
    double closeness = 0.0;
    double compactness = 0.0;

    //Adjacent links
    Elements edges = element->adjacentEdges();
    for (Elements::iterator i = edges.begin(); i != edges.end(); i++) {
        links += (*i)->toLink()->getThroughput();
    }

    Elements physElements = Elements();
    //Physical elements which keep assignedVirtElements
    for (Elements::iterator k = assignedVirtElements.begin(); k != assignedVirtElements.end(); k++) {
        Element * physElement = (*k)->getAssignee();
        physElements.insert(physElement);
    }

    //find shortest pathes from current node to all
    //path consist of only links
    //if path is not exist from <current> element to <another> element then pathes
    //hasn't key <another>
    DijkstraRouter router = DijkstraRouter(const_cast<Element *>(element), network);
    router.route();
    std::map<Element *, std::vector<Element * > > pathes = router.getPathes();
    std::map<Element *, std::vector<Element * > >::iterator i;

    for (i = pathes.begin(); i != pathes.end(); i++) {
        Element * neighbor = i->first;
        std::vector<Element * > path = i->second;
        uint minThroughput = path[0]->toLink()->getThroughput();
        uint currThroughput;

        for (size_t j = 0; j < path.size(); j++) {
            currThroughput = path[j]->toLink()->getThroughput();
            if (currThroughput < minThroughput)
                minThroughput = currThroughput;
        }

        closeness += neighbor->weight() * ( double( minThroughput ) / path.size());

        if (Operation::isIn(neighbor, physElements))
            compactness += double( minThroughput ) / path.size();
    }

    compactness = pow(0.57721, compactness);

    result = element->weight() * links * closeness * compactness;

    return result;
}

//=========================NEW_ASSIGNMENT_STRATEGY=============================
//============================TWO_STAGE_ALGORITHM==============================

void PrototypeAlgorithm::schedule() {
    size_t assignedRequests = 0;
    size_t alreadyAssignedRequests = 0;
    std::vector<Request *> pRequests = prioritizeRequests(requests);
    for (std::vector<Request *>::iterator i = pRequests.begin();
         i != pRequests.end(); i++)
    {
        Request * r = *i;

        bool remove = false;
        for (const auto &it : r->getAllVMsParameters()) {
            if (it.second == -1) {
                remove = true;
            }
        }
        if (r->getName()[0] == '-') {
            for (const auto &server : getNetwork()->getElements()) {
                bool found = false;
                for (auto &assignment : server->getAssignments()) {
                    if (assignment->request->getName() == r->getName().substr(1)) {
                        found = true;
                        assignment->unassign();
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            continue;
        }

        std::cout << r->getName().c_str() << std::endl;

        if (r->isAssigned()) {
            alreadyAssignedRequests++;
            std::cout << "Was assigned" << std::endl;
            continue;
        }

        Request * fakeRequest = new Request(*r);
        if ( assignNodesAndLinks(fakeRequest)) {
            assignedRequests++;
            std::cout << "Now assigned -> " << assignedRequests << std::endl;

            Elements elems = r->getElements();
            for (Elements::iterator k = elems.begin(); k != elems.end(); k++) {
                if (!(*k)->isAssigned()) {
                    incorrectRequests.insert(r);
                    fprintf(stdout, "[ERROR] Not all element from request were assigned %s.\n", r->getName().c_str());
                    fprintf(stderr, "[ERROR] Not all element from request were assigned %s.\n", r->getName().c_str());
                    break;
                }
            }

        } else {
            fprintf(stdout, "[ERROR] Failed to assign request %s.\n", r->getName().c_str());
            fprintf(stderr, "[ERROR] Failed to assign request %s.\n", r->getName().c_str());
            r->purgeAssignments();
            Elements elems = r->getElements();
            for (Elements::iterator k = elems.begin(); k != elems.end(); k++) {
                if ((*k)->isAssigned()) {
                    printf("[ERROR] There are assigned element in request after purge assignment\n\n");
                    break;
                }
            }
            std::cout << "After r->purgeAssignments();"  << std::endl;
        }
        delete fakeRequest;
    }
    std::cout << "Already assigned requests -> " << alreadyAssignedRequests << std::endl;
}


bool PrototypeAlgorithm::assignNodesAndLinks(Request * r) {
    Elements pool = network->getNodes();
    Elements unassignedNodes = Operation::filter(r->elementsToAssign(), Criteria::isComputational);
    Elements serverLayered = Operation::filter(r->elementsToAssign(), Criteria::isServerLayered);

    if ( !slAssignmentWithCompactness(serverLayered, pool, r) ) {
        fprintf(stdout, "----------------[ERROR] server layer requirement failed\n---------------------");
        return false;
    }

    fprintf(stdout, "----------------[INFO] slAssignment was finished\n---------------------");


    //ASSIGN_NODES
    if( !assignNodes(unassignedNodes, pool, r)) {
        fprintf(stdout, "----------------[ERROR] nodes assignment failed\n---------------------");
        return false;
    }

    fprintf(stdout, "----------------[INFO] assignNodes was finished\n---------------------");
    //ASSIGN_LINKS

    if (r->assignedElements().size() != r->getNodes().size())
        fprintf(stdout, "----------------[ERROR] not all nodes were assigned in assignNodes\n---------------------");

    bool result = assignLinks(r);

    if (result)
        fprintf(stdout, "----------------[INFO] links assignment was finished\n---------------------");
    else
        fprintf(stdout, "----------------[ERROR] links assignment failed\n---------------------");
    return result;
}


bool PrototypeAlgorithm::slAssignmentWithCompactness(Elements & nodes, Elements & pool, Request * r) {
    Elements layered = Operation::filter(nodes, Criteria::isServerLayered);

    if ( layered.empty() )
        return true;

    using std::map;
    map<int, Elements> layeredModel;
    for (Elements::iterator i = layered.begin(); i != layered.end(); i++) {
        LeafNode * l = (LeafNode *)(*i);
        layeredModel[l->sl()].insert(l);
    }


    for(map<int, Elements>::iterator i = layeredModel.begin(); i != layeredModel.end(); i++) {
        Elements & elements = i->second;

        if ((*elements.begin())->anti_affinity) {
            //anti_affinity rule processing
            // sort virtual elements by weight
            std::vector<Element *> tmp = std::vector<Element *>(elements.begin(), elements.end());
            std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightDescending);

            //vector with different pms
            std::vector<Element *> pms;
            for (size_t k = 0; k < tmp.size(); k++) {
                BFSRouter router(*r, tmp[k]);

                if ( router.isValid() ) {
                    if (router.search())
                        continue;

                    Elements globalCandidates = Operation::filter(pool, tmp[k], Criteria::canHostAssignment);
                    std::vector<Element *> candidates = std::vector<Element *>(globalCandidates.begin(),
                                                                               globalCandidates.end());

                    //sort physical elements by value
                    std::sort(candidates.begin(), candidates.end(), Criteria::elementIncidentLinksDescending);

                    for (std::vector<Element *>::iterator c = candidates.begin(); c != candidates.end(); c++) {
                        Elements localAssigned;
                        for (Elements::iterator a = (*c)->getAssignments().begin();
                             a != (*c)->getAssignments().end(); a++) {
                            if (Operation::isIn(*a, r->getElements()))
                                localAssigned.insert(*a);
                        }

                        bool layerConflict = false;
                        for (Elements::iterator a = localAssigned.begin(); a != localAssigned.end(); a++) {
                            if (!Criteria::isServerLayered(*a))
                                continue;

                            LeafNode *l = (LeafNode *) (*a);
                            if (l->sl() == i->first)
                                layerConflict = true;
                        }
                        if (layerConflict)
                            continue;

                        bool result = true;

                        result = (*c)->assign(tmp[k]);
                        if (result)
                            break;

                        tmp[k]->unassign();
                    }

                    if (!tmp[k]->isAssigned())
                        return false;

                } else {


                    Elements globalCandidates = Operation::filter(pool, tmp[k], Criteria::canHostAssignment);
                    std::vector<Element *> candidates = std::vector<Element *>(globalCandidates.begin(),
                                                                               globalCandidates.end());

                    //sort physical elements by value
                    std::sort(candidates.begin(), candidates.end(), Criteria::elementIncidentLinksDescending);

                    for (std::vector<Element *>::iterator c = candidates.begin(); c != candidates.end(); c++) {
                        Elements localAssigned;
                        for (Elements::iterator a = (*c)->getAssignments().begin();
                             a != (*c)->getAssignments().end(); a++) {
                            if (Operation::isIn(*a, r->getElements()))
                                localAssigned.insert(*a);
                        }

                        bool layerConflict = false;
                        for (Elements::iterator a = localAssigned.begin(); a != localAssigned.end(); a++) {
                            if (!Criteria::isServerLayered(*a))
                                continue;

                            LeafNode *l = (LeafNode *) (*a);
                            if (l->sl() == i->first)
                                layerConflict = true;
                        }
                        if (layerConflict)
                            continue;

                        bool result = true;

                        result = (*c)->assign(tmp[k]);
                        if (result)
                            break;

                        tmp[k]->unassign();
                    }

                    if (!tmp[k]->isAssigned())
                        return false;
                }
            }


            if (!Operation::filter(elements, Criteria::isUnassigned).empty())
                return false;

        } else if ((*elements.begin())->affinity) {
            //sort virtual elements by weight

            std::vector<Element *> tmp = std::vector<Element *>(elements.begin(), elements.end());
            std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightDescending);

            Elements globalCandidates = Operation::filter(pool, tmp[0], Criteria::canHostAssignment);
            std::vector<Element *> candidates = std::vector<Element *>(globalCandidates.begin(),
                                                                       globalCandidates.end());

            //sort physical elements by value
            std::sort(candidates.begin(), candidates.end(), Criteria::elementIncidentLinksDescending);

            for (std::vector<Element *>::iterator c = candidates.begin(); c != candidates.end(); c++) {

                bool result = true;
                for (Elements::iterator e = elements.begin(); e != elements.end(); e++) {
                    result = (*c)->assign(*e);
                    if (!result)
                        break;
                }

                if (result)
                    break;

                Operation::forEach(elements, Operation::unassign);
            }

            if (!Operation::filter(elements, Criteria::isUnassigned).empty())
                return false;

            for (Elements::iterator k = elements.begin(); k != elements.end(); k++) {
                LeafNode * node = (LeafNode *)(*k);
                node->setMigration(0);
            }
        }
    }

    return true;
}


//=================================FIRST_STAGE=================================
bool PrototypeAlgorithm::assignNodes(Elements & virtualNodes, Elements & pool, Request * r) {
    Elements nodes = Operation::filter(virtualNodes, Criteria::isUnassigned);
    while ( !nodes.empty() ) {
        Element * unassignedSeed = getSeedElement(r, nodes);
        std::deque<Element *> queue;
        queue.push_back(unassignedSeed);
        while ( !queue.empty() ) {
            Element * nextToAssign = queue.front();
            queue.pop_front();

            if ( nextToAssign->isAssigned() )
                continue;

            if ( nextToAssign->isSwitch() && !nextToAssign->isRouter())
                continue;

            // BFSRouter router(*r, nextToAssign);
            // bool result = false;

            // if ( router.isValid() ) {
            //     result = router.search();
            // } else {
            //     result = assignSeedElement(r, nextToAssign, pool);
            // }


            bool result = assignSeedElement(r, nextToAssign, pool);

            if ( !result ) {
                std::cout << "assign " << tenantsElements[nextToAssign][1] << " result == FALSE THEN EXHAUSTIVESEARCHER" << std::endl;
                nextToAssign->unassign();
                Elements assignedElements = Operation::filter(r->assignedElements(), Criteria::isComputational);
                if ( !exhaustiveSearch(nextToAssign, pool, assignedElements, true) ) {
                    return false;
                }
            }

            Elements adjacentNodes = Operation::filter(nextToAssign->adjacentNodes(), Criteria::isUnassigned);
            Elements adjacentEdges = nextToAssign->adjacentEdges();
            std::vector<Element * > tmp;
            tmp.insert(tmp.end(), adjacentEdges.begin(), adjacentEdges.end());
            std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightDescending);

            for(size_t e = 0; e < tmp.size(); e ++) {
                Edge * edge = tmp[e]->toEdge();
                for (Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++){
                    if (edge->connects(*i)) {
                        queue.push_back(*i);
                        adjacentNodes.erase(*i);
                        break;
                    }
                }
            }

            nodes.erase(nextToAssign);
        }
    }

    return true;
}


//=================SECOND_STAGE========================

bool PrototypeAlgorithm::assignLinks(Request * request) {
    Elements pool = network->getNodes();
    Elements edges = request->getTunnels();

    std::vector<Element *> tunnels;
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
            bool result = true;
            Elements assignedElements;
            std::map<Link *, Path> oldPathes;

            if (((LeafNode *)tunnel->getFirst()->getParentNode())->getMigration() != 0) {
                //safe old links assignment
                Elements virtualLinks = tunnel->getFirst()->getParentNode()->adjacentEdges();
                for (Elements::iterator l = virtualLinks.begin(); l != virtualLinks.end(); l++) {
                    Element *virtualLink = *l;
                    Link *tun = virtualLink->toLink();
                    if (!tun->isAssigned())
                        continue;

                    Path oldPath = tun->getRoute();
                    oldPathes[tun] = oldPath;
                }

                tunnel->getFirst()->getParentNode()->unassign();
                assignedElements = Operation::filter(request->assignedElements(), Criteria::isComputational);

                result = exhaustiveSearch(tunnel->getFirst()->getParentNode(), pool, assignedElements, false);

                if (result)
                    continue;
            }

            if (((LeafNode *)tunnel->getSecond()->getParentNode())->getMigration() != 0) {

                if (!result) {
                    //restore first node
                    if (!start->assign(tunnel->getFirst()->getParentNode()))
                        return false;
                    //restore old links assignment
                    for (std::map<Link *, Path>::iterator i = oldPathes.begin(); i != oldPathes.end(); i++) {
                        Link *link = i->first;
                        Path route = i->second;
                        std::vector<Element *> path = route.getPath();
                        for (size_t j = 0; j < path.size(); j++) {
                            if (path[j]->isLink())
                                path[j]->toLink()->assignLink(link);
                        }
                        link->setRoute(route);
                        link->setAssignedFlag(true);
                    }
                }

                tunnel->getSecond()->getParentNode()->unassign();

                assignedElements = Operation::filter(request->assignedElements(), Criteria::isComputational);
                result = exhaustiveSearch(tunnel->getSecond()->getParentNode(), pool, assignedElements, false);

                if (result)
                    continue;

            }

            return false;
        } else {
            Path route = searcher.getPath();
            tunnel->setRoute(route);
            tunnel->setAssignedFlag(true);
        }
    }

    std::cout << "++++++++++++++++++++++" << std::endl;
    return true;

}
