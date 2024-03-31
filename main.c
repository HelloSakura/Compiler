#include<stdio.h>
#include "compiler.h"

int main()
{
     int res = compile_file("./test.c", "./test", 0);
     if(res == COMPILER_FILE_COMPILED_OK){
        printf("Compile done!\n");
     }
     else if(res == COMPILER_FAILED_WITH_ERRORS){
        printf("Compile failed!\n");
     }
     else{
        printf("Unknown reason.\n");
     }
    return 0;
}
