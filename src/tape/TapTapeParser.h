#ifndef TAP_TAPE_PARSER_H
#define TAP_TAPE_PARSER_H

#include "TzxTapeParser.h"

namespace calypso {

    class TapTapeParser: public TzxTapeParser {
    public:

    private:
        static constexpr const char* TYPE = {"TAP"};
    public: 
        TapTapeParser();
        const char *type();
        bool insert(Stream &stream);
        void rewind(Stream &stream);
        void renderStep(PulseRenderer &pulseRenderer, Stream &stream);
    };

}

#endif //TAP_TAPE_PARSER_H