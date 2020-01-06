
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include "hma_drv.h"
#include "libhma.h"


#if 0
#define debug(fmt,args...)	printf("%s.%d: "fmt,__func__,__LINE__,##args)
#else
#define debug(fmt,args...)	do{}while(0)
#endif
#define info(fmt,args...)	printf("%s.%d: "fmt,__func__,__LINE__,##args)
#define error(fmt,args...)	printf("%s.%d: "fmt,__func__,__LINE__,##args)

struct mem_desc
{
	int fd;
	unsigned long paddr;
	void *vaddr;
	int size;
	int type;
#define NONCACHED	0
#define CACHED		1

	unsigned int ump_secure_id;

	struct mem_desc *next;
};

static struct mem_desc desc_root;
static pthread_mutex_t root_lock = PTHREAD_MUTEX_INITIALIZER;

static unsigned long alloc (const char *name, int size, int align, void **vaddr, int cache)
{
	struct hma_ioctl_alloc alloc;
	int fd, ret;
	struct mem_desc *desc;
	void *_vaddr = MAP_FAILED;

	/* open and ioctl */
	fd = open ("/dev/hma", O_RDWR | O_CLOEXEC);
	if (fd < 0)
	{
		error ("hma open failed. %s\n", strerror (errno));
		return 0;
	}

	/* allocate */
	if (name)
	{
		if (strlen (name) >= sizeof (alloc.pool))
		{
			error ("too long pool name. %s\n", name);
			close (fd);
			return 0;
		}

		strcpy (alloc.pool, name);
	}
	else
		alloc.pool[0] = 0;
	alloc.size = size;
	alloc.align = align;
	alloc.paddr = 0;
	ret = ioctl (fd, HMA_IOCTL_ALLOC, &alloc);
	if (ret < 0)
	{
		error ("hma ioctl failed. %s\n", strerror (errno));
		close (fd);
		return 0;
	}

	if (vaddr)
	{
		if (cache)
		{
			cache = !!cache;
			ret = ioctl (fd, HMA_IOCTL_CACHED, &cache);
			if (ret < 0)
			{
				error ("hma ioctl failed. %s\n", strerror (errno));
				close (fd);
				return 0;
			}
		}

		/* mmap */
		_vaddr = mmap (0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		if (_vaddr == MAP_FAILED)
		{
			error ("mmap failed\n");
			close (fd);
			return 0;
		}
	}

	/* add to local linked list */
	desc = (struct mem_desc*)malloc (sizeof (*desc));
	if (desc == NULL)
	{
		error ("malloc failed\n");
		close (fd);
		return 0;
	}
	desc->fd = fd;
	desc->paddr = alloc.paddr;
	desc->vaddr = _vaddr;
	desc->size = size;
	if (cache)
		desc->type = CACHED;
	else
		desc->type = NONCACHED;
	desc->ump_secure_id = -1;

	pthread_mutex_lock (&root_lock);
	desc->next = desc_root.next;
	desc_root.next = desc;
	pthread_mutex_unlock (&root_lock);

	if (vaddr)
		*vaddr = _vaddr;

	debug ("allocated %lx(%x), size %d, vaddr %p on %d\n",
			alloc.paddr, align, size, _vaddr, fd);

	return alloc.paddr;
}

/**
 * \defgroup libhma HMA user level library
 *
 * 사용자 어플리케이션에서 HMA 메모리를 할당하고 해지하는 함수를 제공해준다.
 * HMA는 연속적인 하드웨어 메모리를 관리하는 모듈로 사용자 어플리케이션에서는
 * \ref libhma 모듈을 통해 \ref hma_drv 을 이용해 메모리를 할당한다.
 *
 * 메모리를 할당할 때에는 libhma_alloc() 함수를 사용하면 되며, libhma_free()
 * 함수를 이용해 메모리를 해지하면 된다.
 */

/**
 * \ingroup libhma
 * \brief HMA 메모리를 할당한다.(noncached memory)
 *
 * aa
 */
unsigned long libhma_alloc (const char *name,
		int size, int align, void **vaddr)
{
	return alloc (name, size, align, vaddr, 0);
}

/**
 * \ingroup libhma
 * \brief HMA 메모리를 할당한다.(ncached memory)
 *
 */
unsigned long libhma_alloc_cached (const char *name,
		int size, int align, void **vaddr)
{
	return alloc (name, size, align, vaddr, 1);
}

/*
 * \brief 이함수를 직접사용하지 말것. libhma_cache_ctl() 사용!!
 */
static int __libhma_cache_ctl(struct mem_desc *desc,
		size_t offset, size_t size, enum libhma_cache_control ctl)
{
	static int dummy_fd = -1;
	int fd = -1;
	int ret;
	struct hma_ioctl_cache_ctl cache_ctl;

	if(desc->fd < 0)
	{
		if(dummy_fd < 0)
		{
			dummy_fd = open ("/dev/hma", O_RDWR);
			if (dummy_fd < 0)
			{
				error ("hma open failed. %s\n",
						strerror (errno));
				return -1;
			}
		}
		fd = dummy_fd;
	}
	else
		fd = desc->fd;

	cache_ctl.paddr = desc->paddr + offset;
	cache_ctl.vaddr = desc->vaddr + offset;
	cache_ctl.size = size;
	switch (ctl)
	{
	default:
	case libhma_cache_clean:
		cache_ctl.operation = HMA_CACHE_CLEAN;
		break;

	case libhma_cache_invalidate:
		cache_ctl.operation = HMA_CACHE_INV;
		break;
	}

#if 0
	printf("hma cache ctl : %x %x %x %x\n", cache_ctl.paddr
			, cache_ctl.vaddr
			, cache_ctl.size
			, cache_ctl.operation);
#endif

	ret = ioctl(fd, HMA_IOCTL_CACHE_CTL, &cache_ctl);
	if (ret < 0)
	{
		error ("hma ioctl failed. %s\n", strerror (errno));
		close (fd);
		fd = -1;
		return -1;
	}

	return 0;
}

int libhma_memcpy_tocpu(void *vaddr_des, void *vaddr_src,
		void *paddr_src, unsigned int size)
{
	struct mem_desc desc;
	int ret = -1;

	desc.fd = -1;
	desc.paddr = (unsigned long)paddr_src;
	desc.vaddr = vaddr_src;

	ret = __libhma_cache_ctl(&desc, 0, size, libhma_cache_invalidate);

	memcpy(vaddr_des, vaddr_src, size);

	return ret;
}

/*
 * HMA memory pool에서 paddr 물리주소를 가지는 mem_desc를 찾는다
 *
 * paddr : libhma_alloc_cached 으로 메모리를 할당 받았을 때 return 된 값. 해당
 * 메모리의 키값으로 사용됨.  현재는 일반 list에서 순차 검색하는 방법이다.
 * 만약 mem_desc item의 수가 많아서 검색 시간이 오래 걸리면 개선된 방법이
 * 필요함.
 */
static struct mem_desc* libhma_search_mem(unsigned long paddr)
{
	struct mem_desc *desc;

	desc = &desc_root;

	pthread_mutex_lock (&root_lock);

	while (desc->next != NULL)
	{
		if (desc->next->paddr == paddr)
		{
			desc = desc->next;
			goto done;
		}
		desc = desc->next;
	}

	error ("unknown paddr. %08lx\n", paddr);
	desc = NULL;

done:
	pthread_mutex_unlock (&root_lock);

	return desc;
}

/**
 * \ingroup libhma
 * \brief   hma에서 할당받은 메모리를 cached로 사용하려면 cache를 수동으로 control하는 것이 필요하며 이를 제공함.
 *          paddr : libhma_alloc_cached 으로 메모리를 할당 받았을 때 return 된 값. 해당 메모리의 키값으로 사용됨.
 *          vaddr : 할당 받은 메모리의 가상주소, 컨트롤 하려는 시작 주소값.
 *          size :  컨트롤 하려는 크기
 *          ctl : cache invalidate(HMA_CACHE_INV), cache clean(HMA_CACHE_CLEAN)
 */
int libhma_cache_ctl(unsigned long paddr, void *vaddr,
		size_t size, enum libhma_cache_control ctl)
{
	struct mem_desc *desc;
	size_t offset;
	int ret = -1;

	if (size == 0)
		return 0;

	if((desc = libhma_search_mem(paddr)) == NULL)
		goto func_error;

	if (vaddr)
	{
		//해당 desc와 vaddr, size, cache_type등 유효성 판별
		if(desc->vaddr > vaddr)
			goto func_error;

		if(((size_t)desc->vaddr + desc->size) < ((size_t)vaddr + size))
			goto func_error;
	}
	else
	{
		vaddr = desc->vaddr;
		size = desc->size;
	}

	if(desc->type == NONCACHED)
		goto func_error;

	if(desc->fd < 0)
		goto func_error;

	offset = (size_t)(vaddr - desc->vaddr);
	//cache control low function
	ret = __libhma_cache_ctl(desc, offset, size, ctl);

	return ret;

func_error:
	error ("hma: unvalid memory region\n");
	return -1;

}

/**
 */
unsigned int libhma_get_ump_secure_id (unsigned long paddr)
{
	struct mem_desc *desc;

	desc = &desc_root;
	pthread_mutex_lock (&root_lock);
	while (desc->next != NULL)
	{
		if (desc->next->paddr == paddr)
		{
			struct mem_desc *tmp;

			tmp = desc->next;

			if (tmp->ump_secure_id == -1)
			{
				int ret;

				ret = ioctl (tmp->fd, HMA_IOCTL_GET_UMP_SECURE_ID, &tmp->ump_secure_id);
				if (ret < 0)
					printf ("HMA_IOCTL_GET_UMP_SECURE_ID failed.\n");
			}

			pthread_mutex_unlock (&root_lock);
			return tmp->ump_secure_id;
		}

		desc = desc->next;
	}
	pthread_mutex_unlock (&root_lock);

	fprintf (stderr, "%s: unknown paddr. %08lx\n", __func__, paddr);

	return -1;
}

/**
 * \ingroup libhma
 * \brief HMA 메모리를 반납한다.
 *
 * free hma memory
 */
void libhma_free (unsigned long paddr)
{
	struct mem_desc *desc;

	debug ("free %lx\n", paddr);
	desc = &desc_root;
	pthread_mutex_lock (&root_lock);
	while (desc->next != NULL)
	{
		if (desc->next->paddr == paddr)
		{
			struct mem_desc *tmp;

			tmp = desc->next;
			if (tmp->vaddr != MAP_FAILED)
			{
				debug ("unmap %p, %d\n",
						tmp->vaddr, tmp->size);
				munmap (tmp->vaddr, tmp->size);
			}
			debug ("close %d\n", tmp->fd);
			close (tmp->fd);
			desc->next = tmp->next;
			free (tmp);

			goto done;
		}

		desc = desc->next;
	}

	error ("unknown paddr. %08lx\n", paddr);

done:
	pthread_mutex_unlock (&root_lock);

	return;
}

