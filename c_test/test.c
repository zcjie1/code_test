#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#define MAX_LINE 1024

int main(int argc, char *argv[]) {
    int opt;
    char *filename = NULL;
    int verbose = 0;
    int number = 0;

    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"file", required_argument, NULL, 'f'},
        {"number", required_argument, NULL, 'n'},
        {NULL, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "hvfn:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s [-h] [-v] [-f FILE] [-n NUMBER]\n", argv[0]);
                printf("Options:\n");
                printf("  -h, --help       Show this help message and exit.\n");
                printf("  -v, --verbose    Enable verbose output.\n");
                printf("  -f, --file=FILE  File to process.\n");
                printf("  -n, --number=NUM Number to use.\n");
                return 0;
            case 'v':
                verbose = 1;
                break;
            case 'f':
                filename = optarg;
                break;
            case 'n':
                number = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Unknown option: %c\n", opt);
                return 1;
        }
    }

    // 处理剩余的参数
    for (int i = optind; i < argc; i++) {
        printf("Non-option argument: %s\n", argv[i]);
    }

    // 打印解析结果
    printf("Verbose mode: %s\n", verbose ? "Enabled" : "Disabled");
    if (filename) {
        printf("File: %s\n", filename);
    }
    if (number) {
        printf("Number: %d\n", number);
    }

    return 0;
}