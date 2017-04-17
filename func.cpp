//
// Created by yriy on 11.04.17.
//

#include "func.h"

char* substr(char* ptr, int n, int i, int j)
{
    char* res = new char[j - i + 1];
    if(i > n || j > n)
        return new char[1];
    int idx = 0;
    for(char* c = ptr + i; c != ptr + j; c++, idx++)
    {
        res[idx] = *c;
    }
    return res;
}

