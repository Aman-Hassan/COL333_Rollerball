#include <algorithm>
#include <random>
#include <iostream>
#include <limits>

#include "board.hpp"
#include "engine.hpp"

typedef uint8_t U8;
typedef uint16_t U16;

float MinVal(Board *b, float alpha, float beta, int cutoff);
float MaxVal(Board *b, float alpha, float beta, int cutoff);

constexpr U8 cw_90[64] = {
    48, 40, 32, 24, 16, 8, 0, 7,
    49, 41, 33, 25, 17, 9, 1, 15,
    50, 42, 18, 19, 20, 10, 2, 23,
    51, 43, 26, 27, 28, 11, 3, 31,
    52, 44, 34, 35, 36, 12, 4, 39,
    53, 45, 37, 29, 21, 13, 5, 47,
    54, 46, 38, 30, 22, 14, 6, 55,
    56, 57, 58, 59, 60, 61, 62, 63};

constexpr U8 acw_90[64] = {
    6, 14, 22, 30, 38, 46, 54, 7,
    5, 13, 21, 29, 37, 45, 53, 15,
    4, 12, 18, 19, 20, 44, 52, 23,
    3, 11, 26, 27, 28, 43, 51, 31,
    2, 10, 34, 35, 36, 42, 50, 39,
    1, 9, 17, 25, 33, 41, 49, 47,
    0, 8, 16, 24, 32, 40, 48, 55,
    56, 57, 58, 59, 60, 61, 62, 63};

constexpr U8 cw_180[64] = {
    54, 53, 52, 51, 50, 49, 48, 7,
    46, 45, 44, 43, 42, 41, 40, 15,
    38, 37, 18, 19, 20, 33, 32, 23,
    30, 29, 26, 27, 28, 25, 24, 31,
    22, 21, 34, 35, 36, 17, 16, 39,
    14, 13, 12, 11, 10, 9, 8, 47,
    6, 5, 4, 3, 2, 1, 0, 55,
    56, 57, 58, 59, 60, 61, 62, 63};

constexpr U8 id[64] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63};

void undo_last_move(Board *b, U16 move)
{

    U8 p0 = getp0(move);
    U8 p1 = getp1(move);
    U8 promo = getpromo(move);

    U8 piecetype = b->data.board_0[p1];
    U8 deadpiece = b->data.last_killed_piece;
    b->data.last_killed_piece = 0;

    // scan and get piece from coord
    U8 *pieces = (U8 *)(&(b->data));
    for (int i = 0; i < 12; i++)
    {
        if (pieces[i] == p1)
        {
            pieces[i] = p0;
            break;
        }
    }
    if (b->data.last_killed_piece_idx >= 0)
    {
        pieces[b->data.last_killed_piece_idx] = p1;
        b->data.last_killed_piece_idx = -1;
    }

    if (promo == PAWN_ROOK)
    {
        piecetype = (piecetype & (WHITE | BLACK)) | ROOK;
    }
    else if (promo == PAWN_BISHOP)
    {
        piecetype = (piecetype & (WHITE | BLACK)) | BISHOP;
    }

    b->data.board_0[p0] = piecetype;
    b->data.board_90[cw_90[p0]] = piecetype;
    b->data.board_180[cw_180[p0]] = piecetype;
    b->data.board_270[acw_90[p0]] = piecetype;

    b->data.board_0[p1] = deadpiece;
    b->data.board_90[cw_90[p1]] = deadpiece;
    b->data.board_180[cw_180[p1]] = deadpiece;
    b->data.board_270[acw_90[p1]] = deadpiece;

    // // std::cout << "Undid last move\n";
    // // std::cout << all_boards_to_str(*this);
}

// No of white - No of black
float material_check(const Board *b)
{
    float val = 0;
    float weight_arr[12] = {8, 8, 0, 4, 2, 2, 8, 8, 0, 4, 2, 2};
    U8 *pieces = (U8 *)(&(b->data));
    for (int i = -6; i < 6; i++)
    {
        // // std::cout << "checking " << piece_to_char(this->data.board_0[pieces[i]]) << "\n";
        if (pieces[i + 6] != DEAD)
        {
            val += (((i >= 0) - (i < 0))) * weight_arr[i + 6];
        }
    }
    return val;
}

float check_condition(const Board *b)
{
    //
    float val = 0;
    bool player = (b->data.player_to_play == WHITE); // current player
    if (b->in_check())
    {
        // if white is in check, bad, negative
        val = 5 * std::pow(-1, int(player));
        if (b->get_legal_moves().size() == 0) // if you're checkmated
        {
            val += 500 * std::pow(-1, int(player));
        }
    }
    return val;
}

float eval_fn(const Board *b)
{
    float final_val = 0;
    final_val += material_check(b);
    final_val += check_condition(b);
    return final_val;
}

float MinVal(Board *b, float alpha, float beta, int cutoff)
{
    auto moveset = b->get_legal_moves();
    // std::cout << "Entered MinVal\n";
    if (cutoff == 0 || moveset.size() == 0)
        return std::min(beta, eval_fn(b));

    for (auto m : moveset)
    {
        b->do_move(m);
        // std::cout << "Depth:" << 2 - cutoff << std::endl;
        // std::cout << "Next Move to take:" << move_to_str(m) << "\n\n";
        float child = MaxVal(b, alpha, beta, cutoff - 1);
        // std::cout << "Returned Value from Maxval at depth " << 2 - cutoff << ":" << child << "\n\n";
        beta = std::min(beta, child);

        if (alpha >= beta)
            return child;
        undo_last_move(b, m);
    }
    return beta;
}

float MaxVal(Board *b, float alpha, float beta, int cutoff)
{
    // std::cout << "Entered MaxVal\n";
    auto moveset = b->get_legal_moves();

    if (cutoff == 0 || moveset.size() == 0)
        return std::max(alpha, eval_fn(b));
    for (auto m : moveset)
    {
        b->do_move(m);
        // std::cout << "Depth:" << 2 - cutoff << std::endl;
        // std::cout << "Next Move to take:" << move_to_str(m) << "\n\n";
        float child = MinVal(b, alpha, beta, cutoff - 1);
        // std::cout << "Returned Value from Minval at depth " << 2 - cutoff << ":" << child << "\n\n";
        alpha = std::max(alpha, child);
        if (alpha >= beta)
            return child;
        undo_last_move(b, m);
    }
    return alpha;
}

U16 bestmove = 0;

void Minimax(Board *b, int cutoff, std::atomic_ushort *move_ptr)
{
    auto moveset = b->get_legal_moves();

    float alpha = std::numeric_limits<float>::min();
    float beta = std::numeric_limits<float>::max();
    if (b->data.player_to_play == WHITE) // Runs maxval if white
    {
        for (auto m : moveset)
        {
            b->do_move(m);
            // std::cout << "Minimax" << std::endl;
            // std::cout << "Depth:" << 1 - cutoff << std::endl;
            // std::cout << "Next Move to take:" << move_to_str(m) << "\n\n";
            float child = MinVal(b, alpha, beta, cutoff);
            alpha = std::max(alpha, child);
            if (alpha == child)
                *move_ptr = m;
            if (alpha >= beta)
                return;
            undo_last_move(b, m);
        }
    }
    else
    {
        for (auto m : moveset)
        {
            b->do_move(m);
            float child = MaxVal(b, alpha, beta, cutoff);
            beta = std::max(alpha, child);
            if (beta == child)
                *move_ptr = m;
            if (alpha >= beta)
                return;
            undo_last_move(b, m);
        }
    }

    // return bestmove;
}

void Engine::find_best_move(const Board &b)
{

    // pick a random move

    auto moveset = b.get_legal_moves();
    if (moveset.size() == 0)
    {
        this->best_move = 0;
    }
    else
    {
        std::cout << all_boards_to_str(b) << std::endl;
        for (auto m : moveset)
        {
            std::cout << move_to_str(m) << " ";
        }
        std::cout << std::endl;
        Board *b_copy = b.copy();
        Minimax(b_copy, 50, &(this->best_move));
        // this->best_move = bestmove;
    }
}

// #include <algorithm>
// #include <random>
// #include <iostream>

// #include "board.hpp"
// #include "engine.hpp"

// void Engine::find_best_move(const Board& b) {

//     // pick a random move

//     auto moveset = b.get_legal_moves();
//     if (moveset.size() == 0) {
//         this->best_move = 0;
//     }
//     else {
//         std::vector<U16> moves;
//         // std::cout << all_boards_to_str(b) << std::endl;
//         for (auto m : moveset) {
//             // std::cout << move_to_str(m) << " ";
//         }
//         // std::cout << std::endl;
//         std::sample(
//             moveset.begin(),
//             moveset.end(),
//             std::back_inserter(moves),
//             1,
//             std::mt19937{std::random_device{}()}
//         );
//         this->best_move = moves[0];
//     }
// }
