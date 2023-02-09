#pragma once

void setup();
void finishup();

#if defined(TEST_CORRECTNESS)
void test_correctness(double *);
#endif
