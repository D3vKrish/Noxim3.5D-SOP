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
    /* Router  0 */ RouterElevatorConfig{ {0, 2, 12}, 0, 0, 0, 0, 0 },
    /* Router  1 */ RouterElevatorConfig{ {0, 2, 7}, 0, 0, 2, 0, 0 },
    /* Router  2 */ RouterElevatorConfig{ {2, 0, 7}, 2, 0, 2, 2, 2 },
    /* Router  3 */ RouterElevatorConfig{ {2, 7, 0}, 2, 2, 7, 2, 2 },
    /* Router  4 */ RouterElevatorConfig{ {0, 12, 2}, 0, 0, 12, 0, 0 },
    /* Router  5 */ RouterElevatorConfig{ {0, 2, 7}, 0, 12, 7, 0, 2 },
    /* Router  6 */ RouterElevatorConfig{ {2, 7, 0}, 2, 12, 7, 0, 2 },
    /* Router  7 */ RouterElevatorConfig{ {7, 2, 15}, 7, 12, 7, 2, 7 },
    /* Router  8 */ RouterElevatorConfig{ {12, 0, 2}, 12, 12, 12, 12, 0 },
    /* Router  9 */ RouterElevatorConfig{ {12, 0, 2}, 12, 12, 15, 0, 2 },
    /* Router 10 */ RouterElevatorConfig{ {2, 7, 15}, 2, 12, 15, 0, 2 },
    /* Router 11 */ RouterElevatorConfig{ {7, 15, 2}, 7, 12, 15, 2, 7 },
    /* Router 12 */ RouterElevatorConfig{ {12, 0, 15}, 12, 12, 12, 12, 0 },
    /* Router 13 */ RouterElevatorConfig{ {12, 15, 0}, 12, 12, 15, 0, 2 },
    /* Router 14 */ RouterElevatorConfig{ {15, 12, 2}, 15, 12, 15, 0, 2 },
    /* Router 15 */ RouterElevatorConfig{ {15, 7, 12}, 15, 12, 15, 2, 7 },
    /* Router 16 */ RouterElevatorConfig{ {18, 21, 28}, 18, 18, 18, 18, 18 },
    /* Router 17 */ RouterElevatorConfig{ {18, 21, 26}, 18, 28, 18, 18, 18 },
    /* Router 18 */ RouterElevatorConfig{ {18, 21, 26}, 18, 21, 18, 18, 18 },
    /* Router 19 */ RouterElevatorConfig{ {18, 21, 26}, 18, 18, 18, 18, 18 },
    /* Router 20 */ RouterElevatorConfig{ {21, 28, 18}, 21, 21, 21, 21, 18 },
    /* Router 21 */ RouterElevatorConfig{ {21, 18, 26}, 21, 28, 21, 21, 18 },
    /* Router 22 */ RouterElevatorConfig{ {18, 21, 26}, 18, 21, 26, 18, 18 },
    /* Router 23 */ RouterElevatorConfig{ {18, 21, 26}, 18, 21, 18, 18, 18 },
    /* Router 24 */ RouterElevatorConfig{ {28, 21, 26}, 28, 28, 28, 28, 21 },
    /* Router 25 */ RouterElevatorConfig{ {21, 26, 29}, 21, 28, 26, 21, 21 },
    /* Router 26 */ RouterElevatorConfig{ {26, 18, 21}, 26, 29, 26, 21, 18 },
    /* Router 27 */ RouterElevatorConfig{ {26, 18, 21}, 26, 26, 26, 18, 26 },
    /* Router 28 */ RouterElevatorConfig{ {28, 29, 21}, 28, 28, 28, 28, 21 },
    /* Router 29 */ RouterElevatorConfig{ {29, 28, 21}, 29, 28, 29, 29, 21 },
    /* Router 30 */ RouterElevatorConfig{ {26, 29, 28}, 26, 29, 26, 21, 26 },
    /* Router 31 */ RouterElevatorConfig{ {26, 29, 28}, 26, 29, 26, 26, 26 },
    /* Router 32 */ RouterElevatorConfig{ {32, 37, 39}, 32, 32, 32, 32, 32 },
    /* Router 33 */ RouterElevatorConfig{ {32, 37, 39}, 32, 32, 37, 32, 32 },
    /* Router 34 */ RouterElevatorConfig{ {32, 37, 39}, 32, 32, 39, 32, 32 },
    /* Router 35 */ RouterElevatorConfig{ {39, 32, 37}, 39, 32, 39, 39, 39 },
    /* Router 36 */ RouterElevatorConfig{ {32, 37, 39}, 32, 32, 37, 32, 32 },
    /* Router 37 */ RouterElevatorConfig{ {37, 32, 39}, 37, 37, 37, 32, 37 },
    /* Router 38 */ RouterElevatorConfig{ {37, 39, 42}, 37, 37, 39, 32, 37 },
    /* Router 39 */ RouterElevatorConfig{ {39, 37, 42}, 39, 37, 39, 32, 39 },
    /* Router 40 */ RouterElevatorConfig{ {32, 37, 42}, 32, 32, 42, 32, 32 },
    /* Router 41 */ RouterElevatorConfig{ {37, 42, 32}, 37, 37, 42, 32, 37 },
    /* Router 42 */ RouterElevatorConfig{ {42, 37, 39}, 42, 42, 42, 37, 39 },
    /* Router 43 */ RouterElevatorConfig{ {39, 42, 47}, 39, 42, 47, 37, 39 },
    /* Router 44 */ RouterElevatorConfig{ {32, 37, 42}, 32, 32, 47, 32, 32 },
    /* Router 45 */ RouterElevatorConfig{ {37, 42, 47}, 37, 37, 47, 32, 37 },
    /* Router 46 */ RouterElevatorConfig{ {42, 47, 37}, 42, 42, 47, 37, 42 },
    /* Router 47 */ RouterElevatorConfig{ {47, 39, 42}, 47, 47, 47, 42, 39 },
    /* Router 48 */ RouterElevatorConfig{ {48, 53, 55}, 48, 48, 48, 48, 48 },
    /* Router 49 */ RouterElevatorConfig{ {48, 53, 55}, 48, 48, 53, 48, 48 },
    /* Router 50 */ RouterElevatorConfig{ {48, 53, 55}, 48, 48, 55, 48, 48 },
    /* Router 51 */ RouterElevatorConfig{ {55, 48, 53}, 55, 48, 55, 55, 55 },
    /* Router 52 */ RouterElevatorConfig{ {48, 53, 55}, 48, 48, 53, 48, 48 },
    /* Router 53 */ RouterElevatorConfig{ {53, 48, 55}, 53, 53, 53, 48, 53 },
    /* Router 54 */ RouterElevatorConfig{ {53, 55, 58}, 53, 53, 55, 48, 53 },
    /* Router 55 */ RouterElevatorConfig{ {55, 53, 58}, 55, 53, 55, 48, 55 },
    /* Router 56 */ RouterElevatorConfig{ {48, 53, 58}, 48, 48, 58, 48, 48 },
    /* Router 57 */ RouterElevatorConfig{ {53, 58, 48}, 53, 53, 58, 48, 53 },
    /* Router 58 */ RouterElevatorConfig{ {58, 53, 55}, 58, 58, 58, 53, 55 },
    /* Router 59 */ RouterElevatorConfig{ {55, 58, 63}, 55, 58, 63, 53, 55 },
    /* Router 60 */ RouterElevatorConfig{ {48, 53, 58}, 48, 48, 63, 48, 48 },
    /* Router 61 */ RouterElevatorConfig{ {53, 58, 63}, 53, 53, 63, 48, 53 },
    /* Router 62 */ RouterElevatorConfig{ {58, 63, 53}, 58, 58, 63, 53, 58 },
    /* Router 63 */ RouterElevatorConfig{ {63, 55, 58}, 63, 63, 63, 58, 55 }
}};

#endif // __ELEVATOR_SUBSETS_H__
