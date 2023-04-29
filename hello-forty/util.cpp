#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util.h"

int Util::getValue(std::string& v) {
    if(v.compare("A") == 0)
        return 1;
    else if(
          v.compare("2") == 0
       || v.compare("3") == 0
       || v.compare("4") == 0
       || v.compare("5") == 0
       || v.compare("6") == 0
       || v.compare("7") == 0
       || v.compare("8") == 0
       || v.compare("9") == 0
       || v.compare("10") == 0
    )
        return atoi(v.c_str());
    else if(v.compare("J") == 0)
        return 11;
    else if(v.compare("Q") == 0)
        return 12;
    else if(v.compare("K") == 0)
        return 13;
    return -1;
}

Suit Util::getSuit(std::string& v) {
    char c = tolower(v.c_str()[0]);
    if(c == 's')
        return spades;
    else if(c == 'h')
        return hearts;
    else if(c == 'd')
        return diamonds;
    else if(c == 'c')
        return clubs;
    return undefined;
}