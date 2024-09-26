#include "Service.h"

Service* Service::services[MAX_SERVICES];
int Service::serviceCount = 0;

void Service::registerService(Service *service) {
    if (serviceCount < MAX_SERVICES) {
        services[serviceCount++] = service;
    } else {
        printf("Trying to register too many services\n");
    }
}