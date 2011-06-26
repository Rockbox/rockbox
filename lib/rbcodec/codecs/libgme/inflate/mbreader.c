
/* Memory buffer reader, simulates file read 
   @ gama 
*/

#include "mbreader.h"

int mbread(struct mbreader_t *md, void *buf, size_t n)
{
	if (!md) return -1;
	size_t read_bytes = (md->offset+n) > md->size ? 
		md->size-md->offset : n;
	memcpy(buf,md->ptr + md->offset,read_bytes);
	md->offset += read_bytes;
	return read_bytes;
}
