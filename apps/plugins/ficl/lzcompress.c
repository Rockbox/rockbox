#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ficl.h"

#define NETWORK_ORDER(X) ((((unsigned char*)X[0]) << 8) | (((unsigned char *)X[1])))


static int ficlLzCompareWindow(const unsigned char *window, const unsigned char *buffer,
	int *offset, unsigned char *next, int windowSize, int bufferSize)
	{
	const unsigned char *windowEnd;
	const unsigned char *bufferEnd;
	int longest;
	unsigned char bufferFirst;
	const unsigned char *windowTrace;

	longest = 0;
	bufferFirst = buffer[0];
	*next = bufferFirst;

	/*
	** we can't match more than bufferSize-1 characters...
	** we need to reserve the last character for the "next",
	** and this also prevents us from returning FICL_LZ_BUFFER_LENGTH
	** as the length (which won't work, max we can store is FICL_LZ_BUFFER_LENGTH - 1)
	*/
	bufferSize--;

	windowEnd = window + windowSize;
	bufferEnd = buffer + bufferSize;

	for (windowTrace = window; windowTrace < windowEnd; windowTrace++)
		{
		const unsigned char *bufferTrace;
		const unsigned char *windowTrace2;
		int length;

		if (*windowTrace != bufferFirst)
			continue;

		bufferTrace = buffer;
		for (windowTrace2 = windowTrace;
			(windowTrace2 < windowEnd) && (bufferTrace < bufferEnd)
				&& (*windowTrace2 == *bufferTrace);
			windowTrace2++, bufferTrace++)
			{
			}

		length = windowTrace2 - windowTrace;
		if ((length > longest) && (length >= FICL_LZ_MINIMUM_USEFUL_MATCH))
			{
			*offset = windowTrace - window;
			longest = length;
			*next = *bufferTrace;
			}
		}

	return longest;
	}



void ficlLzEncodeHeaderField(unsigned char *data, unsigned int input, int *byteOffset)
	{
	int i;

	if (input <= 252)
		data[(*byteOffset)++] = (unsigned char)input;
	else
		{
		unsigned char id;
		int length;
		int inputPosition;
		int bitsOffset;

		if (input <= 65536)
			{
			id = 253;
			length = 2;
			}
		else
			{
			id = 254;
			length = 4;
			}

		input = ficlNetworkUnsigned32(input);
		inputPosition = (sizeof(unsigned long) * 8) - (length * 8);
		/* bitsOffset; */

		data[(*byteOffset)++] = (unsigned char)id;
		bitsOffset = *byteOffset * 8;
		(*byteOffset) += length;

		for (i = 0; i < (length * 8); i++)
			ficlBitSet(data, bitsOffset++, ficlBitGet((unsigned char *)&input, inputPosition++));
		}
	}



int ficlLzCompress(const unsigned char *uncompressed, size_t uncompressedSize, unsigned char **compressed_p, size_t *compressedSize_p)
	{
	unsigned char *compressed;
	const unsigned char *window;
	const unsigned char *buffer;
	int outputPosition;
	int remaining;
	int windowSize;
	int headerLength;
	unsigned char headerBuffer[10];
	int compressedSize;
	int totalSize;

	*compressed_p = NULL;

	compressed = (unsigned char *)calloc(((uncompressedSize * 5) / 4) + 10, 1);
	if (compressed == NULL)
		return -1;

	window = buffer = uncompressed;

	outputPosition = 0;
	remaining = uncompressedSize;
	windowSize = 0;

	while (remaining > 0)
		{
		int bufferSize = FICL_MIN(remaining, FICL_LZ_BUFFER_SIZE);
		int useWindowSize = FICL_MIN(remaining, windowSize);
		int offset = 0;
		int i;

		unsigned long token;
		int tokenLength;
		unsigned char next;

		int length = ficlLzCompareWindow(window, buffer, &offset, &next, useWindowSize, bufferSize);
		if (length > 1)
			{
			/* phrase token */
			assert((length - FICL_LZ_MINIMUM_USEFUL_MATCH) < (1 << FICL_LZ_LENGTH_BITS));
			token = (1 << (FICL_LZ_PHRASE_BITS - 1))
				| (offset << (FICL_LZ_PHRASE_BITS - FICL_LZ_TYPE_BITS - FICL_LZ_OFFSET_BITS))
				| ((length - FICL_LZ_MINIMUM_USEFUL_MATCH) << (FICL_LZ_PHRASE_BITS - FICL_LZ_TYPE_BITS - FICL_LZ_OFFSET_BITS - FICL_LZ_LENGTH_BITS))
				| next;

			tokenLength = FICL_LZ_PHRASE_BITS;
			}
		else
			{
			token = next;
			tokenLength = FICL_LZ_SYMBOL_BITS;
			}

		token = ficlNetworkUnsigned32(token);
		for (i = 0; i < tokenLength; i++)
			{
			int inputPosition = (sizeof(unsigned long) * 8) - tokenLength + i;
			ficlBitSet(compressed, outputPosition, ficlBitGet((unsigned char *)&token, inputPosition));
			outputPosition++;
			}

		length++;

		buffer += length;
		if (windowSize == FICL_LZ_WINDOW_SIZE)
			window += length;
		else
			{
			if ((windowSize + length) < FICL_LZ_WINDOW_SIZE)
				windowSize += length;
			else
				{
				window += (windowSize + length) - FICL_LZ_WINDOW_SIZE;
				windowSize = FICL_LZ_WINDOW_SIZE;
				}
			}

		remaining -= length;
		}

	headerLength = 0;
	memset(&headerBuffer, 0, sizeof(headerBuffer));
	ficlLzEncodeHeaderField(headerBuffer, outputPosition, &headerLength);
	ficlLzEncodeHeaderField(headerBuffer, uncompressedSize, &headerLength);

	/* plug in header */
	compressedSize = (((outputPosition - 1) / 8) + 1);
	totalSize = compressedSize + headerLength;
	compressed = (unsigned char *)realloc(compressed, totalSize);
	memmove(compressed + headerLength, compressed, compressedSize);
	memcpy(compressed, headerBuffer, headerLength);

	*compressed_p = compressed;
	*compressedSize_p = totalSize;

	return 0;
	}

