/////////////////////////////////////////////////////////////////////////////
// Name:        game.h
// Purpose:     Forty Thieves patience game
// Author:      Chris Breeze
// Modified by:
// Created:     21/07/97
// Copyright:   (c) 1993-1998 Chris Breeze
// Licence:     wxWindows licence
//---------------------------------------------------------------------------
// Last modified: 22nd July 1998 - ported to wxWidgets 2.0
/////////////////////////////////////////////////////////////////////////////
#ifndef _GAME_H_
#define _GAME_H_
#include "card.h"
#include "pile.h"

const int MaxMoves = 800;


//---------------------------------------//
// A class which holds the pack of cards //
//---------------------------------------//
class Pack : public Pile {
public:
    Pack(int x, int y);
    virtual ~Pack();
    void Redraw(wxDC& dc) wxOVERRIDE;
    void ResetPile() wxOVERRIDE { m_topCard = NumCards - 1; }
    void Shuffle();
    void AddCard(Card* card) wxOVERRIDE; // Add card
    void AddCard(wxDC& dc, Card* card) wxOVERRIDE { AddCard(card); Redraw(dc); }
    // std::string ToString() wxOVERRIDE { return "Pack[" + Pile::ToString() + "]"; }
    std::string ToString() wxOVERRIDE { return "P"; }
};


//----------------------------------------------------------//
// A class which holds a base i.e. the initial 10 x 4 cards //
//----------------------------------------------------------//
class Base : public Pile {
public:
    Base(int x, int y);
    virtual ~Base(){}
    bool AcceptCard(Card* card) wxOVERRIDE;
    // std::string ToString() wxOVERRIDE { return "Base[" + Pile::ToString() + "]"; }
    std::string ToString() wxOVERRIDE { return "B"; }
};


//----------------------------------------------------//
// A class which holds a foundation i.e. Ace, 2, 3... //
//----------------------------------------------------//
class Foundation : public Pile {
public:
    Foundation(int x, int y);
    virtual ~Foundation(){}
    bool AcceptCard(Card* card) wxOVERRIDE;
    // std::string ToString() wxOVERRIDE { return "Foundation[" + Pile::ToString() + "]"; }
    std::string ToString() wxOVERRIDE { return "F"; }
};


//--------------------------------------//
// A class which holds the discard pile //
//--------------------------------------//
class Discard : public Pile {
public:
    Discard(int x, int y);
    virtual ~Discard(){}
    void Redraw(wxDC& dc) wxOVERRIDE;
    void GetTopCardPos(int& x, int& y) wxOVERRIDE;
    Card* RemoveTopCard(wxDC& dc, int m_xOffset, int m_yOffset) wxOVERRIDE;
    // std::string ToString() wxOVERRIDE { return "Discard[" + Pile::ToString() + "]"; }
    std::string ToString() wxOVERRIDE { return "D"; }
};


class Game {
public:
    Game(int wins, int games, int score);
    virtual ~Game();

    void Layout();
    void NewPlayer(int wins, int games, int score);
    void Deal(bool resetPlay = true); // Shuffle and deal a new game
    bool CanYouGo(int x, int y); // can card under (x,y) go somewhere?
    bool HaveYouWon(); // have you won the game?
    void YouWon(wxDC& dc);

    void Undo(wxDC& dc); // Undo the last go
    void Redo(wxDC& dc); // Redo the last go
    void Auto(wxDC& dc); // Auto go
    void Mode1(wxDC& dc);
    void Mode2(wxDC& dc);
    void Mode3(wxDC& dc);
    void Mode4(wxDC& dc);
    void QuickStep(wxDC& dc, int step);
    void QuickN(wxDC& dc, int n);
    void Flip(wxDC& dc);

    void Redraw(wxDC& dc);
    void DisplayScore(wxDC& dc);
    bool LButtonDown(wxDC& dc, int mx, int my);
    void LButtonUp(wxDC& dc, int mx, int my);
    void LButtonDblClk(wxDC& dc, int mx, int my);
    void Choice(wxDC& dc, Pile* pile, Card* card, bool haveYouWon = true);
    void MouseMove(wxDC& dc, int mx, int my);

    int GetInput() const { return m_input; }
    void SetInput(int v) { m_input = v; }
    int GetMode() const { return m_mode; }
    void SetMode(int v) { m_mode = v; }
    int GetNumWins() const { return m_numWins; }
    int GetNumGames() const { return m_numGames; }
    int GetScore() const { return m_currentScore + m_totalScore; }
    bool InPlay() const { return m_inPlay; }

private:
    bool DropCard(int x, int y, Pile* pile, Card* card); // can the card at (x, y) be dropped on the pile?
    Pile* WhichPile(int x, int y); // which pile is (x, y) over?
    void DoMove(wxDC& dc, Pile* src, Pile* dest, bool haveYouWon = true);

    bool m_inPlay; // flag indicating that the game has started

    // undo buffer
    struct {
        Pile* src;
        Pile* dest;
    } m_moves[MaxMoves];
    int m_moveIndex; // current position in undo/redo buffer
    int m_redoIndex; // max move index available for redo
    int m_numMoves; // 

    // the various piles of cards
    Pack* m_pack;
    Discard* m_discard;
    Base* m_bases[10];
    Foundation* m_foundations[8];

    // variable of automation
    int m_mode = 2;
    int m_input = 0;

    // variables to do with dragging cards
    Pile* m_srcPile;
    Card* m_liftedCard;
    int m_xPos, m_yPos; // current coords of card being dragged
    int m_xOffset, m_yOffset; // card/mouse offset when dragging a card

    wxBitmap* m_bmap;
    wxBitmap* m_bmapCard;

    // variables to do with scoring
    int m_numGames;
    int m_numWins;
    int m_totalScore;
    int m_currentScore;
};

#endif // _GAME_H_
