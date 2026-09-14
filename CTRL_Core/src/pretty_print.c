#include <stdio.h>

#include "msg_parser.h"

void Pattern(char* a, char* b, char c, char d, char e, char f, char g, char h, char i, char j, char k, char l, char* m, char* n){
    printf( "          |       |             |   |             |       |         \n"
            "          |   |   |             |===|             |   |   |         \n"
            "          |       |             |   |             |       |         \n"
            "      %s  |   |   |             |===|          %s |   |   |         \n"
            "__________| %c     |_____________|   |_____________| %c     |_________\n"
            "                                |===|                               \n"
            "          %c                                       %c                \n"
            "-  -  -            -  -  -  -   %c   %c -  -  -  -            -  -  - \n"
            "                   %c                                      %c       \n"
            "__________         _____________|===|_____________         _________\n"
            "          |   |  %c|             |   |             |   |  %c|         \n"
            "          |       |  %s         |===|             |       |  %s     \n"
            "          |   |   |             |   |             |   |   |         \n"
            "          |       |             |===|             |       |         \n"
            "          |   |   |             |   |             |   |   |         \n"
            "\n"
            "\n"
            "\n"
            "                                                               E    \n"
            "                                                               |    \n"
            "                                                           N---+---S\n"
            "                                                               |    \n"
            "                                                               W    \n\n\n", a, b, c, d, e, f, g, h, i, j, k, l, m, n);

}

void PrettyPrinter(int State, int Block) {

//	printf("*****This is the State: %d\n", State);
//	printf("*****This is the Block: %d\n", Block);

    char Red = 'R';
    char *Red_str = "R ";
    char Green = 'G';
    char Yellow = 'Y';
    char block = 'B';
    char* arrowleft = "<>";
    char* arrowup = "v^";

    switch (State)
    {
        case EWG_NSR:
            if(Block)
                Pattern(arrowup, arrowup, Green, Green, Red, Red, block, block, Red, Red, Green, Green, arrowup, arrowup);
            else
                Pattern(arrowup, arrowup, Green, Green, Red, Red, ' ', ' ', Red, Red, Green, Green, arrowup, arrowup);
            break;
        case EWY_NSR:
            if(Block)
                Pattern(arrowup, arrowup, Yellow, Yellow, Red, Red, block, block, Red, Red, Yellow, Yellow, arrowup, arrowup);
            else
                Pattern(arrowup, arrowup, Yellow, Yellow, Red, Red, ' ', ' ', Red, Red, Yellow, Yellow, arrowup, arrowup);
            break;
        case EWR_NSR_1:
            if(Block)
                Pattern(Red_str, Red_str, Red, Red, Red, Red, block, block, Red, Red, Red, Red, Red_str, Red_str);
            else
                Pattern(Red_str, Red_str, Red, Red, Red, Red, ' ', ' ', Red, Red, Red, Red, Red_str, Red_str);
            break;
        case EWR_NSR_2:
            if(Block)
                Pattern(Red_str, Red_str, Red, Red, Red, Red, block, block, Red, Red, Red, Red, Red_str, Red_str);
            else
                Pattern(Red_str, Red_str, Red, Red, Red, Red, ' ', ' ', Red, Red, Red, Red, Red_str, Red_str);
            break;
        case EWR_NSG:
            if (Block)
                Pattern(arrowleft, arrowleft, Red, Red, Green, Green, block, block, Green, Green, Red, Red, arrowleft, arrowleft);
            else
                Pattern(arrowleft, arrowleft, Red, Red, Green, Green, ' ', ' ', Green, Green, Red, Red, arrowleft, arrowleft);
            break;
        default:
            if (Block)
                Pattern(arrowleft, arrowleft, Red, Red, Yellow, Yellow, block, block, Yellow, Yellow, Red, Red, arrowleft, arrowleft);
            else
                Pattern(arrowleft, arrowleft, Red, Red, Yellow, Yellow, ' ', ' ', Yellow, Yellow, Red, Red, arrowleft, arrowleft);
            break;
    }
}
