#pragma once
#include "define.h"

typedef struct FticGUID
{
    u8 bytes[16];
}FticGUID;

int guid_compare(FticGUID first, FticGUID second);
FticGUID guid_copy(const FticGUID* src);
FticGUID guid_copy_bytes(const u8 src[16]);
