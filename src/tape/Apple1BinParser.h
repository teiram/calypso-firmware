#ifndef APPLE1_BIN_PARSER_H
#define APPLE1_BIN_PARSER_H

#include "TzxTapeParser.h"

namespace calypso {

    class Apple1BinParser: public TzxTapeParser {
    public:

    private:
        static constexpr const char* TYPE {"BIN-APPLE1"};
    public: 
        Apple1BinParser();
        const char *type();
        bool insert(Stream &stream);
        void rewind(Stream &stream);
        void renderStep(PulseRenderer &pulseRenderer, Stream &stream);
        const char *currentStatus();
    };

}

#endif //APPLE1_BIN_PARSER_H
