#include <stdio.h>
#include <stdbool.h>

struct Settings {
    _Bool offline_disabled : 1;
    _Bool debug_mode: 1;
    //unsigned long volume: 4;
    //bool volume : 4; // 4 bits for volume level (0-15)
};

int main() {
    struct Settings s = { 
        .offline_disabled = true, 
        .debug_mode = false,
        //.volume = 4
    };

    printf("size: %ld\n", sizeof(struct Settings));
    printf("offline_disabled: %d\n", s.offline_disabled);
    printf("debug_mode: %d\n", s.debug_mode);
    //printf("volume: %d\n", s.volume);
    
    return 0;
}