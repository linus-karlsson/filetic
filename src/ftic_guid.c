#include "ftic_guid.h"
#include <string.h>

int guid_compare(FticGUID first, FticGUID second)
{
    return memcmp(first.bytes, second.bytes, sizeof(first.bytes));
}
 
FticGUID guid_copy(const FticGUID* src)
{
    FticGUID result = { 0 };
    memcpy(result.bytes, src->bytes, sizeof(result.bytes));
    return result;
}

FticGUID guid_copy_bytes(const u8 src[16])
{
    FticGUID result = { 0 };
    memcpy(result.bytes, src, sizeof(result.bytes));
    return result;
}

