#include <cstdlib>
#include <string>
#define ASSERT(condition, msg) assert(((void)msg, condition))
inline void
error(const char* msg)
{
    printf("%s", msg);
    exit(EXIT_FAILURE);
}