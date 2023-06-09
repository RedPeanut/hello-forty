/////////////////////////////////////////////////////////////////////////////
// Name:        game.cpp
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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "forty.h"
#include "game.h"
#include "util.h"
#include <format>

Game::Game(int wins, int games, int score) :
    m_inPlay(false),
    m_moveIndex(0),
    m_redoIndex(0),
    m_numMoves(0),
    m_bmap(0),
    m_bmapCard(0) {
    int i;

    m_pack = new Pack(2, 2 + 4 * (CardHeight + 2));
    srand(time(0));

    for(i = 0; i < 5; i++) m_pack->Shuffle();

    m_discard = new Discard(2, 2 + 5 * (CardHeight + 2));

    for(i = 0; i < 8; i++) {
        m_foundations[i] = new Foundation(2 + (i / 4) * (CardWidth + 2),
                    2 + (i % 4) * (CardHeight + 2));
    }

    for(i = 0; i < 10; i++) {
        m_bases[i] = new Base(8 + (i + 2) * (CardWidth + 2), 2);
    }
    Deal();
    m_srcPile = 0;
    m_liftedCard = 0;

    // copy the input parameters for future reference
    m_numWins = wins;
    m_numGames = games;
    m_totalScore = score;
    m_currentScore = 0;
}


void Game::Layout() {
    int i;

    m_pack->SetPos(2, 2 + 4 * (CardHeight + 2));

    m_discard->SetPos(2, 2 + 5 * (CardHeight + 2));

    for(i = 0; i < 8; i++) {
        m_foundations[i]->SetPos(2 + (i / 4) * (CardWidth + 2),
                                2 + (i % 4) * (CardHeight + 2));
    }

    for(i = 0; i < 10; i++) {
        m_bases[i]->SetPos(8 + (i + 2) * (CardWidth + 2), 2);
    }
    delete m_bmap;
    delete m_bmapCard;
    m_bmap = 0;
    m_bmapCard = 0;
}

// Make sure we delete all objects created by the game object
Game::~Game() {
    int i;

    delete m_pack;
    delete m_discard;
    for(i = 0; i < 8; i++) {
        delete m_foundations[i];
    }
    for(i = 0; i < 10; i++) {
        delete m_bases[i];
    }
    delete m_bmap;
    delete m_bmapCard;
}

/*
Set the score for a new player.
NB: call Deal() first if the new player is to start
a new game
*/
void Game::NewPlayer(int wins, int games, int score) {
    m_numWins = wins;
    m_numGames = games;
    m_totalScore = score;
    m_currentScore = 0;
}

// Undo the last move
void Game::Undo(wxDC& dc) {
    if(m_moveIndex > 0) {
        m_moveIndex--;
        m_numMoves--;
        Card* card = m_moves[m_moveIndex].dest->RemoveTopCard(dc);
        m_moves[m_moveIndex].src->AddCard(dc, card);
        DisplayScore(dc);
    }
}

// Redo the last move
void Game::Redo(wxDC& dc) {
    if(m_moveIndex < m_redoIndex) {
        Card* card = m_moves[m_moveIndex].src->RemoveTopCard(dc);
        if(m_moves[m_moveIndex].src == m_pack) {
            m_pack->Redraw(dc);
            card->TurnCard(faceup);
        }
        m_moves[m_moveIndex].dest->AddCard(dc, card);
        DisplayScore(dc);
        m_moveIndex++;
        m_numMoves++;
    }
}

// Auto move
void Game::Auto(wxDC& dc) {
    if(m_mode == 1) Mode1(dc);
    else if(m_mode == 2) Mode2(dc);
    else if(m_mode == 3) Mode3(dc);
    else if(m_mode == 4) Mode4(dc);
}

void Game::Flip(wxDC& dc) {
    Card* card;
    card = m_pack->RemoveTopCard();
    m_pack->Redraw(dc);
    card->TurnCard(faceup);
    m_discard->AddCard(dc, card);
    DoMove(dc, m_pack, m_discard);
}

// To review my move, quickly rewind the step
void Game::QuickStep(wxDC& dc, int step) {
    Deal(false);
    Auto(dc);
    int numMoves = m_numMoves;
    for(int i = 0; i < numMoves; i++) Undo(dc);
    int unit = 20; //numMoves/10; // 20
    for(int i = 0; i < step*unit; i++) Redo(dc);
}

void Game::QuickN(wxDC& dc, int n) {
    Deal(false);
    Auto(dc);
    int numMoves = m_numMoves;
    for(int i = 0; i < numMoves; i++) Undo(dc);
    for(int i = 0; i < n; i++) Redo(dc);
}

#include "wx/file.h"
#include "wx/filename.h"

void Game::DoMove(wxDC& dc, Pile* src, Pile* dest, bool haveYouWon) {
    if(m_moveIndex < MaxMoves) {
        if(src == dest) {
            wxMessageBox(wxT("Game::DoMove() src == dest"), wxT("Debug message"),
                   wxOK | wxICON_EXCLAMATION);
        }
        m_moves[m_moveIndex].src = src;
        m_moves[m_moveIndex].dest = dest;
        m_moveIndex++;
        m_numMoves++;

        // when we do a move any moves in redo buffer are discarded
        m_redoIndex = m_moveIndex;
    } else {
        wxMessageBox(wxT("Game::DoMove() Undo buffer full"), wxT("Debug message"),
               wxOK | wxICON_EXCLAMATION);
    }

    if(!m_inPlay) {
        m_inPlay = true;
        m_numGames++;
    }
    DisplayScore(dc);

    if(haveYouWon && HaveYouWon())
        YouWon(dc);
}

void Game::YouWon(wxDC& dc) {
    // wxPrintf("m_moveIndex = %d\n", m_moveIndex);
    // wxPrintf("m_numMoves = %d\n", m_numMoves);

    wxString appName = wxTheApp->GetAppName();
    wxString homeDir = wxGetHomeDir();
    wxString filename = homeDir+wxT("/local/")+appName+".out";

    wxFile file;
    if(wxFile::Exists(filename))
        file.Open(filename, wxFile::write_append);
    else
        file.Create(filename, false, wxS_IRUSR | wxS_IWUSR);
    
    // Record m_moves rewind in here
    if(false && file.IsOpened()) {

        for(int i = m_moveIndex-1; i > -1; i--) {
            Card* card = m_moves[i].dest->RemoveTopCard(dc);
            m_moves[i].src->AddCard(dc, card);
        }

        std::string output;
        char buffer[32]; int n;
        for(int i = 0; i < m_moveIndex; i++) {
            Card* card = m_moves[i].src->RemoveTopCard(dc);
            m_moves[i].dest->AddCard(dc, card);
            // wxPrintf("[%s]%s>%s\n", card->ToString(), m_moves[i].src->ToString(), m_moves[i].dest->ToString());
            // output += " " + card->ToString() + m_moves[i].src->ToString() + m_moves[i].dest->ToString();
            n = snprintf(buffer, sizeof(buffer), "%s:%s>%s", card->ToString().c_str(), m_moves[i].src->ToString().c_str(), m_moves[i].dest->ToString().c_str());
            output += " " + std::string(buffer, n);
        }
        output = output.replace(output.find(" "), sizeof(" ")-1, "");

        file.Write(output+"\n");
    }
    file.Close();

    wxWindow *frame = wxTheApp->GetTopWindow();
    wxWindow *canvas = (wxWindow *) NULL;

    if(frame) {
        wxWindowList::compatibility_iterator node = frame->GetChildren().GetFirst();
        if(node) canvas = (wxWindow*)node->GetData();
    }

    // This game is over
    m_inPlay = false;

    // Redraw the score box to update games won
    DisplayScore(dc);

    if(wxMessageBox(wxT("Do you wish to play again?"),
        wxT("Well Done, You have won!"), wxYES_NO | wxICON_QUESTION) == wxYES) {
        Deal();
        canvas->Refresh();
    } else {
        // user cancelled the dialog - exit the app
        ((wxFrame*)canvas->GetParent())->Close(true);
    }
}

void Game::DisplayScore(wxDC& dc) {
    wxColour bgColour = FortyApp::BackgroundColour();
    wxPen* pen = wxThePenList->FindOrCreatePen(bgColour);
    dc.SetTextBackground(bgColour);
    dc.SetTextForeground(FortyApp::TextColour());
    dc.SetBrush(FortyApp::BackgroundBrush());
    dc.SetPen(* pen);

    // count the number of cards in foundations
    m_currentScore = 0;
    for(int i = 0; i < 8; i++) {
        m_currentScore += m_foundations[i]->GetNumCards();
    }

    int x, y;
    m_pack->GetTopCardPos(x, y);
    x += 12 * CardWidth - 105;

    wxCoord w, h;
    {
        wxCoord width, height;
        dc.GetTextExtent(wxT("Average score:m_x"), &width, &height);
        w = width;
        h = height;
    }
    dc.DrawRectangle(x + w, y, 20, 4 * h);

    wxString str;
    str.Printf(wxT("%d"), m_currentScore);
    dc.DrawText(wxT("Score:"), x, y);
    dc.DrawText(str, x + w, y);
    y += h;

    str.Printf(wxT("%d"), m_numGames);
    dc.DrawText(wxT("Games played:"), x, y);
    dc.DrawText(str, x + w, y);
    y += h;

    str.Printf(wxT("%d"), m_numWins);
    dc.DrawText(wxT("Games won:"), x, y);
    dc.DrawText(str, x + w, y);
    y += h;

    int average = 0;
    if(m_numGames > 0) {
        average = (2 * (m_currentScore + m_totalScore) + m_numGames ) / (2 * m_numGames);
    }
    str.Printf(wxT("%d"), average);
    dc.DrawText(wxT("Average score:"), x, y);
    dc.DrawText(str, x + w, y);
    y += h;

    str.Printf(wxT("%d"), m_numMoves);
    dc.DrawText(wxT("Moves:"), x, y);
    dc.DrawText(str, x + w, y);
}

// Shuffle the m_pack and deal the cards
void Game::Deal(bool resetPlay) {
    int i, j;
    Card* card;

    // Reset all the piles, the undo buffer and shuffle the m_pack
    m_moveIndex = 0;
    m_numMoves = 0;

    m_pack->ResetPile();
    for(i = 0; i < 5; i++) {
        m_pack->Shuffle();
    }
    m_discard->ResetPile();
    for(i = 0; i < 10; i++) {
        m_bases[i]->ResetPile();
    }
    for(i = 0; i < 8; i++) {
        m_foundations[i]->ResetPile();
    }

    // Deal the initial 40 cards onto the bases
    for(i = 0; i < 10; i++) {
        for(j = 1; j <= 4; j++) {
            card = m_pack->RemoveTopCard();
            card->TurnCard(faceup);
            m_bases[i]->AddCard(card);
        }
    }

    if(resetPlay) {
        if(m_inPlay) {
            // player has started the game and then redealt
            // and so we must add the score for this game to the total score
            m_totalScore += m_currentScore;
        }
        m_inPlay = false;
        m_input = 0;
    }
    m_currentScore = 0;
}


// Redraw the m_pack, discard pile, the bases and the foundations
void Game::Redraw(wxDC& dc) {
    int i;
    m_pack->Redraw(dc);
    m_discard->Redraw(dc);
    for(i = 0; i < 8; i++) {
        m_foundations[i]->Redraw(dc);
    }
    for(i = 0; i < 10; i++) {
        m_bases[i]->Redraw(dc);
    }
    DisplayScore(dc);

    if(m_bmap == 0) {
        m_bmap = new wxBitmap(CardWidth, CardHeight);
        m_bmapCard = new wxBitmap(CardWidth, CardHeight);

        // Initialise the card bitmap to the background colour
        wxMemoryDC memoryDC;
        memoryDC.SelectObject(*m_bmapCard);
        memoryDC.SetPen(*wxTRANSPARENT_PEN);
        memoryDC.SetBrush(FortyApp::BackgroundBrush());
        memoryDC.DrawRectangle(0, 0, CardWidth, CardHeight);
        memoryDC.SelectObject(*m_bmap);
        memoryDC.DrawRectangle(0, 0, CardWidth, CardHeight);
        memoryDC.SelectObject(wxNullBitmap);
    }
}


// Test to see if the point (x, y) is over the top card of one of the piles
// Returns pointer to the pile, or 0 if(x, y) is not over a pile
// or the pile is empty
Pile* Game::WhichPile(int x, int y) {
    if(m_pack->GetCard(x, y) &&
        m_pack->GetCard(x, y) == m_pack->GetTopCard()) {
        return m_pack;
    }

    if(m_discard->GetCard(x, y) &&
        m_discard->GetCard(x, y) == m_discard->GetTopCard()) {
        return m_discard;
    }

    int i;
    for(i = 0; i < 8; i++) {
        if(m_foundations[i]->GetCard(x, y) &&
            m_foundations[i]->GetCard(x, y) == m_foundations[i]->GetTopCard()) {
            return m_foundations[i];
        }
    }

    for(i = 0; i < 10; i++) {
        if(m_bases[i]->GetCard(x, y) &&
            m_bases[i]->GetCard(x, y) == m_bases[i]->GetTopCard()) {
            return m_bases[i];
        }
    }
    return 0;
}


// Left button is pressed - if cursor is over the m_pack then deal a card
// otherwise if it is over a card pick it up ready to be dragged - see MouseMove()
bool Game::LButtonDown(wxDC& dc, int x, int y) {
    m_srcPile = WhichPile(x, y);
    if(m_srcPile == m_pack) {
        Card* card = m_pack->RemoveTopCard();
        if(card) {
            m_pack->Redraw(dc);
            card->TurnCard(faceup);
            m_discard->AddCard(dc, card);
            DoMove(dc, m_pack, m_discard);
        }
        m_srcPile = 0;
    }
    else if(m_srcPile) {
        m_srcPile->GetTopCardPos(m_xPos, m_yPos);
        m_xOffset = m_xPos - x;
        m_yOffset = m_yPos - y;

        // Copy the area under the card
        // Initialise the card bitmap to the background colour
        {
            wxMemoryDC memoryDC;
            memoryDC.SelectObject(*m_bmap);
            m_liftedCard = m_srcPile->RemoveTopCard(memoryDC, m_xPos, m_yPos);
        }

        // Draw the card in card bitmap ready for blitting onto
        // the screen
        {
            wxMemoryDC memoryDC;
            memoryDC.SelectObject(*m_bmapCard);
            m_liftedCard->Draw(memoryDC, 0, 0);
        }
    }
    return m_srcPile != 0;
}

// Called when the left button is double clicked
// If a card is under the pointer and it can move elsewhere then move it.
void Game::LButtonDblClk(wxDC& dc, int x, int y) {
    Pile* pile = WhichPile(x, y);
    if(!pile) return;

    // Double click on m_pack is the same as left button down
    if(pile == m_pack) {
        LButtonDown(dc, x, y);
    } else {
        Card* card = pile->GetTopCard();
        if(card) {
            Choice(dc, pile, card);
        }
    }
}


// Move onto a foundation as first choice, a populated base as second and
// an empty base as third choice.
// NB Cards in the m_pack cannot be moved in this way - they aren't in play
// yet
void Game::Choice(wxDC& dc, Pile* pile, Card* card, bool haveYouWon) {

    int i;

    // if the card is an ace then try to place it next
    // to an ace of the same suit
    if(card->GetPipValue() == 1) {
        for(i = 0; i < 4; i++) {
            Card* m_topCard = m_foundations[i]->GetTopCard();
            if(m_topCard) {
                if(m_topCard->GetSuit() == card->GetSuit() &&
                    m_foundations[i + 4] != pile &&
                    m_foundations[i + 4]->GetTopCard() == 0) {
                    pile->RemoveTopCard(dc);
                    m_foundations[i + 4]->AddCard(dc, card);
                    DoMove(dc, pile, m_foundations[i + 4], haveYouWon);
                    return;
                }
            }
        }
    }

    // try to place the card on a foundation
    for(i = 0; i < 8; i++) {
        if(m_foundations[i]->AcceptCard(card) && m_foundations[i] != pile) {
            pile->RemoveTopCard(dc);
            m_foundations[i]->AddCard(dc, card);
            DoMove(dc, pile, m_foundations[i], haveYouWon);
            return;
        }
    }
    // try to place the card on a populated base
    for(i = 0; i < 10; i++) {
        if(m_bases[i]->AcceptCard(card) &&
            m_bases[i] != pile &&
            m_bases[i]->GetTopCard()) {
            pile->RemoveTopCard(dc);
            m_bases[i]->AddCard(dc, card);
            DoMove(dc, pile, m_bases[i], haveYouWon);
            return;
        }
    }
    // try to place the card on any base
    for(i = 0; i < 10; i++) {
        if(m_bases[i]->AcceptCard(card) && m_bases[i] != pile) {
            pile->RemoveTopCard(dc);
            m_bases[i]->AddCard(dc, card);
            DoMove(dc, pile, m_bases[i], haveYouWon);
            return;
        }
    }
}


// Test to see whether the game has been won:
// i.e. m_pack, discard and bases are empty
bool Game::HaveYouWon() {
    if(m_pack->GetTopCard()) return false;
    if(m_discard->GetTopCard()) return false;
    for(int i = 0; i < 10; i++) {
        if(m_bases[i]->GetTopCard()) return false;
    }
    m_numWins++;
    m_totalScore += m_currentScore;
    m_currentScore = 0;
    return true;
}


// See whether the card under the cursor can be moved somewhere else
// Returns 'true' if it can be moved, 'false' otherwise
bool Game::CanYouGo(int x, int y) {
    Pile* pile = WhichPile(x, y);
    if(pile && pile != m_pack) {
        Card* card = pile->GetTopCard();

        if(card) {
            int i;
            for(i = 0; i < 8; i++) {
                if(m_foundations[i]->AcceptCard(card) && m_foundations[i] != pile) {
                    return true;
                }
            }
            for(i = 0; i < 10; i++) {
                if(m_bases[i]->GetTopCard() &&
                    m_bases[i]->AcceptCard(card) &&
                    m_bases[i] != pile) {
                    return true;
                }
            }
        }
    }
    return false;
}


// Called when the left button is released after dragging a card
// Scan the piles to see if this card overlaps a pile and can be added
// to the pile. If the card overlaps more than one pile on which it can be placed
// then put it on the nearest pile.
void Game::LButtonUp(wxDC& dc, int x, int y) {
    if(m_srcPile) {
        // work out the position of the dragged card
        x += m_xOffset;
        y += m_yOffset;

        Pile* nearestPile = 0;
        int distance = (CardHeight + CardWidth) * (CardHeight + CardWidth);

        // find the nearest pile which will accept the card
        int i;
        for(i = 0; i < 8; i++) {
            if(DropCard(x, y, m_foundations[i], m_liftedCard)) {
                if(m_foundations[i]->CalcDistance(x, y) < distance) {
                    nearestPile = m_foundations[i];
                    distance = nearestPile->CalcDistance(x, y);
                }
            }
        }
        for(i = 0; i < 10; i++) {
            if(DropCard(x, y, m_bases[i], m_liftedCard)) {
                if(m_bases[i]->CalcDistance(x, y) < distance) {
                    nearestPile = m_bases[i];
                    distance = nearestPile->CalcDistance(x, y);
                }
            }
        }

        // Restore the area under the card
        wxMemoryDC memoryDC;
        memoryDC.SelectObject(*m_bmap);
        dc.Blit(m_xPos, m_yPos, CardWidth, CardHeight,
               &memoryDC, 0, 0, wxCOPY);

        // Draw the card in its new position
        if(nearestPile) {
            // Add to new pile
            nearestPile->AddCard(dc, m_liftedCard);
            if(nearestPile != m_srcPile) {
                DoMove(dc, m_srcPile, nearestPile);
            }
        } else {
            // Return card to src pile
            m_srcPile->AddCard(dc, m_liftedCard);
        }
        m_srcPile = 0;
        m_liftedCard = 0;
    }
}




bool Game::DropCard(int x, int y, Pile* pile, Card* card) {
    bool retval = false;
    if(pile->Overlap(x, y)) {
        if(pile->AcceptCard(card)) {
            retval = true;
        }
    }
    return retval;
}


void Game::MouseMove(wxDC& dc, int mx, int my) {
    if(m_liftedCard) {
        wxMemoryDC memoryDC;
        memoryDC.SelectObject(*m_bmap);

        int dx = mx + m_xOffset - m_xPos;
        int dy = my + m_yOffset - m_yPos;

        if(abs(dx) >= CardWidth || abs(dy) >= CardHeight) {
            // Restore the area under the card
            dc.Blit(m_xPos, m_yPos, CardWidth, CardHeight,
               &memoryDC, 0, 0, wxCOPY);

            // Copy the area under the card in the new position
            memoryDC.Blit(0, 0, CardWidth, CardHeight,
               &dc, m_xPos + dx, m_yPos + dy, wxCOPY);
        }
        else if(dx >= 0) {
            // dx >= 0
            dc.Blit(m_xPos, m_yPos, dx, CardHeight, &memoryDC, 0, 0, wxCOPY);
            if(dy >= 0) {
                // dy >= 0
                dc.Blit(m_xPos + dx, m_yPos, CardWidth - dx, dy, &memoryDC, dx, 0, wxCOPY);
                memoryDC.Blit(0, 0, CardWidth - dx, CardHeight - dy,
                       &memoryDC, dx, dy, wxCOPY);
                memoryDC.Blit(0, CardHeight - dy, CardWidth - dx, dy,
                       &dc, m_xPos + dx, m_yPos + CardHeight, wxCOPY);
            } else {
                // dy < 0
                dc.Blit(m_xPos + dx, m_yPos + dy + CardHeight, CardWidth - dx, -dy,
                       &memoryDC, dx, CardHeight + dy, wxCOPY);
                memoryDC.Blit(0, -dy, CardWidth - dx, CardHeight + dy,
                       &memoryDC, dx, 0, wxCOPY);
                memoryDC.Blit(0, 0, CardWidth - dx, -dy,
                       &dc, m_xPos + dx, m_yPos + dy, wxCOPY);
            }
            memoryDC.Blit(CardWidth - dx, 0, dx, CardHeight,
                   &dc, m_xPos + CardWidth, m_yPos + dy, wxCOPY);
        } else {
            // dx < 0
            dc.Blit(m_xPos + CardWidth + dx, m_yPos, -dx, CardHeight,
                   &memoryDC, CardWidth + dx, 0, wxCOPY);
            if(dy >= 0) {
                dc.Blit(m_xPos, m_yPos, CardWidth + dx, dy, &memoryDC, 0, 0, wxCOPY);
                memoryDC.Blit(-dx, 0, CardWidth + dx, CardHeight - dy,
                       &memoryDC, 0, dy, wxCOPY);
                memoryDC.Blit(-dx, CardHeight - dy, CardWidth + dx, dy,
                       &dc, m_xPos, m_yPos + CardHeight, wxCOPY);
            } else {
                // dy < 0
                dc.Blit(m_xPos, m_yPos + CardHeight + dy, CardWidth + dx, -dy,
                       &memoryDC, 0, CardHeight + dy, wxCOPY);
                memoryDC.Blit(-dx, -dy, CardWidth + dx, CardHeight + dy,
                       &memoryDC, 0, 0, wxCOPY);
                memoryDC.Blit(-dx, 0, CardWidth + dx, -dy,
                       &dc, m_xPos, m_yPos + dy, wxCOPY);
            }
            memoryDC.Blit(0, 0, -dx, CardHeight,
                   &dc, m_xPos + dx, m_yPos + dy, wxCOPY);
        }
        m_xPos += dx;
        m_yPos += dy;

        // draw the card in its new position
        memoryDC.SelectObject(*m_bmapCard);
        dc.Blit(m_xPos, m_yPos, CardWidth, CardHeight,
               &memoryDC, 0, 0, wxCOPY);
    }
}



//----------------------------------------------//
// The Pack class: holds the two decks of cards //
//----------------------------------------------//
Pack::Pack(int x, int y) : Pile(x, y, 0, 0) {

    // wxPrintf("Pack::ctor() is called...\n");

    const std::string foundation[] = {"10S","JH","AC","7H","5S","QD","JS","7D","6C","2H","AH","10H","QH","10C","8S","4D","7D","4C","9S","AS","4S","9D","6D","9H","KD","KC","9S","3C","2D","3H","3C","2D","QC","10D","QS","10S","6H","8C","9C","9D"};
    const std::string pack[] = {"2C","5D","QH","4H","7S","6S","2S","8H","AS","AC","6H","7C","8C","4D","JH","3D","6D","JS","JC","10C","10H","3H","AH","8D","AD","5H","9C","4H","KD","2C","KC","3S","4S","4C","3S","JD","KS","8D","3D","9H","QD","8S","6S","5H","KH","10D","QC","7H","5C","8H","7C","5S","2S","JC","2H","AD","JD","KH","6C","QS","KS","7S","5D","5C"};

    std::string value;
    std::string suit;
    int n = 0;
    for(int i = sizeof(pack)/sizeof(pack[0])-1; i >= 0; i--, n++) {
        if(pack[i].length() == 3) {
            value = pack[i].substr(0,2);
            suit = pack[i].substr(2,1);
        } else { //if(pack[i].length() == 2)
            value = pack[i].substr(0,1);
            suit = pack[i].substr(1,1);
        }
        m_cards[n] = new Card(Util::getValue(value), Util::getSuit(suit), facedown);
    }

    for(int i = sizeof(foundation)/sizeof(foundation[0])-1; i >= 0; i--, n++) {
        if(foundation[i].length() == 3) {
            value = foundation[i].substr(0,2);
            suit = foundation[i].substr(2,1);
        } else { //if(pack[i].length() == 2)
            value = foundation[i].substr(0,1);
            suit = foundation[i].substr(1,1);
        }
        m_cards[n] = new Card(Util::getValue(value), Util::getSuit(suit), facedown);
    }

    m_topCard = n - 1;
    // wxPrintf("m_topCard = %d\n", m_topCard);
}


void Pack::Shuffle() {

    for(int i = 0; i <= m_topCard; i++) {
        m_cards[i]->TurnCard(facedown);
    }

    return;

    Card* temp[NumCards];
    int i;

    // Don't try to shuffle an empty m_pack!
    if(m_topCard < 0) return;

    // Copy the cards into a temporary array. Start by clearing
    // the array and then copy the card into a random position.
    // If the position is occupied then find the next lower position.
    for(i = 0; i <= m_topCard; i++) {
        temp[i] = 0;
    }
    for(i = 0; i <= m_topCard; i++) {
        int pos = rand() % (m_topCard + 1);
        while (temp[pos]) {
            pos--;
            if(pos < 0) pos = m_topCard;
        }
        m_cards[i]->TurnCard(facedown);
        temp[pos] = m_cards[i];
        m_cards[i] = 0;
    }

    // Copy each card back into the m_pack in a random
    // position. If position is occupied then find nearest
    // unoccupied position after the random position.
    for(i = 0; i <= m_topCard; i++) {
        int pos = rand() % (m_topCard + 1);
        while (m_cards[pos]) {
            pos++;
            if(pos > m_topCard) pos = 0;
        }
        m_cards[pos] = temp[i];
    }
}

void Pack::Redraw(wxDC& dc) {
    Pile::Redraw(dc);

    wxString str;
    str.Printf(wxT("%d  "), m_topCard + 1);

    dc.SetBackgroundMode(wxBRUSHSTYLE_SOLID);
    dc.SetTextBackground(FortyApp::BackgroundColour());
    dc.SetTextForeground(FortyApp::TextColour());
    dc.DrawText(str, m_x + CardWidth + 5, m_y + CardHeight / 2);

}

void Pack::AddCard(Card* card) {
    if(card == m_cards[m_topCard + 1]) {
        m_topCard++;
    } else {
        wxMessageBox(wxT("Pack::AddCard() Undo error"), wxT("Forty Thieves: Warning"),
           wxOK | wxICON_EXCLAMATION);
    }
    card->TurnCard(facedown);
}


Pack::~Pack() {
    for(m_topCard = 0; m_topCard < NumCards; m_topCard++) {
        delete m_cards[m_topCard];
    }
}


//------------------------------------------------------//
// The Base class: holds the initial pile of four cards //
//------------------------------------------------------//
Base::Base(int x, int y) : Pile(x, y, 0, 12) {
    m_topCard = -1;
}


bool Base::AcceptCard(Card* card) {
    bool retval = false;

    if(m_topCard >= 0) {
        if(m_cards[m_topCard]->GetSuit() == card->GetSuit() &&
            m_cards[m_topCard]->GetPipValue() - 1 == card->GetPipValue()) {
            retval = true;
        }
    } else {
        // pile is empty - ACCEPT
        retval = true;
    }
    return retval;
}


//----------------------------------------------------------------//
// The Foundation class: holds the cards built up from the ace... //
//----------------------------------------------------------------//
Foundation::Foundation(int x, int y) : Pile(x, y, 0, 0) {
    m_topCard = -1;
}

bool Foundation::AcceptCard(Card* card) {
    bool retval = false;

    if(m_topCard >= 0) {
        if(m_cards[m_topCard]->GetSuit() == card->GetSuit() &&
            m_cards[m_topCard]->GetPipValue() + 1 == card->GetPipValue()) {
            retval = true;
        }
    }
    else if(card->GetPipValue() == 1) {
        // It's an ace and the pile is empty - ACCEPT
        retval = true;
    }
    return retval;
}


//----------------------------------------------------//
// The Discard class: holds cards dealt from the m_pack //
//----------------------------------------------------//
Discard::Discard(int x, int y) : Pile(x, y, 19, 0) {
    m_topCard = -1;
}

void Discard::Redraw(wxDC& dc) {
    if(m_topCard >= 0) {
        if(m_dx == 0 && m_dy == 0) {
            m_cards[m_topCard]->Draw(dc, m_x, m_y);
        } else {
            int x = m_x;
            int y = m_y;
            for(int i = 0; i <= m_topCard; i++) {
                m_cards[i]->Draw(dc, x, y);
                x += m_dx;
                y += m_dy;
                if(i == 31) {
                    x = m_x;
                    y = m_y + CardHeight / 3;
                }
            }
        }
    } else {
        Card::DrawNullCard(dc, m_x, m_y);
    }
}


void Discard::GetTopCardPos(int& x, int& y) {
    if(m_topCard < 0) {
        x = m_x;
        y = m_y;
    }
    else if(m_topCard > 31) {
        x = m_x + m_dx * (m_topCard - 32);
        y = m_y + CardHeight / 3;
    } else {
        x = m_x + m_dx * m_topCard;
        y = m_y;
    }
}


Card* Discard::RemoveTopCard(wxDC& dc, int m_xOffset, int m_yOffset) {
    Card* card;

    if(m_topCard <= 31) {
        card = Pile::RemoveTopCard(dc, m_xOffset, m_yOffset);
    } else {
        int topX, topY, x, y;
        GetTopCardPos(topX, topY);
        card = Pile::RemoveTopCard();
        card->Erase(dc, topX - m_xOffset, topY - m_yOffset);
        GetTopCardPos(x, y);
        dc.SetClippingRegion(topX - m_xOffset, topY - m_yOffset,
                     CardWidth, CardHeight);

        for(int i = m_topCard - 31; i <= m_topCard - 31 + CardWidth / m_dx; i++) {
            m_cards[i]->Draw(dc, m_x - m_xOffset + i * m_dx, m_y - m_yOffset);
        }
        if(m_topCard > 31) {
            m_cards[m_topCard]->Draw(dc, topX - m_xOffset - m_dx, topY - m_yOffset);
        }
        dc.DestroyClippingRegion();
    }

    return card;
}

void Game::Mode1(wxDC& dc) {
    Choice(dc, m_bases[4], m_bases[4]->GetTopCard());		// AS:B>F // 1
    Flip(dc);												// 2C:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 2C:D>B // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 9H:B>B // 
    Flip(dc);												// 5D:P>D // 5
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5D:D>B // 
    Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 4D:B>B // 
    Flip(dc);												// QH:P>D // 
    Flip(dc);												// 4H:P>D // 
    Flip(dc);												// 7S:P>D // 10
    Choice(dc, m_discard, m_discard->GetTopCard());			// 7S:D>B // 
    Flip(dc);												// 6S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 6S:D>B // 
    Flip(dc);												// 2S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 2S:D>F // 15
    Flip(dc);												// 8H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 8H:D>B // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 7H:B>B // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// AC:B>F // 
    Flip(dc);												// AS:P>D // 20
    Choice(dc, m_discard, m_discard->GetTopCard());			// AS:D>F // 
    Flip(dc);												// AC:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// AC:D>F // 
    Choice(dc, m_bases[6], m_bases[6]->GetTopCard());		// 2C:B>F // 
    Choice(dc, m_bases[6], m_bases[6]->GetTopCard());		// 3C:B>F // 25
    Flip(dc);												// 6H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// x 6H:D>B -> 55/5H:D>B -> 58/4H:D>B // 뒷단 카드들 때문에 줄일수 없음
    Flip(dc);												// 7C:P>D // 
    Flip(dc);												// 8C:P>D // 
    Flip(dc);												// 4D:P>D // 30
    Flip(dc);												// JH:P>D // 
    Flip(dc);												// 3D:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 3D:D>B // 
    Flip(dc);												// 6D:P>D // 
    Flip(dc);												// JS:P>D // 35
    Flip(dc);												// JC:P>D // 
    Flip(dc);												// 10C:P>D // 
    Flip(dc);												// 10H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 10H:D>B // 
    Flip(dc);												// 3H:P>D // 40
    Flip(dc);												// AH:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// AH:D>F // 
    Flip(dc);												// 8D:P>D // 
    Flip(dc);												// AD:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// AD:D>F // 45
    Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 2D:B>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 3D:B>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 4D:B>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 5D:B>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 6D:B>F // 50
    Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// 7D:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 8D:D>F // 
    Choice(dc, m_bases[9], m_bases[9]->GetTopCard());		// ?? 9D:B>F -> // 
    Flip(dc);												// 5H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5H:D>B // 55
    Flip(dc);												// 9C:P>D // 
    Flip(dc);												// 4H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 4H:D>B // 
    Flip(dc);												// KD:P>D // 
    Flip(dc);												// 2C:P>D // 60
    Choice(dc, m_discard, m_discard->GetTopCard());			// 2C:D>F // 
    Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 3C:B>F // 
    Flip(dc);												// KC:P>D // 
    Flip(dc);												// 3S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 3S:D>F // 65
    Flip(dc);												// 4S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 53/9D:B>F -> ? 4S:D>F // 
    Flip(dc);												// 4C:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 4C:D>F // 
    Flip(dc);												// 3S:P>D // 70
    Flip(dc);												// JD:P>D // 
    Flip(dc);												// KS:P>D // 
    Flip(dc);												// 8D:P>D // 
    Flip(dc);												// 3D:P>D // 
    Flip(dc);												// 9H:P>D // 75
    Choice(dc, m_discard, m_discard->GetTopCard());			// 9H:D>B // 
    Flip(dc);												// QD:P>D // 
    Flip(dc);												// 8S:P>D // 
    Flip(dc);												// 6S:P>D // 
    Flip(dc);												// 5H:P>D // 80
    Flip(dc);												// KH:P>D // 
    Flip(dc);												// 10D:P>D // 
    Flip(dc);												// QC:P>D // 
    Flip(dc);												// 7H:P>D // 
    Flip(dc);												// 5C:P>D // 85
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5C:D>F // 
    Flip(dc);												// 8H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 8H:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 7H:D>B // 
    Flip(dc);												// 7C:P>D // 90
    Flip(dc);												// 5S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5S:D>F // 
    Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 6S:B>F // 
    Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 7S:B>F // 
    Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 8S:B>F // 95
    Choice(dc, m_bases[4], m_bases[4]->GetTopCard());		// x 9S:B>F // 아닌듯 // 
    Choice(dc, m_bases[8], m_bases[8]->GetTopCard());		// 10S:B>F // 
    Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// JS:B>F // 
    Choice(dc, m_bases[8], m_bases[8]->GetTopCard());		// QS:B>F // 
    Choice(dc, m_bases[9], m_bases[9]->GetTopCard());		// 9C:B>B // 100
    Flip(dc);												// 2S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 2S:D>F // 
    Flip(dc);												// JC:P>D // 
    Flip(dc);												// 2H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 2H:D>F // 105
    Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 3H:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 4H:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 5H:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 6H:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 7H:B>F // 110
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 8H:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 9H:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 10H:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// AH:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 2H:B>F // 115
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 6C:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// JC:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 7C:D>F // 
    Choice(dc, m_bases[9], m_bases[9]->GetTopCard());		// 8C:B>F // 
    Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 9C:B>F // 120
    Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 10C:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// JC:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// QC:D>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 10D:D>F // 
    Flip(dc);												// AD:P>D // 125
    Choice(dc, m_discard, m_discard->GetTopCard());			// AD:D>F // 
    Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 2D:B>F // 
    Flip(dc);												// JD:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// JD:D>F // 
    Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// QD:B>F // 130
    Choice(dc, m_bases[4], m_bases[4]->GetTopCard());		// 4C:B>F // 
    Choice(dc, m_bases[9], m_bases[9]->GetTopCard());		// 6H:B>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// KH:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5H:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 6S:D>B // 135
    Choice(dc, m_discard, m_discard->GetTopCard());			// 8S:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// QD:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 3D:D>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 9D:B>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 8D:D>B // 140
    Choice(dc, m_discard, m_discard->GetTopCard());			// KS:D>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// JD:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 3S:D>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 4S:B>F // 
    Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// 5S:B>F // 145
    Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 6S:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// KC:D>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// KD:D>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 9C:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 3H:D>F // 150
    Choice(dc, m_discard, m_discard->GetTopCard());			// 10C:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// JC:D>B // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 10C:B>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// JS:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 6D:D>B // 155
    Choice(dc, m_discard, m_discard->GetTopCard());			// JH:D>F // 
    Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// QH:B>F // 
    Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// KH:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 4D:D>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 8C:D>B // 160
    Choice(dc, m_discard, m_discard->GetTopCard());			// 7C:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 4H:D>F // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 5H:B>F // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 6H:B>F // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 7H:B>F // 165
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 8H:B>F // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 9H:B>F // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 10H:B>F // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// JH:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// QH:D>F // 170
    Flip(dc);												// KH:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// KH:D>F // 
    Flip(dc);												// 6C:P>D // 
    Flip(dc);												// QS:P>D // 
    Flip(dc);												// KS:P>D // 175
    Flip(dc);												// 7S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 7S:D>F // 
    Choice(dc, m_bases[6], m_bases[6]->GetTopCard());		// 8S:B>F // 
    Choice(dc, m_bases[6], m_bases[6]->GetTopCard());		// 9S:B>F // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 10S:B>F // 180
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// JS:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// KS:D>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// QS:D>F // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// KS:B>F // 
    Flip(dc);												// 5D:P>D // 185
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5D:D>F // 
    Choice(dc, m_bases[4], m_bases[4]->GetTopCard());		// 6D:B>F // 
    Choice(dc, m_bases[4], m_bases[4]->GetTopCard());		// 7D:B>F // 
    Choice(dc, m_bases[8], m_bases[8]->GetTopCard());		// 8D:B>F // 
    Choice(dc, m_bases[8], m_bases[8]->GetTopCard());		// 9D:B>F // 190
    Choice(dc, m_bases[8], m_bases[8]->GetTopCard());		// 10D:B>F // 
    Choice(dc, m_bases[9], m_bases[9]->GetTopCard());		// JD:B>F // 
    Choice(dc, m_bases[9], m_bases[9]->GetTopCard());		// QD:B>F // 
    Flip(dc);												// 5C:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5C:D>F // 195
    Choice(dc, m_discard, m_discard->GetTopCard());			// 6C:D>F // 
    Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// 7C:B>F // 
    Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// 8C:B>F // 
    Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// 9C:B>F // 
    Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 10C:B>F // 200
    Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// JC:B>F // 
    Choice(dc, m_bases[8], m_bases[8]->GetTopCard());		// QC:B>F // 
    Choice(dc, m_bases[6], m_bases[6]->GetTopCard());		// KC:B>F // 
    Choice(dc, m_bases[6], m_bases[6]->GetTopCard(), false);	// KD:B>F // 204
}

void Game::Mode2(wxDC& dc) {
    Choice(dc, m_bases[4], m_bases[4]->GetTopCard());		// AS:B>F // 1
    Flip(dc);												// 2C:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 2C:D>B // 
    Flip(dc);												// 5D:P>D // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 9H:B>B // 5
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5D:D>B // 
    Flip(dc);												// QH:P>D // 
    Flip(dc);												// 4H:P>D // 
    Flip(dc);												// 7S:P>D // 
    Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 4D:B>B // 10
    Choice(dc, m_discard, m_discard->GetTopCard());			// 7S:D>B // 
    Flip(dc);												// 6S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 6S:D>B // 
    Flip(dc);												// 2S:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 2S:D>F // 15
    Flip(dc);												// 8H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 8H:D>B // 
    Flip(dc);												// AS:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// AS:D>F // 
    Flip(dc);												// AC:P>D // 20
    Choice(dc, m_discard, m_discard->GetTopCard());			// AC:D>F // 
    Flip(dc);												// 6H:P>D // 
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// 7H:B>B // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 6H:D>B // 
    Flip(dc);												// 7C:P>D // 25
    Flip(dc);												// 8C:P>D // 
    Flip(dc);												// 4D:P>D // 
    Flip(dc);												// JH:P>D // 
    Flip(dc);												// 3D:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 3D:D>B // 30
    Flip(dc);												// 6D:P>D // 
    Flip(dc);												// JS:P>D // 
    Flip(dc);												// JC:P>D // 
    Flip(dc);												// 10C:P>D // 
    Flip(dc);												// 10H:P>D // 35
    Choice(dc, m_bases[0], m_bases[0]->GetTopCard());		// AC:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 10H:D>B // 
    Flip(dc);												// 3H:P>D // 
    Flip(dc);												// AH:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// AH:D>F // 40
    Flip(dc);												// 8D:P>D // 
    Flip(dc);												// AD:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// AD:D>F // 
    Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 2D:B>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 3D:B>F // 45
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 4D:B>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 5D:B>F // 
    Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 6D:B>F // 
    Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// 7D:B>F // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 8D:D>F # // 50
    Flip(dc);												// 5H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 5H:D>B // 
    Flip(dc);												// 9C:P>D // 
    Flip(dc);												// 4H:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 4H:D>B // 55
    Flip(dc);												// KD:P>D // 
    Flip(dc);												// 2C:P>D // 
    Choice(dc, m_discard, m_discard->GetTopCard());			// 2C:D>F // 
    Flip(dc);												// KC:P>D // 
    Flip(dc);												// 3S:P>D // 60
    Choice(dc, m_discard, m_discard->GetTopCard());			// 3S:D>F # // 

    

    // Flip(dc);												// 4S:P>D // 
    // Flip(dc);												// 4C:P>D // 
    // Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 3C:B>F // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 4C:D>F // 65
    // Flip(dc);												// 3S:P>D // 
    // Flip(dc);												// JD:P>D // 
    // Flip(dc);												// KS:P>D // 
    // Flip(dc);												// 8D:P>D // 
    // Flip(dc);												// 3D:P>D // 70
    // Flip(dc);												// 9H:P>D // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 9H:D>B // 
    // Flip(dc);												// QD:P>D // 
    // Flip(dc);												// 8S:P>D // 
    // Flip(dc);												// 6S:P>D // 75
    // Flip(dc);												// 5H:P>D // 
    // Flip(dc);												// KH:P>D // 
    // Flip(dc);												// 10D:P>D // 
    // Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 9D:B>F // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 10D:D>F // 80
    // Choice(dc, m_bases[5], m_bases[5]->GetTopCard());		// 4S:B>F // ?

    // Flip(dc);												// QC:P>D // 
    // Flip(dc);												// 7H:P>D // 
    // Flip(dc);												// 5C:P>D // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 5C:D>F // 
    // Flip(dc);												// 8H:P>D // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 8H:D>B // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 7H:D>B // 
    // Flip(dc);												// 7C:P>D // 
    // Flip(dc);												// 5S:P>D // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 5S:D>F // 
    // Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 6S:B>F // 
    // Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 7S:B>F // 
    // Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 8S:B>F // 
    // Choice(dc, m_bases[4], m_bases[4]->GetTopCard());		// 9S:B>F //
    // Choice(dc, m_bases[8], m_bases[8]->GetTopCard());		// 10S:B>F // 
    // Choice(dc, m_bases[1], m_bases[1]->GetTopCard());		// JS:B>F // 
    // Choice(dc, m_bases[8], m_bases[8]->GetTopCard());		// QS:B>F // 
    // Choice(dc, m_bases[9], m_bases[9]->GetTopCard());		// 9C:B>B // 
    // Flip(dc);												// 2S:P>D // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 2S:D>F // 
    // Flip(dc);												// JC:P>D // 
    // Flip(dc);												// 2H:P>D // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 2H:D>F // 
    // Choice(dc, m_bases[7], m_bases[7]->GetTopCard());		// 3H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 4H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 5H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 6H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 7H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 8H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 9H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 10H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// AH:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 2H:B>F // 
    // Choice(dc, m_bases[2], m_bases[2]->GetTopCard());		// 6C:B>F // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// JC:D>B // 
    // Choice(dc, m_discard, m_discard->GetTopCard());			// 7C:D>F // 
    // Choice(dc, m_bases[9], m_bases[9]->GetTopCard());		// 8C:B>F // 
    // Choice(dc, m_bases[3], m_bases[3]->GetTopCard());		// 9C:B>F // 
}

void Game::Mode3(wxDC& dc) {}

void Game::Mode4(wxDC& dc) {}