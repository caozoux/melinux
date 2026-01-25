#include <stdio.h>
#include <stdlib.h>

static int helper(int x) {
    return x * 2;
}

void print_message(const char* msg) {
    printf("%s\n", msg);
}

int calculate(int a, int b) {
    int result = 0;

    result = helper(a) + helper(b);

    if (result > 100) {
        print_message("Result is large");
    }

    return result;
}

int main(int argc, char* argv[]) {
    int x = 10;
    int y = 20;

    print_message("Starting program");

    int sum = calculate(x, y);

    printf("Sum: %d\n", sum);

    return 0;
}
