#include <algorithm>
#include <random>
#include <iostream>
#include <limits>
#include <chrono>
#include <bitset>
#include <math.h>

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
float material_check(Board *b)
{
    float val = 0;
    // float weight_arr[12] = {8, 8, 0, 4, 2, 2, 8, 8, 0, 4, 2, 2};
    float p = 2, bi = 4, r = 8;
    U8 *pieces = (U8 *)(&(b->data));
    for (int i = 0; i < 12; i++)
    {
        int temp = 0;
        if (pieces[i] != DEAD)
        {
            // val += (((int(i >= 6) - int(i < 6))) * weight_arr[i])* (pieces[i] != DEAD);

            U8 piecetype = b->data.board_0[pieces[i]];
            temp += ((piecetype & PAWN) == PAWN) * p;
            temp += ((piecetype & BISHOP) == BISHOP) * bi;
            temp += ((piecetype & ROOK) == ROOK) * r;
            temp *= std::pow(-1, (piecetype & BLACK) == BLACK);
        }
        val += temp;
    }
    return val;
}

float check_condition(Board *b)
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

float pawn_distance(Board *b)
{
    float val = 0;
    // the below are the proper non-overlapping sector regions.
    std::unordered_set<U8> bottom({1, 2, 3, 4, 5, 6, 10, 11, 12, 13});
    std::unordered_set<U8> left({0, 8, 16, 24, 32, 40, 9, 17, 25, 33});
    std::unordered_set<U8> top({48, 49, 50, 51, 52, 53, 41, 42, 43, 44});
    std::unordered_set<U8> right({54, 46, 38, 30, 22, 14, 45, 37, 29, 21});
    U8 corners[4] = {pos(5, 1), pos(1, 1), pos(1, 5), pos(5, 5)};
    U8 *pieces = (U8 *)(&(b->data));

    int ids[4] = {4, 5, 10, 11};
    for (int i = 0; i < 4; i++) // for each piece
    {
        std::bitset<4> piece_arr;
        std::bitset<4> king_arr;
        U8 piece_pos = pieces[ids[i]];
        U8 king_pos = pieces[(i > 5) * 2 + (i <= 5) * 8];
        U8 piecetype = b->data.board_0[piece_pos];

        if (piece_pos == DEAD || (piecetype & PAWN) != PAWN)
            continue;
        piece_arr[0] = bottom.count(piece_pos);
        piece_arr[1] = left.count(piece_pos);
        piece_arr[2] = top.count(piece_pos);
        piece_arr[3] = right.count(piece_pos);
        king_arr[0] = bottom.count(king_pos);
        king_arr[1] = left.count(king_pos);
        king_arr[2] = top.count(king_pos);
        king_arr[3] = right.count(king_pos);
        int sector_dist = (int)std::log2((piece_arr.to_ulong() << 4) / king_arr.to_ulong()) % 4;
        int piece_c = 0;
        int king_c = 0;
        for (int i = 0; i < 4; i++)
        {
            piece_c += (int)piece_arr[i] * corners[i];
            king_c += (int)king_arr[i] * corners[i];
        }
        int piece_dist = std::abs(getx(piece_pos) - getx(piece_c)) + std::abs(gety(piece_pos) - gety(piece_c));
        int king_dist = std::abs(getx(king_pos) - getx(king_c)) + std::abs(gety(king_pos) - gety(king_c));
        int diff = king_dist - piece_dist;
        if (diff < 0)
            diff += 40;
        int total_dist = diff + 4 * sector_dist;
        val -= total_dist * std::pow(-1, (piecetype & BLACK) == BLACK);
    }
    return val;
}

float rook_distance(Board *b)
{
    float val = 0;
    // std::unordered_set<U8> bottom({1, 2, 3, 4, 5, 6, 10, 11, 12, 13});
    // std::unordered_set<U8> left({0, 8, 16, 24, 32, 40, 9, 17, 25, 33});
    // std::unordered_set<U8> top({48, 49, 50, 51, 52, 53, 41, 42, 43, 44});
    // std::unordered_set<U8> right({54, 46, 38, 30, 22, 14, 45, 37, 29, 21});

    std::unordered_set<U8> bottom_r({2, 3, 4, 5, 6, 10, 11, 12, 13, 14});
    std::unordered_set<U8> left_r({0, 1, 8, 9, 16, 24, 32, 17, 25, 33});
    std::unordered_set<U8> top_r({50, 51, 52, 42, 43, 44, 40, 41, 48, 49});
    std::unordered_set<U8> right_r({54, 46, 38, 30, 22, 14, 45, 37, 29, 21});

    std::unordered_set<U8> bottom_l({0, 1, 8, 9, 2, 3, 4, 10, 11, 12});
    std::unordered_set<U8> left_l({40, 41, 48, 49, 16, 24, 32, 17, 25, 33});
    std::unordered_set<U8> top_l({45, 46, 53, 54, 50, 51, 52, 42, 43, 44});
    std::unordered_set<U8> right_l({5, 6, 13, 14, 38, 30, 22, 37, 29, 21});
    U8 *pieces = (U8 *)(&(b->data));

    int ids[8] = {0, 1, 4, 5, 6, 7, 10, 11}; // iterate through rooks and pawns(maybe upgraded)
    for (int i = 0; i < 8; i++)
    {
        std::bitset<4> piece_arr;
        std::bitset<4> king_arr;
        U8 piece_pos = pieces[ids[i]];
        U8 king_pos = pieces[(i > 5) * 2 + (i <= 5) * 8];
        U8 piecetype = b->data.board_0[piece_pos];

        if (piece_pos == DEAD || (piecetype & ROOK) != ROOK)
            continue;
        piece_arr[0] = bottom_r.count(piece_pos);
        piece_arr[1] = left_r.count(piece_pos);
        piece_arr[2] = top_r.count(piece_pos);
        piece_arr[3] = right_r.count(piece_pos);
        king_arr[0] = bottom_l.count(king_pos);
        king_arr[1] = left_l.count(king_pos);
        king_arr[2] = top_l.count(king_pos);
        king_arr[3] = right_l.count(king_pos);
        int sector_dist = 0;
        if ((piece_arr.to_ulong() & piece_arr.to_ulong()) > 0)
            sector_dist = 5;
        else
            sector_dist = (int)std::log2((piece_arr.to_ulong() << 4) / king_arr.to_ulong()) % 4; // 0 to 3

        // std::cout << "rook:" << rook_arr.to_ulong() << "king:" << king_arr.to_ulong() << "\n";

        val -= 8 * sector_dist * std::pow(-1, (piecetype & BLACK) == BLACK);
    }
    return val;
}

float eval_fn(Board *b)
{
    float final_val = 0;
    final_val += 10 * material_check(b);
    final_val += 3 * check_condition(b);
    // final_val += 0.01 * pawn_distance(b);
    // final_val += 0.02 * rook_distance(b);
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
    if (move != 0)
    {
        std::cout << "Next Move to take: " << move_to_str(move) << std::endl;
        std::cout << "Present Depth:" << global_cutoff - cutoff << std::endl;
        std::cout << "\n";
    }
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

U16 best_move_obtained = 0;

float unified_minimax(Board *b, int cutoff, float alpha, float beta, bool Maximizing)
{
    if (cutoff == 0)
        return eval_fn(b);
    auto moveset = b->get_legal_moves();
    if (moveset.size() == 0)
        return eval_fn(b);

    // Ordering the values using lambda function:
    std::vector<U16> ordered_moveset(moveset.begin(), moveset.end());

    // if (cutoff < 4)
    // {
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
    //     is_sorted = true;
    // }

    // print_moveset(moveset);
    // std::cout << "\nOrdered Moveset->\n\n";
    // print_moveset(ordered_moveset);
    // std::cout << "\n";

    if (Maximizing)
    {
        float max_eval = std::numeric_limits<float>::lowest();
        for (auto m : moveset)
        {
            do_move(b, m);
            float eval = unified_minimax(b, cutoff - 1, alpha, beta, false);
            undo_last_move(b, m);
            max_eval = std::max(max_eval, eval);
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

        // if (is_sorted)
        // {
        std::reverse(ordered_moveset.begin(), ordered_moveset.end());
        // }

        float min_eval = std::numeric_limits<float>::max();
        for (auto m : moveset)
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
        // std::cout << all_boards_to_str(b) << "\n Legal moves:" << std::endl;
        // for (auto m : moveset)
        // {
        //     std::cout << move_to_str(m) << " ";
        // }
        // std::cout << std::endl;
        Board *b_copy = b.copy();

        auto start = std::chrono::high_resolution_clock::now();
        unified_minimax(b_copy, global_cutoff, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), b.data.player_to_play == WHITE);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // if (duration.count() < 2000)
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
