#include "check.h"

#include <stdio.h>

int check_failures = 0;
int check_passes = 0;

void test_keymap(void);
void test_config(void);
void test_repeat(void);

int main(void)
{
    test_keymap();
    test_config();
    test_repeat();
    printf("tacet-cec tests: %d passed, %d failed\n", check_passes, check_failures);
    return check_failures ? 1 : 0;
}
