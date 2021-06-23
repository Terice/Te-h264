#include <stdio.h>
#include <iostream>

#include "gvars.h"
#include "terror.h"

#include "parser.h"
#include "decoder.h"
#include "nal.h"

#define FILEPATH argv[1]

int main(int argc, char* argv[])
{
    if(argc  < 2) terr.error("[main] input err");
    FILE *fp = fopen(FILEPATH, "r");
    parser pa(fp);
    decoder de;

    while (pa.find_nextNAL())
    {
        nal n(&pa,&de);
        n.decode();
    }
    return 0;
}
