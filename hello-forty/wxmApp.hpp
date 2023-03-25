//
//  wxmApp.hpp
//  hello-forty
//
//  Created by 김진규 on 2023/03/26.
//

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


class wxmApp: public wxApp
{
public:
    virtual bool OnInit() override;
};
