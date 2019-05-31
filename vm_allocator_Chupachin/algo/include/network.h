#pragma once

#include "graph.h"
#include "element.h"
#include "operation.h"
#include "criteria.h"
#include "computer.h"
#include "numa_block.h"
#include "link.h"
#include <stdio.h>
#include <algorithm>
#include <string>

#include "interface/elementfactory.h"

class Network : public Graph {
public:
    Network(const Elements & e) {
        elements = Operation::filter(e, Criteria::isPhysical);
	    migrationTime = 100;
    }

    inline Elements availableElements() const {
        return Operation::filter(getElements(), Criteria::isAvailable);
    }

    inline Elements getComputers() const {
        return Operation::filter(getElements(), Criteria::isComputer);
    }

    inline Elements getStores() const {
        return Operation::filter(getElements(), Criteria::isStore);
    }

    inline Elements getSwitches() const {
        return Operation::filter(getElements(), Criteria::isSwitch);
    }

    inline Elements getLinks() const {
        return Operation::filter(getElements(), Criteria::isLink);
    }
    
    void setMigrationTime(const unsigned & m) {
	    migrationTime = m;
    }
   
    unsigned getMigrationTime() {
	    return migrationTime;
    }

    std::map<std::string, double> getServersParameters() {
        std::map<std::string, double> result;
        Elements computers = getComputers();
        if (computers.size() == 0)
            return result;
        result = (*computers.begin())->getParametersTotal();
        Elements::iterator first = ++computers.begin();
        Elements::iterator back = computers.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * computer = *i;
            std::map<std::string, double> params = computer->getParametersTotal();
            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }

        }
        return result;
    }

    std::map<std::string, double> getUsedServersParameters() {
        std::map<std::string, double> result;
        Elements computers = getComputers();
        if (computers.size() == 0)
            return result;
        result = (*computers.begin())->getParametersUsed();
        Elements::iterator first = ++computers.begin();
        Elements::iterator back = computers.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * computer = *i;
            std::map<std::string, double> params = computer->getParametersUsed();
            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }

        }
        return result;
    }

    std::map<std::string, double> getStoragesParameters() {
        std::map<std::string, double> result;
        Elements storages = getStores();
        if (storages.size() == 0)
            return result;
        result = (*storages.begin())->getParametersTotal();
        Elements::iterator first = ++storages.begin();
        Elements::iterator back = storages.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * storage = *i;
            std::map<std::string, double> params = storage->getParametersTotal();
            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }
        }
        return result;
    }

    std::map<std::string, double> getUsedStoragesParameters() {
        std::map<std::string, double> result;
        Elements storages = getStores();
        if (storages.size() == 0)
            return result;
        result = (*storages.begin())->getParametersTotal();
        Elements::iterator first = ++storages.begin();
        Elements::iterator back = storages.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * storage = *i;
            std::map<std::string, double> params = storage->getParametersUsed();
            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }
        }
        return result;
    }

    double getLinksParameters() {
        double result = 0.0;
        Elements links = getLinks();
        Elements::iterator first = links.begin();
        Elements::iterator back = links.end();
        for (Elements::iterator i = first; i != back; i++) {
            result += static_cast<double>(((Link *)(*i))->getFullThroughput());
        }
        return result;
    }

    double getUsedLinksParameters() {
        double result = 0.0;
        Elements links = getLinks();
        Elements::iterator first = links.begin();
        Elements::iterator back = links.end();
        for (Elements::iterator i = first; i != back; i++) {
            result += static_cast<double>((*i)->toLink()->getFullThroughput() -
                    (*i)->toLink()->getThroughput());
        }
        return result;
    }


    static bool sortElementsByRAMDescending(Element * first, Element * second) {
        int first_ram = 0;
        int second_ram = 0;
        
        Parameters parameters = first->parameters;
        for (Parameters::iterator i = parameters.begin() ; i != parameters.end(); ++i ) {
            Parameter* type = i->first;
            ParameterValue* value = i->second;
            if (type->getName() == std::string("RAM")) {
                first_ram = value->weight();
                break;
            }
        }

        parameters = second->parameters;
        for (Parameters::iterator i = parameters.begin() ; i != parameters.end(); ++i ) {
            Parameter* type = i->first;
            ParameterValue* value = i->second;
            if (type->getName() == std::string("RAM")) {
                second_ram = value->weight();
                break;
            }
        }

        return first_ram > second_ram;
    }

    static bool sortElementsByCPUDescending(Element * first, Element * second) {
        int first_cpu = 0;
        int second_cpu = 0;
        
        Parameters parameters = first->parameters;
        for (Parameters::iterator i = parameters.begin() ; i != parameters.end(); ++i ) {
            Parameter* type = i->first;
            ParameterValue* value = i->second;
            if (type->getName() == std::string("VCPUs")) {
                first_cpu = value->weight();
                break;
            }
        }

        parameters = second->parameters;
        for (Parameters::iterator i = parameters.begin() ; i != parameters.end(); ++i ) {
            Parameter* type = i->first;
            ParameterValue* value = i->second;
            if (type->getName() == std::string("VCPUs")) {
                second_cpu = value->weight();
                break;
            }
        }

        return first_cpu > second_cpu;
    }

    static bool descending_sort(const int & first, const int & second) {
        return first > second;
    }

    std::vector<int> getLostPerformance(const std::string & param) {
        std::vector <int> result;
        int vm_count = 0;
        int lost_performance = 0;
        Elements servers = getComputers();
         
        if ((param != std::string("VCPUs")) && (param != std::string("RAM"))) {
            printf("[ERROR] getLostPerformance method 'param' error");
            return result;
        }

        for (Elements::iterator i = servers.begin(); i != servers.end(); i++) { 
            Element * server = (*i);
            int numa_part = server->toComputer()->getNumaPart(param);
            Elements vms_set = server->getAssignments();

            if (numa_part == 0) {
                vm_count += vms_set.size();
                continue;
            }            

            std::vector<int> vms_parameters;
            
            for (Elements::iterator j = vms_set.begin(); j != vms_set.end(); j++) {
                Element * vm = (*j);
                Parameter * type = ElementFactory::parameterByName(param);
                ParameterValue* value = vm->getParameterValue(type);
                vms_parameters.push_back(static_cast<int>(value->weight()));
            }
            
            std::sort(vms_parameters.begin(), vms_parameters.end(), descending_sort);
            vm_count += vms_set.size();
            
            int start_numa =  0;
            int current = 0;
            for (uint j = 0; j < vms_parameters.size(); j++) {
                current += vms_parameters[j];
                if (current > (start_numa + numa_part)) {
                    // VM intersect NUMA "border"
                    lost_performance++;
                    start_numa = (current / numa_part)*numa_part;
                }
            }
        }

        result.push_back(vm_count);
        result.push_back(lost_performance);

        return result;
    }

private:
	unsigned migrationTime;
};
