#if ! defined _UTIL_H
#define _UTIL_H 1

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string>

class util {
    struct MatchPathSeparator
    {
        bool operator()(char ch) const
        {
            return ch == '/';
        }
    };
    
public:
    static std::string basename(std::string const& pathname);
    static int	docgetchar(FILE *fdocp, int *docbuffer, int *docbuffercount, int minstring);
    static bool issignificantbyte(int c);
    static int percentmatching(int firstL, int firstR, int lastL, int lastR, int perfectmatchingwords);
    static bool equals(char *a, char *b);
    static void heapsort(unsigned long *tableA, int *tableB, int n);
    static void heapsort(int **table, int n, int primary, int secondary);
    static void heapsort(int **table, int n, int primary);
    static void heapsort(int **table, int n);
    static std::string removeExtension(std::string const& filename);
    static void quitprogram(int value);
};	

#endif
