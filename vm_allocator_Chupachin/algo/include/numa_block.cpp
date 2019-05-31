#include "numa_block.h"
#include "parameter.h"
#include "interface/export.h"
#include "element.h"
#include "computer.h"


NumaBlock::~NumaBlock() {
    //delete all numa parameters
    for (Parameters::iterator i = parameters.begin(); i != parameters.end(); i++)
        delete i->second;
}


double NumaBlock::weight() const {
    double result = 0.0;
    for (Parameters::const_iterator it = parameters.begin(); it != parameters.end(); it ++) {
        ParameterValue * value = it->second;
        Parameter* par = it->first;
        result += value->weight() / EF::getMaxValue(par) * EF::getDeficit(par);
    }

    return result;
}

bool NumaBlock::assign(Element * other) {
    if (!canAssign(other)) {
        // std::cout << "canAssign return false" << std::endl;
        return false;
    }

    // std::cout << "AFTER canAssign()" << std::endl;
    printNumaInfo();

    //CHECK if virtual element has already assign on this numaBlock
    Elements::iterator a = assignments.find(other);
    if ( a != assignments.end() )
        return true;

    /* Server NB
         -    -  | good sitation
         -    +  | 
         +    -  | 
         +    +  | 
       */

    
    if ( !( other->isAssigned() || other->toComputer()->isNumaAssigned())  ) {
        //OK

    } else if ( !other->isAssigned() && other->toComputer()->isNumaAssigned()) {
        // This situation is impossible. VM is assigned to remote NB, but isn't assigned
        // to the remote server.
        // printf("[ERROR]: VM is assigned to remote NB, but isn't assigned to the remote server.\n");
        NumaBlock * nm = other->toComputer()->nm;
        nm->unassign(other);
    } else if ( other->isAssigned() && !other->toComputer()->isNumaAssigned()) {
        // In exhaustive search this situation can exist.
        // Check that server on which other was assigned contains NB(this) or not.
        if (other->getAssignee()->toComputer()->numaBlocks.find(this) !=
            other->getAssignee()->toComputer()->numaBlocks.end()) {
            // NB(this) is located in server
            // This situation is possible in exhaustive search
            // Nothing DONE
        } else {
            // printf("[ERROR]: VM is assigned to remote server, but isn't assigned to the remote NB.\n");
            other->unassign();
        }
    } else {
        // ( other->isAssigned() && other->toComputer()->isNumaAssigned())
        // VM was assigned to not current NB. See above 'CHECK' in the 
        // beginning of this method
        NumaBlock * nm = other->toComputer()->nm;
        nm->unassign(other);
        other->unassign();
    }

    // //FIRST UNASSIGN from old location
    // if (other->isAssigned()) {
    //     std::cout << "IN NumaBlock::assign. unassign other" << std::endl;
    //     NumaBlock * nm = other->toComputer()->nm;
    //     nm->unassign(other);

    //     if (other->getAssignee()->toComputer()->numaBlocks.find(this) ==
    //         other->getAssignee()->toComputer()->numaBlocks.end()) {
    //         other->unassign();
    //     }
    // }


    // std::cout << "BEFORE decreaseResources" << std::endl;
    printNumaInfo();

    decreaseResources(other);

    other->toComputer()->nm = this;
    assignments.insert(other);

    // std::cout << "AFTER decreaseResources" << std::endl;
    printNumaInfo();

    return true;
}

void NumaBlock::unassign(Element * other) {
    //UNASSIGN ONLY FROM NUMABLOCK
    if ( other->toComputer()->nm == 0 )
        return;

    removeAssignment(other);
}

bool NumaBlock::canAssign(Element * other) {
    //CHECK ONLY NUMA PARAMETERS: RAM and VCPUs

    // std::cout << "IN NumaBlock::canAssign before loop" << std::endl;
    printNumaInfo();

    for (Parameters::iterator it = parameters.begin(); it != parameters.end(); it++) {
        Parameter* type = it->first;
        ParameterValue* value = it->second;

        if ( other->parameters.find(type) == other->parameters.end() )
            return false;

        if ( !value->compare(other->parameters.at(type)) )
            return false;
    }

    // std::cout << "IN NumaBlock::canAssign after loop" << std::endl;
    printNumaInfo();

    return true;

}


void NumaBlock::decreaseResources(const Element * other) {


    // std::cout << "IN NumaBlock::decreaseResources(const Element * other)" << std::endl;

    other->toComputer()->printParametersInfo();


    Parameters::iterator it = parameters.begin();
    for ( ; it != parameters.end(); ++it ) {
        Parameter* type = it->first;
        ParameterValue* value = it->second;
        if (other->parameters.find(type) != other->parameters.end()) {
            // std::cout << "decrease parameters" << std::endl;
            value->decrease(other->parameters.at(type));
            // std::cout << value->weight() << std::endl;
        }
    }
}

void NumaBlock::removeAssignment(Element * other) {
    Elements::iterator a = assignments.find(other);
    if ( a == assignments.end() )
        return;

    restoreResources(other);

    other->toComputer()->nm = 0;
    assignments.erase(a);
}

void NumaBlock::restoreResources(const Element * other)  {
    Parameters::iterator it = parameters.begin();
    for ( ; it != parameters.end(); ++it ) {
        Parameter* type = it->first;
        ParameterValue* value = it->second;
        if (other->parameters.find(type) != other->parameters.end()) {
            value->increase(other->parameters.at(type));
        }
    }
}


std::map<std::string, double> NumaBlock::getParametersUsed() {
    std::map<std::string, double> result;

    Parameters::iterator firstParam = parameters.begin();
    Parameters::iterator lastParam = parameters.end();

    for (Parameters::iterator param = firstParam; param != lastParam; param++ ) {
        result[param->first->getName()] = 0.0;
    }

    Elements::iterator first = assignments.begin();
    for ( Elements::iterator virtElem = first; virtElem != assignments.end(); virtElem++ ) {
        std::map<std::string, double> virtElemParameters = (*virtElem)->getParametersUsed();
        std::map<std::string, double>::iterator firstParam = virtElemParameters.begin();
        std::map<std::string, double>::iterator lastParam = virtElemParameters.end();
        for ( std::map<std::string, double>::iterator param = firstParam; param != lastParam; param++ ) {
            std::string paramName = param->first;
            double paramValue = param->second;

            if ( result.find(paramName) == result.end())
                continue;

            result[paramName] += paramValue;
        }
    }

    return result;
}

std::map<std::string, double> NumaBlock::getParametersTotal() {
    std::map<std::string, double> result;

    //Physical resources
    //The remaining resources
    Parameters::iterator phBegin = parameters.begin();
    Parameters::iterator phEnd = parameters.end();
    for ( Parameters::iterator param = phBegin; param != phEnd; param++ ) {
        std::string paramName = param->first->getName();
        double paramValue = param->second->weight();
        result[paramName] = paramValue;
    }

    //Used resources
    std::map<std::string, double> usedResources = getParametersUsed();
    std::map<std::string, double>::iterator firstParam = usedResources.begin();
    std::map<std::string, double>::iterator lastParam = usedResources.end();
    for ( std::map<std::string, double>::iterator param = firstParam; param != lastParam; param++ ) {
        std::string paramName = param->first;
        double paramValue = param->second;
        if ( result.find(paramName) != result.end() )
            result[paramName] += paramValue;
    }

    return result;
}


std::map<std::string, double> NumaBlock::getAvailableParameters() {
    std::map<std::string, double> result;
    Parameters::iterator phBegin = parameters.begin();
    Parameters::iterator phEnd = parameters.end();
    for ( Parameters::iterator param = phBegin; param != phEnd; param++ ) {
        std::string paramName = param->first->getName();
        double paramValue = param->second->weight();
        result[paramName] = paramValue;
    }

    return result;
}


void NumaBlock::printNumaInfo() {
    // std::cout << std::endl;
    // std::cout << std::endl;
    // std::map<std::string, double> numaTotal = getParametersTotal();
    // std::map<std::string, double> numaUsed = getParametersUsed();
    // std::map<std::string, double> numaAvailable = getAvailableParameters();

    // std::cout << "TOTAL RESOURCES NUMA" << this << std::endl;

    // for (std::map<std::string, double>::iterator j = numaTotal.begin(); j != numaTotal.end(); j++) {
    //     std::cout << j->first << ' ' << j->second<< std::endl;
    // }

    // std::cout << std::endl;

    // std::cout << "USED RESOURCES NUMA" << this << std::endl;

    // for (std::map<std::string, double>::iterator j = numaUsed.begin(); j != numaUsed.end(); j++) {
    //     std::cout << j->first << ' ' << j->second<< std::endl;
    // }

    // std::cout << std::endl;

    // std::cout << "AVAILABLE RESOURCES NUMA" << this << std::endl;

    // for (std::map<std::string, double>::iterator j = numaAvailable.begin(); j != numaAvailable.end(); j++) {
    //     std::cout << j->first << ' ' << j->second<< std::endl;
    // }


    // std::cout << std::endl;
    // std::cout << std::endl;
}
