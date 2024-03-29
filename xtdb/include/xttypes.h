#ifndef XTTYPES_H
#define XTTYPES_H
namespace xtdb {
#define R_NORMAL 1
#define R_ABNORMAL 0
#define XT_INTID_ALLOC_BIT 0x80000000
#define XT_INTID_OFFSET_MASK (~XT_INTID_ALLOC_BIT)

typedef unsigned int uint;

enum class xtobject_kind{
    RECTANGLE,
    LINE,
    BLOCK,
    INST,
    NET,
    PORT,
    PORTINST,
    WIRESEG,
    VIA
};
}

#endif // XTTYPES_H
