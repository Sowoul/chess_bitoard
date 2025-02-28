#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

uint64_t totalCount = 0;
uint64_t saved = 0;
uint64_t collisions = 0;

int MOVES = 1;


#define TABLE_SIZE (1 << 27)
#define DEPTH 5
#define TOTAL_MOVES 20

#define PAWN_VALUE 110
#define KNIGHT_VALUE 330
#define BISHOP_VALUE 360
#define ROOK_VALUE 500
#define QUEEN_VALUE 900
#define KING_VALUE 20000


const int pawn_table[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 10, 10, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5,  5, 10, 35, 35, 10,  5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5, -5,-10,  0,  0,-10, -5,  5,
    5, 10, 10,-20,-20, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0
};

const int knight_table[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 20, 15, 15, 20,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int bishop_table[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};


typedef struct Board {
    uint64_t BLACK_ROOKS, BLACK_KNIGHTS, BLACK_BISHOPS, BLACK_QUEEN, BLACK_PAWNS, BLACK_KING;
    uint64_t WHITE_ROOKS, WHITE_KNIGHTS, WHITE_BISHOPS, WHITE_QUEEN, WHITE_PAWNS, WHITE_KING;
    uint64_t WHITE_PIECES, BLACK_PIECES;
    bool turn;
} board;

typedef struct MovePair{
    char *from, *to;
    _Float16 eval;
} movepair;

typedef struct transposition {
    board BOARD;
    int depth;
    movepair *best_move;
    struct transposition *next;
} Transposition;

Transposition *TRANSPOSITIONS[TABLE_SIZE] = {NULL};

void init_table() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        TRANSPOSITIONS[i] = NULL;
    }
}

void free_table(){
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (TRANSPOSITIONS[i]!=NULL){
            free(TRANSPOSITIONS[i]);
            TRANSPOSITIONS[i] = NULL;
        }
    }
}

int random_table[12][64];

void setupRandomTable(){
    for (int piece = 0; piece<12; piece++){
        for (int i = 0; i < 64; i++){
            random_table[piece][i] = rand();
        }
    }
}


uint64_t hash(board BOARD) {
    uint64_t h = BOARD.turn;
    board temp = BOARD;
    h *= MOVES;
    while (temp.BLACK_ROOKS) {
        int pos = __builtin_ctzll(temp.BLACK_ROOKS);
        h ^= random_table[0][pos];
        temp.BLACK_ROOKS &= temp.BLACK_ROOKS - 1;
    }

    while (temp.WHITE_ROOKS) {
        int pos = __builtin_ctzll(temp.WHITE_ROOKS);
        h ^= random_table[1][pos];
        temp.WHITE_ROOKS &= temp.WHITE_ROOKS - 1;
    }

    while (temp.BLACK_KNIGHTS) {
        int pos = __builtin_ctzll(temp.BLACK_KNIGHTS);
        h ^= random_table[2][pos];
        temp.BLACK_KNIGHTS &= temp.BLACK_KNIGHTS - 1;
    }

    while (temp.WHITE_KNIGHTS) {
        int pos = __builtin_ctzll(temp.WHITE_KNIGHTS);
        h ^= random_table[3][pos];
        temp.WHITE_KNIGHTS &= temp.WHITE_KNIGHTS - 1;
    }

    while (temp.BLACK_BISHOPS) {
        int pos = __builtin_ctzll(temp.BLACK_BISHOPS);
        h ^= random_table[4][pos];
        temp.BLACK_BISHOPS &= temp.BLACK_BISHOPS - 1;
    }

    while (temp.WHITE_BISHOPS) {
        int pos = __builtin_ctzll(temp.WHITE_BISHOPS);
        h ^= random_table[5][pos];
        temp.WHITE_BISHOPS &= temp.WHITE_BISHOPS - 1;
    }

    while (temp.BLACK_QUEEN) {
        int pos = __builtin_ctzll(temp.BLACK_QUEEN);
        h ^= random_table[6][pos];
        temp.BLACK_QUEEN &= temp.BLACK_QUEEN - 1;
    }

    while (temp.WHITE_QUEEN) {
        int pos = __builtin_ctzll(temp.WHITE_QUEEN);
        h ^= random_table[7][pos];
        temp.WHITE_QUEEN &= temp.WHITE_QUEEN- 1;
    }

    while (temp.BLACK_KING) {
        int pos = __builtin_ctzll(temp.BLACK_KING);
        h ^= random_table[8][pos];
        temp.BLACK_KING = 0;
    }

    while (temp.WHITE_KING) {
        int pos = __builtin_ctzll(temp.WHITE_KING);
        h ^= random_table[9][pos];
        temp.WHITE_KING = 0;
    }

    while (temp.BLACK_PAWNS) {
        int pos = __builtin_ctzll(temp.BLACK_PAWNS);
        h ^= random_table[10][pos];
        temp.BLACK_PAWNS &= temp.BLACK_PAWNS - 1;
    }

    while (temp.WHITE_PAWNS) {
        int pos = __builtin_ctzll(temp.WHITE_PAWNS);
        h ^= random_table[11][pos];
        temp.WHITE_PAWNS &= temp.WHITE_PAWNS - 1;
    }

    return h % TABLE_SIZE;
}


bool same(const board *BOARD1, const board *BOARD2) {
    return memcmp(BOARD1, BOARD2, sizeof(board)) == 0;
}

bool addTransposition(Transposition *position) {
    if (position == NULL) return false;

    uint64_t idx = hash(position->BOARD);
    position->next = NULL;


    Transposition *current = TRANSPOSITIONS[idx];
    Transposition *prev = NULL;
    if (current != NULL) collisions++;
    while (current != NULL) {
        if (same(&current->BOARD, &position->BOARD) && current->depth >= position->depth) {
            current->best_move = position->best_move;
            collisions--;
            return true;
        }
        prev = current;
        current = current->next;
    }


    if (prev == NULL) {
        TRANSPOSITIONS[idx] = position;
    } else {
        prev->next = position;
    }

    return true;
}

Transposition* getOrDefault(const board *BOARD, int depth) {
    uint64_t idx = hash(*BOARD);
    Transposition *current = TRANSPOSITIONS[idx];


    const int MAX_PROBES = 10;
    int probes = 0;

    while (current != NULL && probes < MAX_PROBES) {
        if (same(BOARD, &current->BOARD) && current->depth >= depth) {
            return current;
        }
        current = current->next;
        probes++;
    }

    return NULL;
}


_Float16 evaluateBoard(board *BOARD);

void setupBoard(board *BOARD) {
    BOARD->BLACK_ROOKS   = 0x8100000000000000ULL;
    BOARD->BLACK_KNIGHTS = 0x4200000000000000ULL;
    BOARD->BLACK_BISHOPS = 0x2400000000000000ULL;
    BOARD->BLACK_PAWNS   = 0x00ff000000000000ULL;
    BOARD->BLACK_QUEEN   = 0x1000000000000000ULL;
    BOARD->BLACK_KING    = 0x0800000000000000ULL;

    BOARD->WHITE_ROOKS   = 0x81ULL;
    BOARD->WHITE_KNIGHTS = 0x42ULL;
    BOARD->WHITE_BISHOPS = 0x24ULL;
    BOARD->WHITE_PAWNS   = 0xff00ULL;
    BOARD->WHITE_QUEEN   = 0x10ULL;
    BOARD->WHITE_KING    = 0x08ULL;

    BOARD->turn = true;

    BOARD->WHITE_PIECES = BOARD->WHITE_ROOKS | BOARD->WHITE_KNIGHTS | BOARD->WHITE_BISHOPS |
                           BOARD->WHITE_QUEEN | BOARD->WHITE_KING | BOARD->WHITE_PAWNS;
    BOARD->BLACK_PIECES = BOARD->BLACK_ROOKS | BOARD->BLACK_KNIGHTS | BOARD->BLACK_BISHOPS |
                           BOARD->BLACK_QUEEN | BOARD->BLACK_KING | BOARD->BLACK_PAWNS;
}

void updateMasks(board *BOARD) {
    BOARD->WHITE_PIECES = BOARD->WHITE_ROOKS | BOARD->WHITE_KNIGHTS | BOARD->WHITE_BISHOPS |
                           BOARD->WHITE_QUEEN | BOARD->WHITE_KING | BOARD->WHITE_PAWNS;
    BOARD->BLACK_PIECES = BOARD->BLACK_ROOKS | BOARD->BLACK_KNIGHTS | BOARD->BLACK_BISHOPS |
                           BOARD->BLACK_QUEEN | BOARD->BLACK_KING | BOARD->BLACK_PAWNS;
}

void toggleMove(board *BOARD) {
    BOARD->turn = !BOARD->turn;
}

bool getTurn(board *BOARD) {
    return BOARD->turn;
}

int square_index(const char *coords) {
    int file = 'h' - coords[0];
    int rank = coords[1] - '1';
    return rank * 8 + file;
}

bool is_clear_path(uint64_t occupancy, int from, int to) {
    int from_row = from / 8, from_col = from % 8;
    int to_row   = to / 8,   to_col   = to % 8;
    int d_row = to_row - from_row;
    int d_col = to_col - from_col;

    if (!(from_row == to_row || from_col == to_col || abs(d_row) == abs(d_col)))
        return false;

    d_row = (d_row > 0) ? 1 : (d_row < 0 ? -1 : 0);
    d_col = (d_col > 0) ? 1 : (d_col < 0 ? -1 : 0);

    int current_row = from_row + d_row;
    int current_col = from_col + d_col;
    while (current_row != to_row || current_col != to_col) {
        int idx = current_row * 8 + current_col;
        if (occupancy & (1ULL << idx))
            return false;
        current_row += d_row;
        current_col += d_col;
    }
    return true;
}

bool is_valid_knight_move(int from, int to) {
    int from_row = from / 8, from_col = from % 8;
    int to_row = to / 8, to_col = to % 8;
    int d_row = abs(to_row - from_row), d_col = abs(to_col - from_col);
    return ((d_row == 2 && d_col == 1) || (d_row == 1 && d_col == 2));
}

bool is_valid_king_move(int from, int to) {
    int from_row = from / 8, from_col = from % 8;
    int to_row = to / 8, to_col = to % 8;
    return (abs(to_row - from_row) <= 1 && abs(to_col - from_col) <= 1);
}

bool is_valid_rook_move(int from, int to, uint64_t occupancy) {
    int from_row = from / 8, from_col = from % 8;
    int to_row = to / 8, to_col = to % 8;
    if (from_row == to_row || from_col == to_col)
        return is_clear_path(occupancy, from, to);
    return false;
}

bool is_valid_bishop_move(int from, int to, uint64_t occupancy) {
    int from_row = from / 8, from_col = from % 8;
    int to_row = to / 8, to_col = to % 8;
    if (abs(from_row - to_row) == abs(from_col - to_col))
        return is_clear_path(occupancy, from, to);
    return false;
}

bool is_valid_queen_move(int from, int to, uint64_t occupancy) {
    return is_valid_rook_move(from, to, occupancy) || is_valid_bishop_move(from, to, occupancy);
}

bool is_valid_pawn_move(int from, int to, bool white, uint64_t occupancy, uint64_t opponent) {
    int from_row = from / 8, from_col = from % 8;
    int to_row   = to / 8,   to_col   = to % 8;
    int row_diff = to_row - from_row;
    int col_diff = to_col - from_col;
    if (white) {
        if (col_diff == 0 && row_diff == 1 && !(occupancy & (1ULL << to)))
            return true;
        if (col_diff == 0 && row_diff == 2 && from_row == 1 &&
            !(occupancy & (1ULL << (from + 8))) && !(occupancy & (1ULL << to)))
            return true;
        if (abs(col_diff) == 1 && row_diff == 1 && (opponent & (1ULL << to)))
            return true;
    } else {
        if (col_diff == 0 && row_diff == -1 && !(occupancy & (1ULL << to)))
            return true;
        if (col_diff == 0 && row_diff == -2 && from_row == 6 &&
            !(occupancy & (1ULL << (from - 8))) && !(occupancy & (1ULL << to)))
            return true;
        if (abs(col_diff) == 1 && row_diff == -1 && (opponent & (1ULL << to)))
            return true;
    }
    return false;
}

bool move(board *BOARD, const char* from, const char* to) {
    int from_index = square_index(from);
    int to_index   = square_index(to);
    uint64_t from_mask = 1ULL << from_index;
    uint64_t to_mask   = 1ULL << to_index;
    uint64_t occupancy = BOARD->WHITE_PIECES | BOARD->BLACK_PIECES;
    bool whiteTurn = BOARD->turn;
    uint64_t friendly, enemy;

    if (whiteTurn) {
        friendly = BOARD->WHITE_PIECES;
        enemy    = BOARD->BLACK_PIECES;
    } else {
        friendly = BOARD->BLACK_PIECES;
        enemy    = BOARD->WHITE_PIECES;
    }

    if (!(from_mask & friendly))
        return false;
    if (to_mask & friendly)
        return false;

    bool valid = false;

    if (whiteTurn) {
        if (from_mask & BOARD->WHITE_PAWNS) {
            valid = is_valid_pawn_move(from_index, to_index, true, occupancy, enemy);
            if (valid) {
                BOARD->WHITE_PAWNS ^= from_mask;
                BOARD->WHITE_PAWNS |= to_mask;
                if ((from_mask & BOARD->WHITE_PAWNS) && (to_mask & 0xFF00000000000000ULL)) {
                    BOARD->WHITE_PAWNS ^= to_mask;
                    BOARD->WHITE_QUEEN |= to_mask;
                }

            }
        } else if (from_mask & BOARD->WHITE_KNIGHTS) {
            valid = is_valid_knight_move(from_index, to_index);
            if (valid) {
                BOARD->WHITE_KNIGHTS ^= from_mask;
                BOARD->WHITE_KNIGHTS |= to_mask;
            }
        } else if (from_mask & BOARD->WHITE_BISHOPS) {
            valid = is_valid_bishop_move(from_index, to_index, occupancy);
            if (valid) {
                BOARD->WHITE_BISHOPS ^= from_mask;
                BOARD->WHITE_BISHOPS |= to_mask;
            }
        } else if (from_mask & BOARD->WHITE_ROOKS) {
            valid = is_valid_rook_move(from_index, to_index, occupancy);
            if (valid) {
                BOARD->WHITE_ROOKS ^= from_mask;
                BOARD->WHITE_ROOKS |= to_mask;
            }
        } else if (from_mask & BOARD->WHITE_QUEEN) {
            valid = is_valid_queen_move(from_index, to_index, occupancy);
            if (valid) {
                BOARD->WHITE_QUEEN ^= from_mask;
                BOARD->WHITE_QUEEN |= to_mask;
            }
        } else if (from_mask & BOARD->WHITE_KING) {
            valid = is_valid_king_move(from_index, to_index);
            if (valid) {
                BOARD->WHITE_KING ^= from_mask;
                BOARD->WHITE_KING |= to_mask;
            }
        }
        if (valid && (to_mask & enemy)) {
            if (to_mask & BOARD->BLACK_PAWNS)   BOARD->BLACK_PAWNS   ^= to_mask;
            if (to_mask & BOARD->BLACK_KNIGHTS) BOARD->BLACK_KNIGHTS ^= to_mask;
            if (to_mask & BOARD->BLACK_BISHOPS) BOARD->BLACK_BISHOPS ^= to_mask;
            if (to_mask & BOARD->BLACK_ROOKS)   BOARD->BLACK_ROOKS   ^= to_mask;
            if (to_mask & BOARD->BLACK_QUEEN)   BOARD->BLACK_QUEEN   ^= to_mask;
            if (to_mask & BOARD->BLACK_KING)    BOARD->BLACK_KING    ^= to_mask;
        }
    } else {
        if (from_mask & BOARD->BLACK_PAWNS) {
            valid = is_valid_pawn_move(from_index, to_index, false, occupancy, enemy);
            if (valid) {
                BOARD->BLACK_PAWNS ^= from_mask;
                BOARD->BLACK_PAWNS |= to_mask;
                if (to_mask & 0x00000000000000FFULL) {
                    BOARD->BLACK_PAWNS ^= to_mask;
                    BOARD->BLACK_QUEEN |= to_mask;
                }

            }
        } else if (from_mask & BOARD->BLACK_KNIGHTS) {
            valid = is_valid_knight_move(from_index, to_index);
            if (valid) {
                BOARD->BLACK_KNIGHTS ^= from_mask;
                BOARD->BLACK_KNIGHTS |= to_mask;
            }
        } else if (from_mask & BOARD->BLACK_BISHOPS) {
            valid = is_valid_bishop_move(from_index, to_index, occupancy);
            if (valid) {
                BOARD->BLACK_BISHOPS ^= from_mask;
                BOARD->BLACK_BISHOPS |= to_mask;
            }
        } else if (from_mask & BOARD->BLACK_ROOKS) {
            valid = is_valid_rook_move(from_index, to_index, occupancy);
            if (valid) {
                BOARD->BLACK_ROOKS ^= from_mask;
                BOARD->BLACK_ROOKS |= to_mask;
            }
        } else if (from_mask & BOARD->BLACK_QUEEN) {
            valid = is_valid_queen_move(from_index, to_index, occupancy);
            if (valid) {
                BOARD->BLACK_QUEEN ^= from_mask;
                BOARD->BLACK_QUEEN |= to_mask;
            }
        } else if (from_mask & BOARD->BLACK_KING) {
            valid = is_valid_king_move(from_index, to_index);
            if (valid) {
                BOARD->BLACK_KING ^= from_mask;
                BOARD->BLACK_KING |= to_mask;
            }
        }
        if (valid && (to_mask & enemy)) {
            if (to_mask & BOARD->WHITE_PAWNS)   BOARD->WHITE_PAWNS   ^= to_mask;
            if (to_mask & BOARD->WHITE_KNIGHTS) BOARD->WHITE_KNIGHTS ^= to_mask;
            if (to_mask & BOARD->WHITE_BISHOPS) BOARD->WHITE_BISHOPS ^= to_mask;
            if (to_mask & BOARD->WHITE_ROOKS)   BOARD->WHITE_ROOKS   ^= to_mask;
            if (to_mask & BOARD->WHITE_QUEEN)   BOARD->WHITE_QUEEN   ^= to_mask;
            if (to_mask & BOARD->WHITE_KING)    BOARD->WHITE_KING    ^= to_mask;
        }
    }

    if (valid) {
        updateMasks(BOARD);
        toggleMove(BOARD);
        return true;
    }
    return false;
}

struct MoveList{
    char *to,*from;
    struct MoveList *next;
};

struct MoveList* generate_moves(board *BOARD) {
    bool whiteTurn = BOARD->turn;
    struct MoveList *moves = malloc(sizeof(struct MoveList));
    struct MoveList *pointer = moves;
    moves->next = NULL;

    uint64_t friendly, enemy;
    if (whiteTurn) {
        friendly = BOARD->WHITE_PIECES;
        enemy    = BOARD->BLACK_PIECES;
    } else {
        friendly = BOARD->BLACK_PIECES;
        enemy    = BOARD->WHITE_PIECES;
    }
    uint64_t occupancy = BOARD->WHITE_PIECES | BOARD->BLACK_PIECES;

    for (int from = 0; from < 64; from++) {
        uint64_t from_mask = 1ULL << from;
        if (!(friendly & from_mask))
            continue;

        bool isPawn = false, isKnight = false, isBishop = false, isRook = false, isQueen = false, isKing = false;
        if (whiteTurn) {
            if (BOARD->WHITE_PAWNS & from_mask) isPawn = true;
            else if (BOARD->WHITE_KNIGHTS & from_mask) isKnight = true;
            else if (BOARD->WHITE_BISHOPS & from_mask) isBishop = true;
            else if (BOARD->WHITE_ROOKS & from_mask) isRook = true;
            else if (BOARD->WHITE_QUEEN & from_mask) isQueen = true;
            else if (BOARD->WHITE_KING & from_mask) isKing = true;
        } else {
            if (BOARD->BLACK_PAWNS & from_mask) isPawn = true;
            else if (BOARD->BLACK_KNIGHTS & from_mask) isKnight = true;
            else if (BOARD->BLACK_BISHOPS & from_mask) isBishop = true;
            else if (BOARD->BLACK_ROOKS & from_mask) isRook = true;
            else if (BOARD->BLACK_QUEEN & from_mask) isQueen = true;
            else if (BOARD->BLACK_KING & from_mask) isKing = true;
        }

        for (int to = 0; to < 64; to++) {
            if (from == to)
                continue;
            uint64_t to_mask = 1ULL << to;

            if (friendly & to_mask)
                continue;

            bool valid = false;
            if (isPawn)
                valid = is_valid_pawn_move(from, to, whiteTurn, occupancy, enemy);
            else if (isKnight)
                valid = is_valid_knight_move(from, to);
            else if (isBishop)
                valid = is_valid_bishop_move(from, to, occupancy);
            else if (isRook)
                valid = is_valid_rook_move(from, to, occupancy);
            else if (isQueen)
                valid = is_valid_queen_move(from, to, occupancy);
            else if (isKing)
                valid = is_valid_king_move(from, to);

            if (valid) {
                char fromSquare[3], toSquare[3];
                fromSquare[0] = 'h' - (from % 8);
                fromSquare[1] = '1' + (from / 8);
                fromSquare[2] = '\0';
                toSquare[0] = 'h' - (to % 8);
                toSquare[1] = '1' + (to / 8);
                toSquare[2] = '\0';

                struct MoveList *new_move = malloc(sizeof(struct MoveList));
                new_move->from = strdup(fromSquare);
                new_move->to = strdup(toSquare);
                new_move->next = NULL;
                pointer->next = new_move;
                pointer = new_move;
            }
        }
    }

    return moves->next;
}




_Float16 evaluatePosition(board *BOARD);


int count_bits(uint64_t bb) {
    int count = 0;
    while (bb) {
        count++;
        bb &= (bb - 1);
    }
    return count;
}


void get_captures(board *BOARD, bool white_turn, int *num_captures, char captures[][5]) {
    *num_captures = 0;
    uint64_t friendly = white_turn ? BOARD->WHITE_PIECES : BOARD->BLACK_PIECES;
    uint64_t enemy = white_turn ? BOARD->BLACK_PIECES : BOARD->WHITE_PIECES;
    uint64_t occupancy = friendly | enemy;

    for (int from = 0; from < 64; from++) {
        uint64_t from_mask = 1ULL << from;
        if (!(friendly & from_mask)) continue;

        for (int to = 0; to < 64; to++) {
            uint64_t to_mask = 1ULL << to;
            if (!(enemy & to_mask)) continue;

            bool valid = false;
            if ((white_turn ? BOARD->WHITE_PAWNS : BOARD->BLACK_PAWNS) & from_mask)
                valid = is_valid_pawn_move(from, to, white_turn, occupancy, enemy);
            else if ((white_turn ? BOARD->WHITE_KNIGHTS : BOARD->BLACK_KNIGHTS) & from_mask)
                valid = is_valid_knight_move(from, to);
            else if ((white_turn ? BOARD->WHITE_BISHOPS : BOARD->BLACK_BISHOPS) & from_mask)
                valid = is_valid_bishop_move(from, to, occupancy);
            else if ((white_turn ? BOARD->WHITE_ROOKS : BOARD->BLACK_ROOKS) & from_mask)
                valid = is_valid_rook_move(from, to, occupancy);
            else if ((white_turn ? BOARD->WHITE_QUEEN : BOARD->BLACK_QUEEN) & from_mask)
                valid = is_valid_queen_move(from, to, occupancy);
            else if ((white_turn ? BOARD->WHITE_KING : BOARD->BLACK_KING) & from_mask)
                valid = is_valid_king_move(from, to);

            if (valid) {
                captures[*num_captures][0] = 'h' - (from % 8);
                captures[*num_captures][1] = '1' + (from / 8);
                captures[*num_captures][2] = 'h' - (to % 8);
                captures[*num_captures][3] = '1' + (to / 8);
                captures[*num_captures][4] = '\0';
                (*num_captures)++;
            }
        }
    }
}


_Float16 evaluatePosition(board *BOARD) {
    if (!BOARD->BLACK_KING) return INFINITY;
    else if (!BOARD->WHITE_KING) return -INFINITY;
    _Float16 score = 0;
    score += count_bits(BOARD->WHITE_PAWNS) * PAWN_VALUE;
    score += count_bits(BOARD->WHITE_KNIGHTS) * KNIGHT_VALUE;
    score += count_bits(BOARD->WHITE_BISHOPS) * BISHOP_VALUE;
    score += count_bits(BOARD->WHITE_ROOKS) * ROOK_VALUE;
    score += count_bits(BOARD->WHITE_QUEEN) * QUEEN_VALUE;
    score += count_bits(BOARD->WHITE_KING) * KING_VALUE;

    score -= count_bits(BOARD->BLACK_PAWNS) * PAWN_VALUE;
    score -= count_bits(BOARD->BLACK_KNIGHTS) * KNIGHT_VALUE;
    score -= count_bits(BOARD->BLACK_BISHOPS) * BISHOP_VALUE;
    score -= count_bits(BOARD->BLACK_ROOKS) * ROOK_VALUE;
    score -= count_bits(BOARD->BLACK_QUEEN) * QUEEN_VALUE;
    score -= count_bits(BOARD->BLACK_KING) * KING_VALUE;


    uint64_t bb;
    _Float16 bonus = 0;
    int sq;
    bb = BOARD->WHITE_PAWNS;
    while (bb) {
        sq = __builtin_ctzll(bb);
        bonus += pawn_table[sq];
        bb &= (bb - 1);
    }

    bb = BOARD->WHITE_KNIGHTS;
    while (bb) {
        sq = __builtin_ctzll(bb);
        bonus += knight_table[sq];
        bb &= (bb - 1);
    }

    bb = BOARD->WHITE_BISHOPS;
    while (bb) {
        sq = __builtin_ctzll(bb);
        bonus += bishop_table[sq];
        bb &= (bb - 1);
    }


    bb = BOARD->BLACK_PAWNS;
    while (bb) {
        sq = __builtin_ctzll(bb);
        bonus -= pawn_table[63 - sq];
        bb &= (bb - 1);
    }

    bb = BOARD->BLACK_KNIGHTS;
    while (bb) {
        sq = __builtin_ctzll(bb);
        bonus -= knight_table[63 - sq];
        bb &= (bb - 1);
    }

    bb = BOARD->BLACK_BISHOPS;
    while (bb) {
        sq = __builtin_ctzll(bb);
        bonus -= bishop_table[63 - sq];
        bb &= (bb - 1);
    }
    return (score + bonus/20)/100;
}



_Float16 evaluateBoard(board *BOARD) {
    return evaluatePosition(BOARD);
}

_Float16 evaluateMove(board *BOARD, struct MoveList *MOVE){
    board new_board;
    memcpy(&new_board, BOARD, sizeof(board));
    move(&new_board, MOVE->from, MOVE->to);
    return evaluateBoard(&new_board);
}

movepair* minimax(board *BOARD, int depth, _Float16 alpha, _Float16 beta, bool maximizingPlayer) {
    if (depth <= 0) {
        movepair *leaf = malloc(sizeof(movepair));
        leaf->from = NULL;
        leaf->to = NULL;
        leaf->eval = evaluateBoard(BOARD);
        return leaf;
    }

    Transposition *transpose;
    if (transpose = getOrDefault(BOARD, depth)){
        saved += 1;
        return transpose->best_move;
    }
    totalCount++;
    movepair *bestMove = malloc(sizeof(movepair));
    bestMove->eval = maximizingPlayer ? -INFINITY : INFINITY;

    struct MoveList *movelist = generate_moves(BOARD);
    while (movelist){
        board newBoard = *BOARD;
        move(&newBoard, movelist->from, movelist->to);
        MOVES++;
        movepair *evalMove = minimax(&newBoard, depth - 1, alpha, beta, !maximizingPlayer);
        if (maximizingPlayer) {
            if (evalMove->eval > bestMove->eval) {
                bestMove->from = movelist->from;
                bestMove->to = movelist->to;
                bestMove->eval = evalMove->eval;
            }
            alpha = fmax(alpha, bestMove->eval);
        } else {
            if (evalMove->eval < bestMove->eval) {
                movepair *new_move = malloc(sizeof(movepair));
                bestMove->from = movelist->from;
                bestMove->to = movelist->to;
                bestMove->eval = evalMove->eval;
            }
            beta = fmin(beta, bestMove->eval);
        }
        MOVES--;
        if (beta <= alpha) break;
        movelist = movelist->next;
    }

    Transposition *new_transposition = malloc(sizeof(Transposition));
    if (!new_transposition) return bestMove;
    new_transposition->best_move = bestMove;
    new_transposition->BOARD = *BOARD;
    new_transposition->depth = depth;
    addTransposition(new_transposition);
    return bestMove;
}

movepair* searchBestMove(board *BOARD, int depth) {
    return minimax(BOARD, depth, -INFINITY, INFINITY, BOARD->turn);
}

void playGame(board *BOARD, int depth,int moves){
        for (int _ = 0; _<moves; _++){
            movepair *best_move;
            best_move = searchBestMove(BOARD,depth);
            printf("%d > %s's turn = ",MOVES, BOARD->turn?"White":"Black");
            printf("%s -> %s (%f) || Positions : %llu || Saved : %llu|| Collisions : %llu\n", best_move->from, best_move->to, (double)best_move->eval, totalCount, saved, collisions);
            move(BOARD, best_move->from, best_move->to);
            totalCount = 0;
            saved =0;
            MOVES++;
            collisions = 0;
            if (!(BOARD->BLACK_KING && BOARD->WHITE_KING))return;
        }
    }

void playAlong(board *BOARD, int depth){
    while (true){
        char movestr[4];
        printf("Your turn :\t");
        scanf("%s", movestr);
        char from[2] = { movestr[0], movestr[1] };
        char to[2] = { movestr[2], movestr[3] };
        move(BOARD, from,to);
        playGame(BOARD, depth, 1);
    }
}

int main(void) {
    board BOARD;
    init_table();
    setupRandomTable();
    setupBoard(&BOARD);
    updateMasks(&BOARD);
    struct MoveList *moves = malloc(sizeof(struct MoveList));
    moves = generate_moves(&BOARD);
    playGame(&BOARD,DEPTH,TOTAL_MOVES);
    // playAlong(&BOARD, DEPTH);
    return 0;
}
