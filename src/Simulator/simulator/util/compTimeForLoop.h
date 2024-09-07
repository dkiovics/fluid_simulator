#pragma once

#define COMP_FOR_LOOP_1(iteratorVarName, code) \
{ constexpr int iteratorVarName = 0; code }

#define COMP_FOR_LOOP_2(iteratorVarName, code) COMP_FOR_LOOP_1(iteratorVarName, code) \
{ constexpr int iteratorVarName = 1; code }

#define COMP_FOR_LOOP_3(iteratorVarName, code) COMP_FOR_LOOP_2(iteratorVarName, code) \
{ constexpr int iteratorVarName = 2; code }

#define COMP_FOR_LOOP_4(iteratorVarName, code) COMP_FOR_LOOP_3(iteratorVarName, code) \
{ constexpr int iteratorVarName = 3; code }

#define COMP_FOR_LOOP_5(iteratorVarName, code) COMP_FOR_LOOP_4(iteratorVarName, code) \
{ constexpr int iteratorVarName = 4; code }

#define COMP_FOR_LOOP_6(iteratorVarName, code) COMP_FOR_LOOP_5(iteratorVarName, code) \
{ constexpr int iteratorVarName = 5; code }

#define COMP_FOR_LOOP_7(iteratorVarName, code) COMP_FOR_LOOP_6(iteratorVarName, code) \
{ constexpr int iteratorVarName = 6; code }

#define COMP_FOR_LOOP_8(iteratorVarName, code) COMP_FOR_LOOP_7(iteratorVarName, code) \
{ constexpr int iteratorVarName = 7; code }

#define COMP_FOR_LOOP_9(iteratorVarName, code) COMP_FOR_LOOP_8(iteratorVarName, code) \
{ constexpr int iteratorVarName = 8; code }

#define COMP_FOR_LOOP_10(iteratorVarName, code) COMP_FOR_LOOP_9(iteratorVarName, code) \
{ constexpr int iteratorVarName = 9; code }

#define COMP_FOR_LOOP_11(iteratorVarName, code) COMP_FOR_LOOP_10(iteratorVarName, code) \
{ constexpr int iteratorVarName = 10; code }

#define COMP_FOR_LOOP_12(iteratorVarName, code) COMP_FOR_LOOP_11(iteratorVarName, code) \
{ constexpr int iteratorVarName = 11; code }

#define COMP_FOR_LOOP_13(iteratorVarName, code) COMP_FOR_LOOP_12(iteratorVarName, code) \
{ constexpr int iteratorVarName = 12; code }

#define COMP_FOR_LOOP(iteratorVarName, numIterations, code) COMP_FOR_LOOP_##numIterations(iteratorVarName, code)
