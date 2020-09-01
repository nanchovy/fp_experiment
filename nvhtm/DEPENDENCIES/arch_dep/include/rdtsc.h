#ifndef RDTSC_H_GUARD
#define RDTSC_H_GUARD

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void) {
    register unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

#define rdtscp(void) ({ \
    register unsigned long long res; \
    __asm__ __volatile__ ( \
		"xor %%rax,%%rax \n\t" \
		"rdtscp          \n\t" \
		"shl $32,%%rdx   \n\t" \
		"or  %%rax,%%rdx \n\t" \
		"mov %%rdx,%0" \
		: "=r"(res) \
		: \
		: "rax", "rdx"); \
    res; \
})


#elif defined(__powerpc__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int result = 0;
    unsigned long int upper, lower, tmp;
    __asm__ volatile(
            "0:                  \n"
            "\tmftbu   %0           \n"
            "\tmftb    %1           \n"
            "\tmftbu   %2           \n"
            "\tcmpw    %2,%0        \n"
            "\tbne     0b         \n"
            : "=r"(upper), "=r"(lower), "=r"(tmp)
            );
    result = upper;
    result = result << 32;
    result = result | lower;

    return (result);
}

#else

#error "No tick counter is available!"

#endif

/*  $RCSfile:  $   $Author: kazutomo $
 *  $Revision: 1.6 $  $Date: 2005/04/13 18:49:58 $
 */

#ifdef __cplusplus
}
#endif

#endif /* RDTSC_H_GUARD */

