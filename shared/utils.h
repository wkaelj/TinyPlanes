
/*
 * Progrma wide utils
 */

#include <sys/time.h>
#include <stdlib.h>

#define SEC_TO_MICROSEC 1000000

static inline time_t get_time(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    // they must be combined to prevent huge lag at microsecond wrap
    // every second, which would result in the calculation of time
    // difference in the get_delta function to be a negative number.
    // when said negative was subtracted from frametime it would a
    // very large number and cause a long wait
    return t.tv_usec + t.tv_sec * SEC_TO_MICROSEC;
}
