#ifndef ERROR_H_
#define ERROR_H_

#define UNIMPLEMENTED \
    do { \
        fprintf(stderr, "%s:%d: %s is not implemented yet\n", \
                __FILE__, __LINE__, __func__); \
        abort(); \
    } while(0)

#define UNREACHABLE \
    do { \
        fprintf(stderr, "%s:%d: unreachable\n",  __FILE__, __LINE__); \
        abort(); \
    } while(0)

#endif // ERROR_H_
