#include <stdlib.h>

void *alloca(size_t size)
{
	return calloc(1, size);
}
