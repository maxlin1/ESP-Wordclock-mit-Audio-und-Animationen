#pragma once

#include "DE10x11.hpp"

/*
 *           Layout Front
 *                COL
 *       X 9 8 7 6 5 4 3 2 1 0
 * ROW + - - - - - - - - - - -
 *  0  | E S K I S T L F Ü N F
 *  1  | Z E H N Z W A N Z I G
 *  2  | D R E I V I E R T E L
 *  3  | T G N A C H V O R J M
 *  4  | H A L B X Z W Ö L F P
 *  5  | Z W E I N S I E B E N
 *  6  | K D R E I R H F Ü N F
 *  7  | E L F N E U N V I E R
 *  8  | W A C H T Z E H N R S
 *  9  | B S E C H S F M U H R
 */

// Same words as the standard 10x11 layout, but UHR uses the last three
// letters of the bottom row (fronts like the one of Wortuhr_mp3).
class De10x11UhrRechts_t : public De10x11_t {
public:
    void show(FrontWord word) override {
        if (word == FrontWord::uhr) {
            setFrontMatrixWord(9, 0, 2);
            return;
        }
        De10x11_t::show(word);
    };
};

De10x11UhrRechts_t _de10x11UhrRechts;
