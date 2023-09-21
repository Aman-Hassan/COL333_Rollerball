#include <algorithm>
#include <random>
#include <iostream>
#include <limits>

#include "board.hpp"
#include "engine.hpp"

typedef uint8_t U8;
typedef uint16_t U16;

int global_cutoff = 2;
std::vector<std::string> moves_taken;

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
    for (int i = 0; i < 12; i++)
    {
        std::cout << "checking " << piece_to_char(b->data.board_0[pieces[i]]) << " weight " << weight_arr[i] << "\n";
        if (pieces[i] != DEAD)
        {
            val += ((int(i >= 6) - int(i < 6))) * weight_arr[i];
            std::cout << "value obtained till now " << val << "\n";
        }
    }
    std::cout << std::endl;
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

void print_state(Board *b, U16 move, int cutoff)
{
    std::cout << "Present board state:" << std::endl;
    std::cout << all_boards_to_str(*b) << std::endl;
    std::cout << "Moves taken till now: ";
    for (auto m : moves_taken)
    {
        std::cout << m << " ";
    }
    std::cout << std::endl;
    std::cout << "Next Move to take: " << move_to_str(move) << std::endl;
    std::cout << "Present Depth:" << global_cutoff - cutoff << std::endl;
    std::cout << "\n";
}

void print_moveset(std::unordered_set<U16> moveset)
{
    std::cout << "Moves that can be taken from this node: ";
    for (auto m : moveset)
    {
        std::cout << move_to_str(m) << " ";
    }
    std::cout << std::endl;
}

float MinVal(Board *b, float alpha, float beta, int cutoff)
{
    auto moveset = b->get_legal_moves();
    std::cout << "Entered New MinVal\n\n";
    print_moveset(moveset); // Print Moveset at beginning of node
    if (cutoff == 0 || moveset.size() == 0)
    {
        std::cout << "Reached cutoff/Moveset size 0, printing eval function: \n\n";
        return std::min(beta, eval_fn(b));
    }

    for (auto m : moveset)
    {
        print_state(b, m, cutoff); // Print the board state, moves taken till now, next move to take and current depth
        b->do_move(m);
        moves_taken.push_back(move_to_str(m));
        float child = MaxVal(b, alpha, beta, cutoff - 1);
        std::cout << "Back at MinVal, Returned Value from Maxval at depth " << global_cutoff + 1 - cutoff << ":" << child << std::endl;
        beta = std::min(beta, child);
        undo_last_move(b, m);
        moves_taken.pop_back();
        if (alpha >= beta)
            std::cout << "Pruned at depth " << global_cutoff - cutoff << "\n\n";
        return child;
    }
    std::cout << "Minval at depth " << global_cutoff - cutoff << "ended \n\n";
    return beta;
}

float MaxVal(Board *b, float alpha, float beta, int cutoff)
{
    auto moveset = b->get_legal_moves();
    std::cout << "Entered New MaxVal\n";
    print_moveset(moveset);
    if (cutoff == 0 || moveset.size() == 0)
    {
        std::cout << "Reached cutoff/Moveset size 0, printing eval function: \n\n";
        return std::max(alpha, eval_fn(b));
    }
    for (auto m : moveset)
    {
        print_state(b, m, cutoff);
        b->do_move(m);
        moves_taken.push_back(move_to_str(m));
        float child = MinVal(b, alpha, beta, cutoff - 1);
        std::cout << "Back at MaxVal, Returned Value from Minval at depth " << global_cutoff + 1 - cutoff << ":" << child << std::endl;
        alpha = std::max(alpha, child);
        undo_last_move(b, m);
        moves_taken.pop_back();
        if (alpha >= beta)
            std::cout << "Pruned at depth " << global_cutoff - cutoff << "\n\n";
        return child;
    }
    std::cout << "Maxval at depth " << global_cutoff - cutoff << "ended"
              << "\n\n";
    return alpha;
}

U16 bestmove = 0;

U16 Minimax(Board *b, int cutoff)
{
    auto moveset = b->get_legal_moves();
    std::cout << "Starting new Minimax\n";
    print_moveset(moveset);
    U16 bestmove = 0;

    float alpha = std::numeric_limits<float>::lowest(); // ::min() would only give least positive float instead of smallest float
    float beta = std::numeric_limits<float>::max();
    if (b->data.player_to_play == WHITE) // Runs maxval if white
    {
        for (auto m : moveset)
        {
            std::cout << "Back at Minimax" << std::endl;
            print_state(b, m, cutoff);
            b->do_move(m);
            moves_taken.push_back(move_to_str(m));
            float child = MinVal(b, alpha, beta, cutoff - 1);
            alpha = std::max(alpha, child);
            undo_last_move(b, m);
            moves_taken.pop_back();
            if (alpha == child)
                bestmove = m;
            if (alpha >= beta)
                return bestmove;
        }
    }
    else
    {
        for (auto m : moveset)
        {
            b->do_move(m);
            float child = MaxVal(b, alpha, beta, cutoff - 1);
            beta = std::min(beta, child);
            undo_last_move(b, m);
            if (beta == child)
                bestmove = m;
            if (alpha >= beta)
                return bestmove;
        }
    }
    std::cout << "MiniMax Ended" << std::endl;
    return bestmove;
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
        U16 bestmove = Minimax(b_copy, global_cutoff);
        this->best_move = bestmove;
        delete b_copy;
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
