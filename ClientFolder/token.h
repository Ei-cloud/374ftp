#define MAX_NUM_TOKENS 100
#define tokenSeparators " \t\n"
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<signal.h>
#include<stdlib.h>

char *token[MAX_NUM_TOKENS];

int tokenise (char *inputLine, char *token[]);
