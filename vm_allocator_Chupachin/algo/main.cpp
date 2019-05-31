#include <stdio.h>
#include <unistd.h>
#include <vector>
#include "interface/snapshot.h"
#include "interface/elementfactory.h"
#include "prototype/prototype.h"


#include "defs.h"
#include "request.h"
#include "network.h"
#include "operation.h"
#include "criteria.h"

#include <QString>
#include <ctime>
#include <iostream>
#include <sstream>

void check_input();
template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}

enum ReturnCodes {
   SUCCESS = 0,
   PARTIAL_FAILURE,
   INVALID_INPUT,
   INVALID_USAGE
};

int main(int argc, char ** argv)
{
    //check_input();
    //print current data and time
    time_t t = time(0);
    struct tm * now = localtime(&t);
    std::string result = ToString<int>(now->tm_year + 1900) + std::string("-") +
                         ToString<int>(now->tm_mon + 1) + std::string("-") +
                         ToString<int>(now->tm_mday ) + std::string(".") +
                         ToString<int>(now->tm_hour ) + std::string(":") +
                         ToString<int>(now->tm_min ) + std::string(":") +
                         ToString<int>(now->tm_sec );
    std::cout << "Current time -> " << result << std::endl;
    //
    int hour_start = now->tm_hour;
    int min_start = now->tm_min;
    int sec_start = now->tm_sec;
    //

    if ( argc < 2 )
    {
        printf("Usage: %s <input file> <output file xml> <output file huawei>\n", *argv);
        return INVALID_USAGE;
    }

    Snapshot snapshot;
    if ( !snapshot.read(argv[1]) )
       return INVALID_INPUT;

    std::string comment = std::string("Before_scheduling");
    //snapshot.print(comment);

    Requests requests = snapshot.getRequests();
    PrototypeAlgorithm algorithm(snapshot.getNetwork(), requests);

    algorithm.setResources(snapshot.getResources());
    algorithm.setTenants(snapshot.getTenants());

    std::cout << "BEFORE algorithm.schedule();" << std::endl;
    algorithm.schedule();
    std::cout << "AFTER algorithm.schedule();" << std::endl;

    int assignedRequests = 0;
    int notAssignedRequests = 0;
    int nodeAssignedRequests = 0;
    bool success = 0;
    for ( Requests::iterator i = requests.begin(); i != requests.end(); i++ ) {
        Request * r = *i;
        if ( r->isAssigned() )
            success = true;
            assignedRequests++;
        if ( r-> notAssigned())
            success = false;
            notAssignedRequests++;
        Elements unassignedComputational = Operation::filter(r->elementsToAssign(), Criteria::isComputational);
        if ( unassignedComputational.empty() )
            nodeAssignedRequests++;

    }
    if (success) {
        std::cout << "vm created!!!" << std::endl;
    }

    //snapshot.write(argv[2]);
    std::cout << "Before printResultsInHuaweiStyle" << std::endl;
    //snapshot.printResultsInHuaweiStyle(argv[3]);

    //print current data and time
    t = time(0);
    now = localtime(&t);
    result = ToString<int>(now->tm_year + 1900) + std::string("-") +
             ToString<int>(now->tm_mon + 1) + std::string("-") +
             ToString<int>(now->tm_mday ) + std::string(".") +
             ToString<int>(now->tm_hour ) + std::string(":") +
             ToString<int>(now->tm_min ) + std::string(":") +
             ToString<int>(now->tm_sec );
    std::cout << "Current time -> " << result << std::endl;
    //
    int time = now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec - hour_start * 3600 - min_start * 60 - sec_start;
    std::cout << "Algorithm work time -> " <<  time / 3600  << "  hours " << (time % 3600) / 60  << " mins " << (time % 3600) % 60 << " secs" << std::endl;
    //

    {
        //PRINT ALL VIRTUAL RESOURCES
        std::cout << std::endl;
        std::cout << "All virtual resources ";

        //PRINT VM PARAMETERS
        std::map<std::string, double> vmParams = (*requests.begin())->getAllVMsParameters();
        Requests::iterator firstReq = ++requests.begin();
        Requests::iterator lastReq = requests.end();
        for (Requests::iterator i = firstReq; i != lastReq; i++) {
            Request *request = *i;
            std::map<std::string, double> reqParams = request->getAllVMsParameters();

            std::map<std::string, double>::iterator f = reqParams.begin();
            std::map<std::string, double>::iterator l = reqParams.end();
            for (std::map<std::string, double>::iterator j = f; j != l; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                vmParams[paramName] += paramValue;
            }

        }
        //

        //PRINT STORAGE PARAMETERS
        std::map<std::string, double> stParams = (*requests.begin())->getAllStParameters();
        firstReq = ++requests.begin();
        lastReq = requests.end();
        for (Requests::iterator i = firstReq; i != lastReq; i++) {
            Request *request = *i;
            //std::cout <<  request->getName() << std::endl;
            std::map<std::string, double> reqParams = request->getAllStParameters();

            std::map<std::string, double>::iterator f = reqParams.begin();
            std::map<std::string, double>::iterator l = reqParams.end();
            for (std::map<std::string, double>::iterator j = f; j != l; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                stParams[paramName] += paramValue;
            }

        }
        //

        std::map<std::string, double>::iterator first = vmParams.begin();
        std::map<std::string, double>::iterator last = vmParams.end();
        for (std::map<std::string, double>::iterator i = first; i != last; i++) {
            std::cout << i->first << ' ' << i->second <<' ';
        }

        first = stParams.begin();
        last = stParams.end();
        for (std::map<std::string, double>::iterator i = first; i != last; i++) {
            std::cout << i->first << ' ' << i->second <<' ';
        }

        //PRINT LINK'S INFORMATION
        std::cout << std::endl;
        double sumThroughput = 0.0;
        for (Requests::iterator k = requests.begin(); k != requests.end(); k++) {
            sumThroughput += (*k)->getAllLinksParameters();
        }
        std::cout << "Sum virtual throughput " << sumThroughput << std::endl;
        //

    }

    std::cout << std::endl;
    std::cout << "All physical resources" << ' ';
    //SERVERS
    std::map<std::string, double> allPhysicalParams = snapshot.getNetwork()->getServersParameters();
    std::map<std::string, double>::iterator first = allPhysicalParams.begin();
    std::map<std::string, double>::iterator last = allPhysicalParams.end();
    for (std::map<std::string, double>::iterator i = first; i != last; i++ ){
        std::cout << i->first << ' ' << i->second << ' ';
    }
    //

    //STORAGES
    std::map<std::string, double>  allStPhysicalParams = snapshot.getNetwork()->getStoragesParameters();
    first = allStPhysicalParams.begin();
    last = allStPhysicalParams.end();
    for (std::map<std::string, double>::iterator i = first; i != last; i++ ){
        std::cout << i->first << ' ' << i->second << ' ';
    }
    //

    //PRINT LINK'S INFORMATION
    std::cout << std::endl;
    double sumThroughput = 0.0;
    sumThroughput = snapshot.getNetwork()->getLinksParameters();
    std::cout << "Sum physical throughput " << sumThroughput << std::endl;

    //virtual resources assigned successfully
    //and utilization rate of physical resources
    std::cout << std::endl;
    std::cout << "Used physical resources" << ' ';
    //Servers
    std::map<std::string, double> usedPhysicalParams = snapshot.getNetwork()->getUsedServersParameters();
    std::map<std::string, double>::iterator firstParam = usedPhysicalParams.begin();
    std::map<std::string, double>::iterator lastParam = usedPhysicalParams.end();
    for (std::map<std::string, double>::iterator i = firstParam; i != lastParam; i++ ){
        std::cout << i->first << ' ' << i->second << ' ';
    }
    //

    //Storages
    std::map<std::string, double> usedStPhysicalParams = snapshot.getNetwork()->getUsedStoragesParameters();
    firstParam = usedStPhysicalParams.begin();
    lastParam = usedStPhysicalParams.end();
    for (std::map<std::string, double>::iterator i = firstParam; i != lastParam; i++ ){
        std::cout << i->first << ' ' << i->second << ' ';
    }
    //

    //Print all link's information
    std::cout << std::endl;
    double sumUsedThroughput = snapshot.getNetwork()->getUsedLinksParameters();
    std::cout << "Used physical throughput  " << sumUsedThroughput << std::endl;
    //

    //utilization of physical resources
    std::cout << std::endl;
    for (std::map<std::string, double>::iterator virt = usedPhysicalParams.begin(), phys = allPhysicalParams.begin();
            virt != usedPhysicalParams.end() || phys != allPhysicalParams.end(); virt++, phys++) {
        std::cout << virt->first << " utilization " << virt->second / phys->second << ' ';
    }

    for (std::map<std::string, double>::iterator virt = usedStPhysicalParams.begin(), phys = allStPhysicalParams.begin();
         virt != usedStPhysicalParams.end() || phys != allStPhysicalParams.end(); virt++, phys++) {
        std::cout << virt->first << " utilization " << virt->second / phys->second << ' ';
    }

    //Network resources utilization
    std::cout << "Network utilization " << sumUsedThroughput / sumThroughput << std::endl;
    //



    if (argc >=5 && std::string(argv[4]) == std::string("numa_stat")) {
        // Performance statistics
        // This statistics methods must launch when
        // In Computer::assign work common assignment procedure
        // 'return LeafNode::assign(other)'
        std::cout << std::endl;
        std::cout << "Performance statistics " << std::endl;
        Network * physNetwork = snapshot.getNetwork();

        std::string param = std::string("VCPUs");
        std::vector<int> performanceStat = physNetwork->getLostPerformance(param);
        std::cout << param << std::endl;
        std::cout << "    ALL: " <<  performanceStat[0] << std::endl;
        std::cout << "    HIGH: " << performanceStat[0] - performanceStat[1] << std::endl;
        std::cout << "    LOW: " <<  performanceStat[1] << std::endl;

        std::cout << std::endl;

        param = std::string("RAM");
        performanceStat = physNetwork->getLostPerformance(param);
        std::cout << param << std::endl;
        std::cout << "    ALL: " <<  performanceStat[0] << std::endl;
        std::cout << "    HIGH: " << performanceStat[0] - performanceStat[1] << std::endl;
        std::cout << "    LOW: " <<  performanceStat[1] << std::endl;
        std::cout << std::endl;
        //
    }

    printf("Requests all -> %lu\n", requests.size());
    printf("Requests assigned -> %u\n", assignedRequests);
    printf("Requests not assigned -> %u\n", notAssignedRequests);
    printf("\nPrint rules statistics\n");
    snapshot.printRulesStat();
    ElementFactory::deleteParameters();


    if ( nodeAssignedRequests != requests.size())
        return PARTIAL_FAILURE;

    return SUCCESS;
}

void check_input() {
    srand(time(0));
    std::cout << rand() % 1000 << std::endl;
    usleep(1e6);
    exit(0);
}
