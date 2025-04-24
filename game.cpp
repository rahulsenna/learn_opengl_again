#include <string>
#include <iostream>


#ifdef __cplusplus
extern "C" {
#endif


void clear_color(float *r, float *g, float *b, float *a)
{
    *r = .2;
    *g = .3;
    *b = .3;
    *a = 1.;
}



#ifdef __cplusplus
}
#endif