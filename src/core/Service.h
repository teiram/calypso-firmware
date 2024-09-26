#ifndef SERVICE_H
#define SERVICE_H
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>

class Service {
private:
    static constexpr uint8_t MAX_SERVICES = 16;
public:
    static Service* services[MAX_SERVICES];
    static int serviceCount;
    static void registerService(Service *service);

    virtual const char* name() = 0;
    virtual bool init() = 0;
    virtual void cleanup() = 0;
    virtual bool needsAttention() = 0;
    virtual void attention() = 0;
};

#endif //SERVICE_H