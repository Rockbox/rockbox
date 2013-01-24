#include <stdlib.h>
#include <string.h>

#include "ficl.h"



int ficlLzDecodeHeaderField(const unsigned char *data, int *byteOffset)
	{
	unsigned char id;
	int networkOrder;
	int length;

	id = data[(*byteOffset)++];
	if (id < 252)
		return id;

	networkOrder = 0;
	length = (id == 253) ? 2: 4;

	ficlBitGetString(((unsigned char *)&networkOrder), data,
		(*byteOffset) * 8,
		length * 8, sizeof(networkOrder) * 8);
	(*byteOffset) += length;

	return ficlNetworkUnsigned32(networkOrder);
	}



int ficlLzUncompress(const unsigned char *compressed, unsigned char **uncompressed_p, size_t *uncompressedSize_p)
	{
	unsigned char *window;
	unsigned char *buffer;
	unsigned char *uncompressed;
	unsigned char *initialWindow;

	int bitstreamLength;
	int inputPosition;
	int uncompressedSize;

	*uncompressed_p = NULL;

	inputPosition = 0;
	bitstreamLength = ficlLzDecodeHeaderField(compressed, &inputPosition);
	uncompressedSize = ficlLzDecodeHeaderField(compressed, &inputPosition);

	inputPosition <<= 3; /* same as * 8 */

	bitstreamLength += inputPosition;

	uncompressed = (unsigned char *)calloc(uncompressedSize + 1, 1);
	if (uncompressed == NULL)
		return -1;
	window = buffer = uncompressed;
	initialWindow = buffer + FICL_LZ_WINDOW_SIZE;

	while (inputPosition != bitstreamLength)
		{
		int length;
		int token = ficlBitGet(compressed, inputPosition);
		inputPosition++;

		if (token)
			{
			/* phrase token */
			int offset = 0;
			ficlBitGetString((unsigned char *)&offset, compressed, inputPosition, FICL_LZ_PHRASE_BITS - (1 + FICL_LZ_NEXT_BITS), sizeof(offset) * 8);
			offset = ficlNetworkUnsigned32(offset);
			inputPosition += FICL_LZ_PHRASE_BITS - (1 + FICL_LZ_NEXT_BITS);

			length = (offset & ((1 << FICL_LZ_LENGTH_BITS) - 1)) + FICL_LZ_MINIMUM_USEFUL_MATCH;
			offset >>= FICL_LZ_LENGTH_BITS;

			memmove(buffer, window + offset, length);
			buffer += length;
			length++;
			}
		else
			length = 1;

		/* symbol token */
		*buffer = 0;
		ficlBitGetString(buffer++, compressed, inputPosition, FICL_LZ_NEXT_BITS, sizeof(*buffer) * 8);
		inputPosition += FICL_LZ_NEXT_BITS;
		if (buffer > initialWindow)
			window = buffer - FICL_LZ_WINDOW_SIZE;
		}

	*uncompressed_p = uncompressed;
	*uncompressedSize_p = uncompressedSize;

	return 0;
	}
