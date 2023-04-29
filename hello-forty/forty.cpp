/////////////////////////////////////////////////////////////////////////////
// Name:        forty.cpp
// Purpose:     Forty Thieves patience game
// Author:      Chris Breeze
// Modified by:
// Created:     21/07/97
// Copyright:   (c) 1993-1998 Chris Breeze
// Licence:     wxWindows licence
//---------------------------------------------------------------------------
// Last modified: 22nd July 1998 - ported to wxWidgets 2.0
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "game.h"
#include "canvas.h"
#include "forty.h"
#include "card.h"
#include "scoredg.h"
#include "forty.xpm"

#if wxUSE_HTML
#include "wx/textfile.h"
#include "wx/html/htmlwin.h"
#endif

#include "wx/stockitem.h"

wxBEGIN_EVENT_TABLE(FortyFrame, wxFrame)
    EVT_MENU(wxID_NEW, FortyFrame::NewGame)
    EVT_MENU(wxID_EXIT, FortyFrame::Exit)
    EVT_MENU(wxID_ABOUT, FortyFrame::About)
    EVT_MENU(wxID_HELP_CONTENTS, FortyFrame::Help)
    EVT_MENU(wxID_UNDO, FortyFrame::Undo)
    EVT_MENU(wxID_REDO, FortyFrame::Redo)
    EVT_MENU(SCORES, FortyFrame::Scores)
    EVT_MENU(RIGHT_BUTTON_UNDO, FortyFrame::ToggleRightButtonUndo)
    EVT_MENU(HELPING_HAND, FortyFrame::ToggleHelpingHand)
    EVT_MENU(LARGE_CARDS, FortyFrame::ToggleCardSize)
    EVT_MENU(QUICK_N, FortyFrame::OnAskQuick)
    EVT_MENU(QUICK_1, FortyFrame::QuickStep)
    EVT_MENU(QUICK_2, FortyFrame::QuickStep)
    EVT_MENU(QUICK_3, FortyFrame::QuickStep)
    EVT_MENU(QUICK_4, FortyFrame::QuickStep)
    EVT_MENU(QUICK_5, FortyFrame::QuickStep)
    EVT_MENU(QUICK_6, FortyFrame::QuickStep)
    EVT_MENU(QUICK_7, FortyFrame::QuickStep)
    EVT_MENU(QUICK_8, FortyFrame::QuickStep)
    EVT_MENU(QUICK_9, FortyFrame::QuickStep)
    EVT_MENU(QUICK_0, FortyFrame::QuickStep)
    EVT_CLOSE(FortyFrame::OnCloseWindow)
wxEND_EVENT_TABLE()

// Create a new application object
wxIMPLEMENT_APP(FortyApp);

wxColour* FortyApp::m_backgroundColour = 0;
wxColour* FortyApp::m_textColour = 0;
wxBrush*  FortyApp::m_backgroundBrush = 0;

FortyApp::~FortyApp() {
    delete m_backgroundColour;
    delete m_textColour;
    delete m_backgroundBrush;
    delete Card::m_symbolBmap;
    delete Card::m_pictureBmap;
}

bool FortyApp::OnInit() {
    bool largecards = false;
    m_helpFile = wxGetCwd() + wxFILE_SEP_PATH + wxT("about.htm");
    if(!wxFileExists(m_helpFile)) {
        m_helpFile = wxPathOnly(argv[0]) + wxFILE_SEP_PATH + wxT("about.htm");
    }

    wxSize size(668,510);

    if((argc > 1) && (!wxStrcmp(argv[1], wxT("-L")))) {
        largecards = true;
        size = wxSize(1000,750);
    }

    FortyFrame* frame = new FortyFrame(
            0,
            wxT("Forty Thieves"),
            wxDefaultPosition,
            size,
            largecards
    );

    // Show the frame
    frame->Show(true);
    frame->GetCanvas()->ShowPlayerDialog();
    return true;
}

const wxColour& FortyApp::BackgroundColour() {
    if(!m_backgroundColour) {
        m_backgroundColour = new wxColour(0, 128, 0);
    }

    return *m_backgroundColour;
}

const wxBrush& FortyApp::BackgroundBrush() {
    if(!m_backgroundBrush) {
        m_backgroundBrush = new wxBrush(BackgroundColour());
    }

    return *m_backgroundBrush;
}

const wxColour& FortyApp::TextColour() {
    if(!m_textColour) {
        m_textColour = new wxColour(*wxBLACK);
    }

    return *m_textColour;
}

// My frame constructor
FortyFrame::FortyFrame(wxFrame* frame, const wxString& title, const wxPoint& pos, const wxSize& size, bool largecards):
    wxFrame(frame, wxID_ANY, title, pos, size) {
#ifdef __WXMAC__
    wxApp::s_macAboutMenuItemId = wxID_ABOUT ;
#endif
    // set the icon
#ifdef __WXMSW__
    SetIcon(wxIcon(wxT("CardsIcon")));
#else
    SetIcon(wxIcon(forty_xpm));
#endif

    // Make a menu bar
    wxMenu* gameMenu = new wxMenu;
    gameMenu->Append(wxID_NEW, wxGetStockLabel(wxID_NEW), wxT("Start a new game"));
    gameMenu->Append(SCORES, wxT("&Scores..."), wxT("Displays scores"));
    gameMenu->Append(wxID_EXIT, wxGetStockLabel(wxID_EXIT), wxT("Exits Forty Thieves"));

    wxMenu* editMenu = new wxMenu;
    editMenu->Append(wxID_UNDO, wxGetStockLabel(wxID_UNDO), wxT("Undo the last move"));
    editMenu->Append(wxID_REDO, wxGetStockLabel(wxID_REDO), wxT("Redo a move that has been undone"));

    wxMenu* optionsMenu = new wxMenu;
    optionsMenu->Append(RIGHT_BUTTON_UNDO,
            wxT("&Right button undo"),
            wxT("Enables/disables right mouse button undo and redo"),
            true
            );
    optionsMenu->Append(HELPING_HAND,
            wxT("&Helping hand"),
            wxT("Enables/disables hand cursor when a card can be moved"),
            true
            );
    optionsMenu->Append(LARGE_CARDS,
            wxT("&Large cards"),
            wxT("Enables/disables large cards for high resolution displays"),
            true
            );
    optionsMenu->Check(HELPING_HAND, true);
    optionsMenu->Check(RIGHT_BUTTON_UNDO, true);
    optionsMenu->Check(LARGE_CARDS, largecards ? true : false);

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_HELP_CONTENTS, wxT("&Help Contents"), wxT("Displays information about playing the game"));
    helpMenu->Append(wxID_ABOUT, wxT("&About"), wxT("About Forty Thieves"));

    wxMenu* quickMenu = new wxMenu;
    quickMenu->Append(QUICK_N, wxT("Go &N\tCtrl-N"));
    quickMenu->Append(QUICK_1, wxT("Step &1\tCtrl-1"));
    quickMenu->Append(QUICK_2, wxT("Step &2\tCtrl-2"));
    quickMenu->Append(QUICK_3, wxT("Step &3\tCtrl-3"));
    quickMenu->Append(QUICK_4, wxT("Step &4\tCtrl-4"));
    quickMenu->Append(QUICK_5, wxT("Step &5\tCtrl-5"));
    quickMenu->Append(QUICK_6, wxT("Step &6\tCtrl-6"));
    quickMenu->Append(QUICK_7, wxT("Step &7\tCtrl-7"));
    quickMenu->Append(QUICK_8, wxT("Step &8\tCtrl-8"));
    quickMenu->Append(QUICK_9, wxT("Step &9\tCtrl-9"));
    quickMenu->Append(QUICK_0, wxT("Step &0\tCtrl-0"));

    m_menuBar = new wxMenuBar;
    m_menuBar->Append(gameMenu,    wxT("&Game"));
    m_menuBar->Append(editMenu,    wxT("&Edit"));
    m_menuBar->Append(optionsMenu, wxT("&Options"));
    m_menuBar->Append(helpMenu,    wxT("&Help"));
    m_menuBar->Append(quickMenu,   wxT("quic&K"));

    SetMenuBar(m_menuBar);

    if(largecards)
        Card::SetScale(1.3);

    m_canvas = new FortyCanvas(this, wxDefaultPosition, size);

    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(m_canvas, 1, wxEXPAND | wxALL, 0);
    SetSizer(topsizer);

#if wxUSE_STATUSBAR
    CreateStatusBar();
#endif // wxUSE_STATUSBAR

    topsizer->SetSizeHints(this);
}

void FortyFrame::OnCloseWindow(wxCloseEvent& event) {
    if(m_canvas->OnCloseCanvas()) {
        this->Destroy();
    } else
        event.Veto();
}

void FortyFrame::NewGame(wxCommandEvent&) {
    m_canvas->NewGame();
}

void FortyFrame::Exit(wxCommandEvent&) {
    Close(true);
}

void FortyFrame::Help(wxCommandEvent& event) {
#if wxUSE_HTML
    if(wxFileExists(wxGetApp().GetHelpFile())) {
        FortyAboutDialog dialog(this, wxID_ANY, wxT("Forty Thieves Instructions"));
        if(dialog.ShowModal() == wxID_OK) {
        }
    }
#endif
    else {
        About(event);
    }
}

void FortyFrame::About(wxCommandEvent&) {
    wxMessageBox(
        wxT("Forty Thieves\n\n")
        wxT("A free card game written with the wxWidgets toolkit\n")
        wxT("Author: Chris Breeze (c) 1992-2004\n")
        wxT("email: chris@breezesys.com"),
        wxT("About Forty Thieves"),
        wxOK|wxICON_INFORMATION, this
        );
}


void FortyFrame::Undo(wxCommandEvent&) {
    m_canvas->Undo();
}

void FortyFrame::Redo(wxCommandEvent&) {
    m_canvas->Redo();
}

void FortyFrame::Scores(wxCommandEvent&) {
    m_canvas->UpdateScores();
    ScoreDialog scores(this, m_canvas->GetScoreFile());
    scores.Display();
}

void FortyFrame::ToggleRightButtonUndo(wxCommandEvent& event) {
    bool checked = m_menuBar->IsChecked(event.GetId());
    m_canvas->EnableRightButtonUndo(checked);
}

void FortyFrame::ToggleHelpingHand(wxCommandEvent& event) {
    bool checked = m_menuBar->IsChecked(event.GetId());
    m_canvas->EnableHelpingHand(checked);
}

void FortyFrame::ToggleCardSize(wxCommandEvent& event) {
    bool checked = m_menuBar->IsChecked(event.GetId());
    Card::SetScale(checked ? 1.3 : 1);
    m_canvas->LayoutGame();
    m_canvas->Refresh();
}

#include "wx/numdlg.h"

void FortyFrame::OnAskQuick(wxCommandEvent& WXUNUSED(event)) {

    long res = wxGetNumberFromUser(
        "설명이 들어갑니다.", // message
        "Enter a number", // prompt
        "제목이들어갑니다.", // title
        m_canvas->GetGame()->GetInput(), // value
        0, // min
        204, // max
        this
    );

    wxString msg;
    if(res == -1) {
        // Invalid number entered or dialog cancelled.
    } else {
        // wxPrintf("dialog.value = %lu", res);
        m_canvas->QuickN(res);
        m_canvas->GetGame()->SetInput(res);
    }
}

void FortyFrame::QuickStep(wxCommandEvent& event) {
    // wxPrintf("event.id = %d\n", event.GetId());
    m_canvas->QuickStep(event.GetId()-QUICK_1+1);
}

//----------------------------------------------------------------------------
// stAboutDialog
//----------------------------------------------------------------------------

FortyAboutDialog::FortyAboutDialog(wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style) :
    wxDialog(parent, id, title, position, size, style) {
    AddControls(this);

    Centre(wxBOTH);
}

bool FortyAboutDialog::AddControls(wxWindow* parent) {
#if wxUSE_HTML
    wxString htmlText;
    wxString htmlFile = wxGetApp().GetHelpFile();

    {
        wxTextFile file(htmlFile);
        if(file.Exists()) {
            file.Open();
            for(htmlText = file.GetFirstLine();
                  !file.Eof();
                  htmlText << file.GetNextLine() << wxT("\n")) ;
        }
    }

    if(htmlText.empty()) {
        htmlText.Printf(wxT("<html><head><title>Warning</title></head><body><P>Sorry, could not find resource for About dialog<P></body></html>"));
    }

    // Customize the HTML
    htmlText.Replace(wxT("$DATE$"), wxT(__DATE__));

    wxSize htmlSize(400, 290);

    // Note: in later versions of wxWin this will be fixed so wxRAISED_BORDER
    // does the right thing. Meanwhile, this is a workaround.
#ifdef __WXMSW__
    long borderStyle = wxDOUBLE_BORDER;
#else
    long borderStyle = wxRAISED_BORDER;
#endif

    wxHtmlWindow* html = new wxHtmlWindow(this, ID_ABOUT_HTML_WINDOW, wxDefaultPosition, htmlSize, borderStyle);
    html -> SetBorders(10);
    html -> SetPage(htmlText);

    //// Start of sizer-based control creation

    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxWindow *aboutBtn = parent->FindWindow(ID_ABOUT_HTML_WINDOW);
    wxASSERT(aboutBtn);
    sizer->Add(aboutBtn, 0, wxALIGN_CENTRE|wxALL, 5);

    wxButton *closeBtn = new wxButton(parent, wxID_CLOSE);
    closeBtn->SetDefault();
    closeBtn->SetFocus();
    SetAffirmativeId(wxID_CLOSE);

    sizer->Add(closeBtn, 0, wxALIGN_RIGHT|wxALL, 5);

    parent->SetSizer(sizer);
    parent->Layout();
    sizer->Fit(parent);
    sizer->SetSizeHints(parent);
#endif

    return true;
}
