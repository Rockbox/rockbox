#include "ficl.h"

int ficlBitGet(const unsigned char *bits, size_t index)
	{
	int byteIndex = index >> 3;
	int bitIndex = index & 7;
	unsigned char mask = (unsigned char)(128 >> bitIndex);

	return ((mask & bits[byteIndex]) ? 1 : 0);
	}



void ficlBitSet(unsigned char *bits, size_t index, int value)
	{
	int byteIndex = index >> 3;
	int bitIndex = index & 7;
	unsigned char mask = (unsigned char)(128 >> bitIndex);

	if (value)
		bits[byteIndex] |= mask;
	else
		bits[byteIndex] &= ~mask;
	}


void ficlBitGetString(unsigned char *destination, const unsigned char *source, int offset, int count, int destAlignment)
	{
	int bit = destAlignment - count;
	while (count--)
		ficlBitSet(destination, bit++, ficlBitGet(source, offset++));
	}


/*
** This will actually work correctly *regardless* of the local architecture.
** --lch
**/
ficlUnsigned16 ficlNetworkUnsigned16(ficlUnsigned16 number)
{
	ficlUnsigned8 *pointer = (ficlUnsigned8 *)&number;
	return (ficlUnsigned16)(((ficlUnsigned16)(pointer[0] << 8)) | (pointer[1]));
}

ficlUnsigned32 ficlNetworkUnsigned32(ficlUnsigned32 number)
{
	ficlUnsigned16 *pointer = (ficlUnsigned16 *)&number;
	return ((ficlUnsigned32)(ficlNetworkUnsigned16(pointer[0]) << 16)) | ficlNetworkUnsigned16(pointer[1]);
}
