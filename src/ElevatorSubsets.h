#ifndef __ELEVATOR_SUBSETS_H__
#define __ELEVATOR_SUBSETS_H__

#include <array>
#include <cstddef>

struct RouterElevatorConfig {
    std::array<int, 3> subset;
    int ce_overall;
    int ce_nw;
    int ce_ne;
    int ce_sw;
    int ce_se;
};

constexpr std::size_t TOTAL_ROUTERS = 64;

// Direct O(1) compile-time array lookup indexed by Global Router ID
constexpr std::array<RouterElevatorConfig, TOTAL_ROUTERS> ROUTER_ELEVATOR_MAP = {{
    /* Router  0 */ RouterElevatorConfig{ {0, 4, 8}, 0, 0, 0, 0, 0 },
    /* Router  1 */ RouterElevatorConfig{ {0, 3, 4}, 0, 0, 3, 0, 0 },
    /* Router  2 */ RouterElevatorConfig{ {3, 0, 7}, 3, 0, 3, 3, 3 },
    /* Router  3 */ RouterElevatorConfig{ {3, 7, 11}, 3, 0, 3, 3, 3 },
    /* Router  4 */ RouterElevatorConfig{ {4, 0, 8}, 4, 4, 4, 4, 0 },
    /* Router  5 */ RouterElevatorConfig{ {4, 0, 7}, 4, 4, 7, 0, 3 },
    /* Router  6 */ RouterElevatorConfig{ {7, 3, 4}, 7, 4, 7, 0, 3 },
    /* Router  7 */ RouterElevatorConfig{ {7, 3, 11}, 7, 4, 7, 0, 3 },
    /* Router  8 */ RouterElevatorConfig{ {8, 4, 12}, 8, 8, 8, 8, 4 },
    /* Router  9 */ RouterElevatorConfig{ {8, 4, 11}, 8, 8, 11, 4, 7 },
    /* Router 10 */ RouterElevatorConfig{ {11, 7, 8}, 11, 8, 11, 4, 7 },
    /* Router 11 */ RouterElevatorConfig{ {11, 7, 15}, 11, 8, 11, 4, 7 },
    /* Router 12 */ RouterElevatorConfig{ {12, 8, 4}, 12, 12, 12, 12, 8 },
    /* Router 13 */ RouterElevatorConfig{ {12, 8, 15}, 12, 12, 15, 8, 11 },
    /* Router 14 */ RouterElevatorConfig{ {15, 11, 12}, 15, 12, 15, 8, 11 },
    /* Router 15 */ RouterElevatorConfig{ {15, 11, 7}, 15, 12, 15, 8, 11 },
    /* Router 16 */ RouterElevatorConfig{ {16, 20, 24}, 16, 16, 16, 16, 16 },
    /* Router 17 */ RouterElevatorConfig{ {16, 19, 20}, 16, 16, 19, 16, 16 },
    /* Router 18 */ RouterElevatorConfig{ {19, 16, 23}, 19, 16, 19, 19, 19 },
    /* Router 19 */ RouterElevatorConfig{ {19, 23, 27}, 19, 16, 19, 19, 19 },
    /* Router 20 */ RouterElevatorConfig{ {20, 16, 24}, 20, 20, 20, 20, 16 },
    /* Router 21 */ RouterElevatorConfig{ {20, 16, 23}, 20, 20, 23, 16, 19 },
    /* Router 22 */ RouterElevatorConfig{ {23, 19, 20}, 23, 20, 23, 16, 19 },
    /* Router 23 */ RouterElevatorConfig{ {23, 19, 27}, 23, 20, 23, 16, 19 },
    /* Router 24 */ RouterElevatorConfig{ {24, 20, 28}, 24, 24, 24, 24, 20 },
    /* Router 25 */ RouterElevatorConfig{ {24, 20, 27}, 24, 24, 27, 20, 23 },
    /* Router 26 */ RouterElevatorConfig{ {27, 23, 24}, 27, 24, 27, 20, 23 },
    /* Router 27 */ RouterElevatorConfig{ {27, 23, 31}, 27, 24, 27, 20, 23 },
    /* Router 28 */ RouterElevatorConfig{ {28, 24, 20}, 28, 28, 28, 28, 24 },
    /* Router 29 */ RouterElevatorConfig{ {28, 24, 31}, 28, 28, 31, 24, 27 },
    /* Router 30 */ RouterElevatorConfig{ {31, 27, 28}, 31, 28, 31, 24, 27 },
    /* Router 31 */ RouterElevatorConfig{ {31, 27, 23}, 31, 28, 31, 24, 27 },
    /* Router 32 */ RouterElevatorConfig{ {32, 36, 40}, 32, 32, 32, 32, 32 },
    /* Router 33 */ RouterElevatorConfig{ {32, 35, 36}, 32, 32, 35, 32, 32 },
    /* Router 34 */ RouterElevatorConfig{ {35, 32, 39}, 35, 32, 35, 35, 35 },
    /* Router 35 */ RouterElevatorConfig{ {35, 39, 43}, 35, 32, 35, 35, 35 },
    /* Router 36 */ RouterElevatorConfig{ {36, 32, 40}, 36, 36, 36, 36, 32 },
    /* Router 37 */ RouterElevatorConfig{ {36, 32, 39}, 36, 36, 39, 32, 35 },
    /* Router 38 */ RouterElevatorConfig{ {39, 35, 36}, 39, 36, 39, 32, 35 },
    /* Router 39 */ RouterElevatorConfig{ {39, 35, 43}, 39, 36, 39, 32, 35 },
    /* Router 40 */ RouterElevatorConfig{ {40, 36, 44}, 40, 40, 40, 40, 36 },
    /* Router 41 */ RouterElevatorConfig{ {40, 36, 43}, 40, 40, 43, 36, 39 },
    /* Router 42 */ RouterElevatorConfig{ {43, 39, 40}, 43, 40, 43, 36, 39 },
    /* Router 43 */ RouterElevatorConfig{ {43, 39, 47}, 43, 40, 43, 36, 39 },
    /* Router 44 */ RouterElevatorConfig{ {44, 40, 36}, 44, 44, 44, 44, 40 },
    /* Router 45 */ RouterElevatorConfig{ {44, 40, 47}, 44, 44, 47, 40, 43 },
    /* Router 46 */ RouterElevatorConfig{ {47, 43, 44}, 47, 44, 47, 40, 43 },
    /* Router 47 */ RouterElevatorConfig{ {47, 43, 39}, 47, 44, 47, 40, 43 },
    /* Router 48 */ RouterElevatorConfig{ {48, 52, 56}, 48, 48, 48, 48, 48 },
    /* Router 49 */ RouterElevatorConfig{ {48, 51, 52}, 48, 48, 51, 48, 48 },
    /* Router 50 */ RouterElevatorConfig{ {51, 48, 55}, 51, 48, 51, 51, 51 },
    /* Router 51 */ RouterElevatorConfig{ {51, 55, 59}, 51, 48, 51, 51, 51 },
    /* Router 52 */ RouterElevatorConfig{ {52, 48, 56}, 52, 52, 52, 52, 48 },
    /* Router 53 */ RouterElevatorConfig{ {52, 48, 55}, 52, 52, 55, 48, 51 },
    /* Router 54 */ RouterElevatorConfig{ {55, 51, 52}, 55, 52, 55, 48, 51 },
    /* Router 55 */ RouterElevatorConfig{ {55, 51, 59}, 55, 52, 55, 48, 51 },
    /* Router 56 */ RouterElevatorConfig{ {56, 52, 60}, 56, 56, 56, 56, 52 },
    /* Router 57 */ RouterElevatorConfig{ {56, 52, 59}, 56, 56, 59, 52, 55 },
    /* Router 58 */ RouterElevatorConfig{ {59, 55, 56}, 59, 56, 59, 52, 55 },
    /* Router 59 */ RouterElevatorConfig{ {59, 55, 63}, 59, 56, 59, 52, 55 },
    /* Router 60 */ RouterElevatorConfig{ {60, 56, 52}, 60, 60, 60, 60, 56 },
    /* Router 61 */ RouterElevatorConfig{ {60, 56, 63}, 60, 60, 63, 56, 59 },
    /* Router 62 */ RouterElevatorConfig{ {63, 59, 60}, 63, 60, 63, 56, 59 },
    /* Router 63 */ RouterElevatorConfig{ {63, 59, 55}, 63, 60, 63, 56, 59 }
}};

#endif // __ELEVATOR_SUBSETS_H__
