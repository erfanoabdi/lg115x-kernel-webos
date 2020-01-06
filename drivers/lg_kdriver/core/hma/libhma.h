#ifndef _LIBHMA_H
#define _LIBHMA_H

unsigned long libhma_alloc (const char *name,
		int size, int align, void **vaddr);
unsigned long libhma_alloc_cached (const char *name,
		int size, int align, void **vaddr);
unsigned int libhma_get_ump_secure_id (unsigned long paddr);
void libhma_free (unsigned long paddr);

enum libhma_cache_control
{
	libhma_cache_invalidate,	/* after  dma from device */
	libhma_cache_clean,		/* before dma to device */
};

/*
 * DANGER !!
 *
 * Unexpected memory values can be written at boundary of cache
 * line when vaddr and size is not aligned with cache line.
 *
 * Use at your own risk.
 */
int libhma_cache_ctl (unsigned long paddr, void *vaddr,
		size_t size, enum libhma_cache_control ctl);

int libhma_cache_control (unsigned long paddr,
		enum libhma_cache_control control);

/* deprecated.
 * hma will not support for memory operations such as memcpy,
 * memset, memmove, ...
 * will be removed soon. */
int libhma_memcpy_tocpu (void *vaddr_des, void *vaddr_src,
		void *paddr_src, unsigned int size)
		__attribute__((deprecated));

#endif
