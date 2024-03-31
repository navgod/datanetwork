#include <stdio.h>
#include <string.h>

int main() {
    char str[] = "This\nis\na\ntest\nstring.";
    char *token;
    char *rest = str;

    while ((token = strtok_r(rest, "\n", &rest))) {
        printf("%s\n", token);
    }

    return 0;
}
