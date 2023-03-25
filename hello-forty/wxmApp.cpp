//
//  wxmApp.cpp
//  hello-forty
//
//  Created by 김진규 on 2023/03/26.
//

#include "wxmApp.hpp"
#include "wxmView.hpp"


bool wxmApp::OnInit() {
    auto frame = new wxmView;
    frame->Show(true);
    return true;
}
