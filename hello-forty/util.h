// #include <stdio.h>
// #include <string.h>
// #include <stddef.h>
// #include <cctype>
#include "card.h"

class Util {
public:
    virtual ~Util();
public:
    static int getValue(std::string& v);
    static Suit getSuit(std::string& v);
};