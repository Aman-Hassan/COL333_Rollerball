#include <algorithm>
#include <random>
#include <iostream>
#include <limits>
#include <chrono>

#include "board.hpp"
#include "engine.hpp"

typedef uint8_t U8;
typedef uint16_t U16;

int global_cutoff = 4;
std::vector<std::string> moves_taken;
std::vector<U8> last_killed_pieces;
std::vector<int> last_killed_pieces_idx;

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

void do_move(Board *b, U16 move)
{

    U8 p0 = getp0(move);
    U8 p1 = getp1(move);
    U8 promo = getpromo(move);
    U8 piecetype = b->data.board_0[p0];
    last_killed_pieces.push_back(0);
    last_killed_pieces_idx.push_back(-1);

    // scan and get piece from coord
    U8 *pieces = (U8 *)b;
    for (int i = 0; i < 12; i++)
    {
        if (pieces[i] == p1)
        {
            pieces[i] = DEAD;
            last_killed_pieces.back() = b->data.board_0[p1];
            last_killed_pieces_idx.back() = i;
        }
        if (pieces[i] == p0)
        {
            pieces[i] = p1;
        }
    }

    if (promo == PAWN_ROOK)
    {
        piecetype = (piecetype & (WHITE | BLACK)) | ROOK;
    }
    else if (promo == PAWN_BISHOP)
    {
        piecetype = (piecetype & (WHITE | BLACK)) | BISHOP;
    }

    b->data.board_0[p1] = piecetype;
    b->data.board_90[cw_90[p1]] = piecetype;
    b->data.board_180[cw_180[p1]] = piecetype;
    b->data.board_270[acw_90[p1]] = piecetype;

    b->data.board_0[p0] = 0;
    b->data.board_90[cw_90[p0]] = 0;
    b->data.board_180[cw_180[p0]] = 0;
    b->data.board_270[acw_90[p0]] = 0;

    b->data.player_to_play = (PlayerColor)(b->data.player_to_play ^ (WHITE | BLACK)); // flipping player
    // std::cout << "Did last move\n";
    // std::cout << all_boards_to_str(*this);
}

void undo_last_move(Board *b, U16 move)
{
    U8 p0 = getp0(move);
    U8 p1 = getp1(move);
    U8 promo = getpromo(move);

    U8 piecetype = b->data.board_0[p1];
    U8 deadpiece = 0;

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

    if (last_killed_pieces_idx.back() != -1)
    {
        deadpiece = last_killed_pieces.back(); // updating deadpiece if there is something in vector
        pieces[last_killed_pieces_idx.back()] = p1;
    }

    last_killed_pieces.pop_back();
    last_killed_pieces_idx.pop_back();

    if (promo == PAWN_ROOK)
    {
        piecetype = ((piecetype & (WHITE | BLACK)) ^ ROOK) | PAWN;
    }
    else if (promo == PAWN_BISHOP)
    {
        piecetype = ((piecetype & (WHITE | BLACK)) ^ BISHOP) | PAWN;
    }

    b->data.board_0[p0] = piecetype;
    b->data.board_90[cw_90[p0]] = piecetype;
    b->data.board_180[cw_180[p0]] = piecetype;
    b->data.board_270[acw_90[p0]] = piecetype;

    b->data.board_0[p1] = deadpiece;
    b->data.board_90[cw_90[p1]] = deadpiece;
    b->data.board_180[cw_180[p1]] = deadpiece;
    b->data.board_270[acw_90[p1]] = deadpiece;

    b->data.player_to_play = (PlayerColor)(b->data.player_to_play ^ (WHITE | BLACK)); // flipping player again

    // std::cout << "Undid last move\n";
    // std::cout << all_boards_to_str(*this);
}

// No of white - No of black
float material_check(const Board *b)
{
    float val = 0;
    float weight_arr[12] = {8, 8, 0, 4, 2, 2, 8, 8, 0, 4, 2, 2};
    U8 *pieces = (U8 *)(&(b->data));
    for (int i = 0; i < 12; i++)
    {
        if (pieces[i] != DEAD)
        {
            val += ((int(i >= 6) - int(i < 6))) * weight_arr[i];
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

// float order_helper(Board *b,U16 move){
//     do_move(b,move);
//     float val = eval_fn(b);
//     undo_last_move(b,move);
//     return val;
// }

// std::vector<U16> order_moves(std::unordered_set<U16> move_set)
// {
//     std::vector<U16> ordered_moves(move_set.begin(), move_set.end());
//     std::sort(ordered_moves.begin(), ordered_moves.end(), order_helper);
//     return ordered_moves;
// }

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
    if (move != 0)
    {
        std::cout << "Next Move to take: " << move_to_str(move) << std::endl;
        std::cout << "Present Depth:" << global_cutoff - cutoff << std::endl;
        std::cout << "\n";
    }
}

void print_moveset(auto moveset)
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
    if (cutoff == 0 || moveset.size() == 0)
    {
        return eval_fn(b);
    }

    float min_val = std::numeric_limits<float>::max();
    for (auto m : moveset)
    {
        do_move(b, m);
        // moves_taken.push_back(move_to_str(m));
        float child = MaxVal(b, alpha, beta, cutoff - 1);
        beta = std::min(beta, child);
        min_val = std::min(min_val, child);
        // moves_taken.pop_back();
        undo_last_move(b, m);
        if (alpha >= beta)
        {
            break;
        }
    }
    return min_val;
}

float MaxVal(Board *b, float alpha, float beta, int cutoff)
{
    auto moveset = b->get_legal_moves();
    if (cutoff == 0 || moveset.size() == 0)
    {
        return eval_fn(b);
    }
    float max_val = std::numeric_limits<float>::lowest();
    for (auto m : moveset)
    {
        do_move(b, m);
        // moves_taken.push_back(move_to_str(m));
        float child = MinVal(b, alpha, beta, cutoff - 1);
        alpha = std::max(alpha, child);
        max_val = std::max(max_val, child);
        // moves_taken.pop_back();
        undo_last_move(b, m);
        if (alpha >= beta)
        {
            break;
        }
    }
    return max_val;
}

// U16 bestmove = 0;

U16 Minimax(Board *b, int cutoff)
{
    auto moveset = b->get_legal_moves();
    print_moveset(moveset);
    U16 bestmove = 0;

    float alpha = std::numeric_limits<float>::lowest(); // ::min() would only give least positive float instead of smallest float
    float beta = std::numeric_limits<float>::max();
    if (b->data.player_to_play == WHITE) // Runs maxval if white
    {
        for (auto m : moveset)
        {
            do_move(b, m);
            // moves_taken.push_back(move_to_str(m));
            float child = MinVal(b, alpha, beta, cutoff - 1);
            if (child > alpha)
                bestmove = m;
            alpha = std::max(alpha, child);
            undo_last_move(b, m);
            // moves_taken.pop_back();
        }
    }
    else
    {
        for (auto m : moveset)
        {
            do_move(b, m);
            float child = MaxVal(b, alpha, beta, cutoff - 1);
            if (child < beta)
                bestmove = m;
            beta = std::min(beta, child);
            undo_last_move(b, m);
        }
    }
    return bestmove;
}

U16 best_move_obtained = 0;

float unified_minimax(Board *b, int cutoff, float alpha, float beta, bool Maximizing)
{
    // bool is_sorted = false;
    if (cutoff == 0)
    {
        return eval_fn(b);
    }
    std::unordered_set<U16> moveset = b->get_legal_moves();
    if (moveset.size() == 0)
    {
        return eval_fn(b);
    }
    // Ordering the values using lambda function:
    std::vector<U16> ordered_moveset(moveset.begin(), moveset.end());

    auto order_moves = [b](U16 move1, U16 move2)
    {
        do_move(b, move1);
        float val1 = eval_fn(b);
        undo_last_move(b, move1);

        do_move(b, move2);
        float val2 = eval_fn(b);
        undo_last_move(b, move2);

        return val1 > val2;
    };

    std::sort(ordered_moveset.begin(), ordered_moveset.end(), order_moves); // Sorted in descending order

    // print_moveset(moveset);
    // std::cout << "\nOrdered Moveset->\n\n";
    // print_moveset(ordered_moveset);
    // std::cout << "\n";

    if (Maximizing)
    {
        float max_eval = std::numeric_limits<float>::lowest();
        for (auto m : ordered_moveset)
        {
            do_move(b, m);
            float eval = unified_minimax(b, cutoff - 1, alpha, beta, false);
            undo_last_move(b, m);
            max_eval = std::max(max_eval, eval);
            // std::cout << "For the move: " << move_to_str(m) << " , The eval is " << eval << " , Maxeval = " << max_eval << "\n";
            if (cutoff == global_cutoff && eval > alpha)
            {
                best_move_obtained = m;
            }
            alpha = std::max(alpha, eval);
            if (alpha >= beta)
            {
                break;
            }
        }
        return max_eval;
    }
    else
    {
        std::reverse(ordered_moveset.begin(), ordered_moveset.end());

        float min_eval = std::numeric_limits<float>::max();
        for (auto m : ordered_moveset)
        {
            do_move(b, m);
            float eval = unified_minimax(b, cutoff - 1, alpha, beta, true);
            undo_last_move(b, m);
            min_eval = std::min(min_eval, eval);
            if (cutoff == global_cutoff && eval < beta)
            {
                best_move_obtained = m;
            }
            beta = std::min(beta, eval);
            if (alpha >= beta)
            {
                break;
            }
        }
        return min_eval;
    }
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
        // std::cout << "\n\n\n\n\n\n\nStart find_best_move()\nBoard state:\n";
        std::cout << all_boards_to_str(b) << "\n Legal moves:" << std::endl;
        for (auto m : moveset)
        {
            std::cout << move_to_str(m) << " ";
        }
        std::cout << std::endl;
        Board *b_copy = b.copy();

        auto start = std::chrono::high_resolution_clock::now();
        unified_minimax(b_copy, global_cutoff, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), b.data.player_to_play == WHITE);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        if (duration.count() < 2000)
            this->best_move = best_move_obtained;
        std::cout << "Best move chosen:" << move_to_str(best_move_obtained) << std::endl;
        delete b_copy;
    }
}

// void Engine::find_best_move(const Board &b)
// {

//     // pick a random move

//     auto moveset = b.get_legal_moves();
//     if (moveset.size() == 0)
//     {
//         this->best_move = 0;
//     }
//     else
//     {
//         std::vector<U16> moves;
//         std::cout << all_boards_to_str(b) << std::endl;
//         for (auto m : moveset)
//         {
//             std::cout << move_to_str(m) << " ";
//         }
//         std::cout << std::endl;
//         std::sample(
//             moveset.begin(),
//             moveset.end(),
//             std::back_inserter(moves),
//             1,
//             std::mt19937{std::random_device{}()});
//         this->best_move = moves[0];
//     }
// }
