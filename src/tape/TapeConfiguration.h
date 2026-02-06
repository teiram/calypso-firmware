#ifndef TAPE_CONFIGURATION_H
#define TAPE_CONFIGURATION_H

namespace calypso {

    struct TapeConfiguration {
        public:
            bool initialLevel;
            bool reverseLevel;
            bool senseMotor;
    };
}

#endif //TAPE_CONFIGURATION_H