#ifndef COP_ATTRIBUES_H
#define COP_ATTRIBUES_H

#if defined(__clang__) || defined(__GNUC__)
#define COP_ATTR_UNUSED __attribute__((unused))
#define COP_ATTR_INLINE __inline__ __attribute__((always_inline))
/* #elif other compilers... */
#else
#define COP_ATTR_UNUSED
#define COP_ATTR_INLINE
#endif

#endif /* COP_ATTRIBUES_H */
