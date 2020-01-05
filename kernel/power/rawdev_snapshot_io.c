#include <linux/module.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/genhd.h>
#include <linux/device.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/crc32.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <linux/mount.h>
#include <linux/vmalloc.h>
#include <linux/mtd/partitions.h>
#include <linux/crypto.h>
#include <linux/lzo.h>
#include <mtd/mtd-abi.h>
#include <asm/uaccess.h>
#include <asm/suspend.h>

#include "power.h"

int lgsnap_image_size = 0;

static char compress_type[20] = SNAP_COMP_ALGO;

/* Number of pages needed to save the original pfns of the image pages */
static unsigned int nr_meta_pages;

/*
 * Number of free pages that are not high.
 */
static inline unsigned long low_free_pages(void)
{
	return nr_free_pages() - nr_free_highpages();
}

/*
 * Number of pages required to be kept free while writing the image. Always
 * half of all available low pages before the writing starts.
 */
static inline unsigned long reqd_free_pages(void)
{
	return low_free_pages() / 2;
}

/**
 *	The rawdev_handle structure is used for handling raw device in
 *	a file-alike way
 */
struct rawdev_handle {
	sector_t cur_pos;
	unsigned long reqd_free_pages;
};

/*
 * Saving part
 */

/**
 *	swsusp_rawdev_check - check if the resume device is a swap device
 *	and get its index (if so)
 *
 *	This is called before saving image
 */
static long swsusp_rawdev_check(void)
{
	long res;

	hib_resume_bdev = blkdev_get_by_dev(swsusp_resume_device,
					    FMODE_WRITE, NULL);
	if (IS_ERR(hib_resume_bdev))
		return PTR_ERR(hib_resume_bdev);

	res = set_blocksize(hib_resume_bdev, PAGE_SIZE);
	if (res < 0)
		blkdev_put(hib_resume_bdev, FMODE_WRITE);

	return res;
}

/**
 *	write_page - Write one page to given swap location.
 *	@buf:		Address we're writing.
 *	@offset:	Offset of the swap page we're writing to.
 *	@bio_chain:	Link the next write BIO here
 */

static int write_page(void *buf, sector_t offset, struct bio **bio_chain)
{
	void *src;
	int ret;

	if (bio_chain) {
		src = (void *)__get_free_page(__GFP_WAIT | __GFP_NOWARN |
		                              __GFP_NORETRY);
		if (src) {
			copy_page(src, buf);
		} else {
			ret = hib_wait_on_bio_chain(bio_chain); /* Free pages */
			if (ret)
				return ret;
			src = (void *)__get_free_page(__GFP_WAIT |
			                              __GFP_NOWARN |
			                              __GFP_NORETRY);
			if (src) {
				copy_page(src, buf);
			} else {
				WARN_ON_ONCE(1);
				bio_chain = NULL;	/* Go synchronous */
				src = buf;
			}
		}
	} else {
		src = buf;
	}
	return hib_bio_write_page(offset, src, bio_chain);
}

/**
 *	rawdev_close - close raw device.
 */

void rawdev_close(fmode_t mode)
{
	if (IS_ERR(hib_resume_bdev)) {
		pr_debug("PM: Image device not initialised\n");
		return;
	}
	
	sync_blockdev(hib_resume_bdev);
	blkdev_put(hib_resume_bdev, mode);
}

static int get_rawdev_writer(struct rawdev_handle *handle)
{
	int ret;

	ret = swsusp_rawdev_check();
	if (ret)
		return ret;

	handle->cur_pos = 0;
	handle->reqd_free_pages = reqd_free_pages();
	return 0;
}

static int rawdev_write_page(struct rawdev_handle *handle, void *buf,
				struct bio **bio_chain)
{
	int error = 0;

	error = write_page(buf, handle->cur_pos, bio_chain);
	if (error)
		return error;

	handle->cur_pos += 1;

	if (bio_chain && low_free_pages() <= handle->reqd_free_pages) {
		error = hib_wait_on_bio_chain(bio_chain);
		if (error)
			goto out;
		/*
		 * Recalculate the number of required free pages, to
		 * make sure we never take more than half.
		 */
		handle->reqd_free_pages = reqd_free_pages();
	}
	
out:	
	return error;
}

static int rawdev_writer_finish(void)
{
	rawdev_close(FMODE_WRITE);

	return 0;
}

/* We need to remember how much compressed data we need to read. */
#define LZO_HEADER	sizeof(size_t)

/* Number of pages/bytes we'll compress at one time. */
#define LZO_UNC_PAGES	32
#define LZO_UNC_SIZE	(LZO_UNC_PAGES * PAGE_SIZE)

/* Number of pages/bytes we need for compressed data (worst case). */
#define LZO_CMP_PAGES	DIV_ROUND_UP(lzo1x_worst_compress(LZO_UNC_SIZE) + \
			             LZO_HEADER, PAGE_SIZE)
#define LZO_CMP_SIZE	(LZO_CMP_PAGES * PAGE_SIZE)

/* Maximum number of threads for compression/decompression. */
#define LZO_THREADS	3

/* Minimum/maximum number of pages for read buffering. */
#define LZO_MIN_RD_PAGES	1024
#define LZO_MAX_RD_PAGES	8192


/**
 *	save_image - save the suspend image data
 */
 
static int save_image(struct rawdev_handle *handle,
			struct snapshot_handle *snapshot,
			unsigned int nr_to_write,
			struct swsusp_info *info)
{
	unsigned int m;
	int ret;
	int nr_pages;
	int err2;
	struct bio *bio;
	struct timeval start;
	struct timeval stop;
	struct snapshot_header *header = GET_SNAP_HEADER(info);

	printk(KERN_INFO "PM: Saving image data pages (%u pages) ...     ", nr_to_write);
	m = nr_to_write / 100;
	if (!m)
		m = 1;
	nr_pages = 0;
	bio = NULL;

	do_gettimeofday(&start);

	while (1) {
		ret = snapshot_read_next(snapshot);
		if (ret <= 0)
			break;

		header->crc = crc32_be(header->crc, data_of(*snapshot), PAGE_SIZE);
		/*
		 * Write payload (one page)
		 */
		ret = rawdev_write_page(handle, data_of(*snapshot), &bio);
		if (ret)
			break;

		if (!(nr_pages % m))
			printk(KERN_CONT "\b\b\b\b%3d%%", nr_pages / m);

		nr_pages++;
	}
	err2 = hib_wait_on_bio_chain(&bio);
	do_gettimeofday(&stop);

	if (!ret)
		ret = err2;
	else
		printk(KERN_CONT "\b\b\b\bdone\n");
	swsusp_show_speed(&start, &stop, nr_to_write, "Wrote");

	header->image_size = snapshot_get_image_size();

	printk("Actual requested size = %d, written image size = %d kbytes\n",
		nr_to_write * 4, nr_pages * 4);

	return ret;
}

/**
 * Structure used for CRC32.
 */
struct crc_data {
	struct task_struct *thr;                  /* thread */
	atomic_t ready;                           /* ready to start flag */
	atomic_t stop;                            /* ready to stop flag */
	unsigned run_threads;                     /* nr current threads */
	wait_queue_head_t go;                     /* start crc update */
	wait_queue_head_t done;                   /* crc update done */
	u32 *crc32;                               /* points to handle's crc32 */
	size_t len;	   		          /* uncompressed lengths */
	unsigned char *buf;			  /* uncompressed data */
};

/**
 * CRC32 update function that runs in its own thread.
 */
static int crc32_threadfn(void *data)
{
	struct crc_data *d = data;

	while (1) {
		wait_event(d->go, atomic_read(&d->ready) ||
		                  kthread_should_stop());

		if (kthread_should_stop()) {
			d->thr = NULL;
			atomic_set(&d->stop, 1);
			wake_up(&d->done);
			break;
		}
		atomic_set(&d->ready, 0);
		*d->crc32 = crc32_be(*d->crc32, d->buf, d->len);
		atomic_set(&d->stop, 1);
		wake_up(&d->done);
	}
	return 0;
}

/**
 * Structure used for LZO data compression.
 */
struct cmp_data {
	struct task_struct *thr;                  /* thread */
	struct crypto_comp *tfm;		  /* crypto struct */			
	atomic_t ready;                           /* ready to start flag */
	atomic_t stop;                            /* ready to stop flag */
	int ret;                                  /* return code */
	wait_queue_head_t go;                     /* start compression */
	wait_queue_head_t done;                   /* compression done */
	size_t unc_len;                           /* uncompressed length */
	size_t cmp_len;                           /* compressed length */
	unsigned char unc[LZO_UNC_SIZE];          /* uncompressed buffer */
	unsigned char cmp[LZO_CMP_SIZE];          /* compressed buffer */
};

/**
 * Compression function that runs in its own thread.
 */
static int lzo_compress_threadfn(void *data)
{
	struct cmp_data *d = data;

	while (1) {
		wait_event(d->go, atomic_read(&d->ready) ||
		                  kthread_should_stop());
		if (kthread_should_stop()) {
			d->thr = NULL;
			d->ret = -1;
			atomic_set(&d->stop, 1);
			wake_up(&d->done);
			break;
		}
		atomic_set(&d->ready, 0);

		d->ret = crypto_comp_compress(d->tfm, d->unc, d->unc_len,
		                          d->cmp, &d->cmp_len);
		atomic_set(&d->stop, 1);
		wake_up(&d->done);
	}
	return 0;
}

#define MB(x) (x/(1024 *1024))
static void print_header(struct swsusp_info *info)
{
	extern char *lgemmc_get_partname(int partnum);
	struct snapshot_header *header = GET_SNAP_HEADER(info);
	struct dep_parts *deps = &header->deps;

	printk("/***************************************************************/\n");
	printk("UTS sys name = %s\n", info->uts.sysname);
	printk("UTS node name = %s\n", info->uts.nodename);
	printk("UTS release = %s\n", info->uts.release);
	printk("UTS version = %s\n", info->uts.version);
	printk("UTS machine = %s\n", info->uts.machine);
	printk("UTS domainname = %s\n", info->uts.domainname);

	printk("Kernel %d.%d.%d\n",
	       ((info->version_code) >> 16) & 0xff,
	       ((info->version_code) >> 8) & 0xff,
	       (info->version_code) & 0xff);

	printk("num_phyuspages = %lu\n", info->num_physpages);
	printk("cpus = %d\n", info->cpus);
	printk("image_pages = %lu\n", info->image_pages);
	printk("pages = %lu\n", info->pages);
	printk("size = %luMB(%lu)\n", MB(info->size), info->size);
	
	if(header->compress_algo)
		printk("compressed(%s) image size = %luMB(%lu)\n", compress_type, MB(header->image_size), header->image_size);
	else
		printk("uncompressed image size = %luMB(%lu)\n", MB(header->image_size), header->image_size);
		
	if(header->crc != ~0) 
		printk("crc = %u\n", header->crc);
	else
		printk("no crc image\n");
	printk("pfn merge info entry count = %u\n", header->pfn_mi_cnt);
	printk("metadata start offset for compressed image = %lu\n", header->metadata_start_offset_for_comp);
	printk("physical address resume func addr %lx\n", header->pa_resume_func);
	printk("snapshot magic number 0x%8.8lx\n", header->magic);
	if (deps->nr_dep_parts) {
		int i;
		struct dep_part_info *info = &deps->dep_part_info[0];
		
		printk("snapshot dependency %d partitions\n", deps->nr_dep_parts);
		printk(" name\t partnum\t %s\t %s\n", deps->value_name[0], deps->value_name[1]);

		for (i = 0; i < deps->nr_dep_parts; i++) {
			printk(" %s\t  %4d\t\t 0x%-8x\t  0x%x\n",
				lgemmc_get_partname(info[i].partnum),
				info[i].partnum,
				info[i].value[0],
				info[i].value[1]);
		}
	} else {
		printk("snapshot has no dependency partitions\n");
	}
	printk("/***************************************************************/\n");

	return;
}

/**
 * merge_adjacent_pfn - merge adjacent pfn and save it into pfn_merge_info structure
 */
static struct pfn_merge_info *merge_adjacent_pfn(struct swsusp_info *info, struct snapshot_handle *snapshot)
{
	int ret;
	int nr_pages = 0, pfn_mi_index = 0;
	int in_page_index, pfn_diff;
	unsigned int *pfn_ptr, *compare_pfn_ptr;
	unsigned int seed_pfn = 0, nr_walked_pfn = 0;
	struct pfn_merge_info *pfn_merge_info;
	struct snapshot_header *header = GET_SNAP_HEADER(info);

	pfn_merge_info = kmalloc(info->image_pages * sizeof(struct pfn_merge_info), GFP_KERNEL);
	if (pfn_merge_info == NULL) {
		printk("%s[%d] : can`t alloc output buffer to compress\n", __func__, __LINE__);
		return NULL;
	}

	while (nr_pages < nr_meta_pages) {
		ret = snapshot_read_next(snapshot);
		if (ret <= 0)
			break;

		pfn_ptr = (unsigned int *)data_of(*snapshot);

		if (nr_pages == 0) {
			seed_pfn = *pfn_ptr;
			pfn_merge_info[pfn_mi_index].start_pfn = seed_pfn;
			pfn_merge_info[pfn_mi_index].info.merged_cnt = 1;
			in_page_index = 1;
			nr_walked_pfn++;
		} else {
			in_page_index = 0;
		}

		while (in_page_index < (PAGE_SIZE/4)) {
			compare_pfn_ptr = pfn_ptr + in_page_index;

			pfn_diff = *compare_pfn_ptr - seed_pfn;
			seed_pfn = *compare_pfn_ptr;

			if (pfn_diff == 1 && (pfn_merge_info[pfn_mi_index].info.merged_cnt < LZO_UNC_PAGES)) { // adjacent pfn
				pfn_merge_info[pfn_mi_index].info.merged_cnt++;
			} else {
				pfn_mi_index++;
				pfn_merge_info[pfn_mi_index].start_pfn = seed_pfn;
				pfn_merge_info[pfn_mi_index].info.merged_cnt = 1;
			}

			nr_walked_pfn++;
			if (nr_walked_pfn == info->image_pages)
				break;

			in_page_index++;
		}

		nr_pages++;
	}
	header->pfn_mi_cnt = pfn_mi_index + 1;

	return pfn_merge_info;
}

/**
 *	save_image_compress - save the suspend image data with compression
 */
static int save_image_compress(struct rawdev_handle *handle,
			struct snapshot_handle *snapshot,
			unsigned int nr_to_write, struct swsusp_info *info)
{
	unsigned int m;
	int ret = 0;
	int nr_pages;
	int err2;
	struct bio *bio;
	struct timeval start;
	struct timeval stop;
	size_t off, pgoff = 0;
	size_t left_len, len, slice;
	unsigned char *buf;
	unsigned thr, run_threads, nr_threads;
	unsigned char *page = NULL;
	struct cmp_data *data = NULL;
	struct crc_data *crc = NULL;
	struct pfn_merge_info *pfn_merge_info;
	struct snapshot_header *header;

	int pfn_mi_index;

	/*
	 * We'll limit the number of threads for compression to limit memory
	 * footprint. 
	 * change nr_thread = num_online_cpus() - 1 to  nr_thread = num_online_cpus() 
	 * for compress performance by sangseok.lee
	 */
	nr_threads = num_online_cpus();
	nr_threads = clamp_val(nr_threads, 1, LZO_THREADS);

	pfn_merge_info = merge_adjacent_pfn(info, snapshot);
	if (pfn_merge_info == NULL)
		goto out_clean;

	header = GET_SNAP_HEADER(info);

	page = (void *)__get_free_page(__GFP_WAIT | __GFP_HIGH);
	if (!page) {
		printk(KERN_ERR "PM: Failed to allocate LZO page\n");
		ret = -ENOMEM;
		goto out_clean;
	}

	data = vmalloc(sizeof(*data) * nr_threads);
	if (!data) {
		printk(KERN_ERR "PM: Failed to allocate LZO data\n");
		ret = -ENOMEM;
		goto out_clean;
	}
	for (thr = 0; thr < nr_threads; thr++)
		memset(&data[thr], 0, offsetof(struct cmp_data, go));

	crc = kmalloc(sizeof(*crc), GFP_KERNEL);
	if (!crc) {
		printk(KERN_ERR "PM: Failed to allocate crc\n");
		ret = -ENOMEM;
		goto out_clean;
	}
	memset(crc, 0, offsetof(struct crc_data, go));

	/*
	 * Start the compression threads.
	 */
	for (thr = 0; thr < nr_threads; thr++) {
		init_waitqueue_head(&data[thr].go);
		init_waitqueue_head(&data[thr].done);

		data[thr].thr = kthread_run(lzo_compress_threadfn,
		                            &data[thr],
		                            "image_compress/%u", thr);
		if (IS_ERR(data[thr].thr)) {
			data[thr].thr = NULL;
			printk(KERN_ERR
			       "PM: Cannot start compression threads\n");
			ret = -ENOMEM;
			goto out_clean;
		}

		data[thr].tfm = crypto_alloc_comp(compress_type, 0, 0);
		if (data[thr].tfm == NULL) {
			printk("PM: crypto_alloc_comp fail\n");
			goto out_clean;
		}
	}

	/*
	 * Start the CRC32 thread.
	 */
	init_waitqueue_head(&crc->go);
	init_waitqueue_head(&crc->done);

	crc->crc32 = &header->crc;
	crc->thr = kthread_run(crc32_threadfn, crc, "image_crc32");
	if (IS_ERR(crc->thr)) {
		crc->thr = NULL;
		printk(KERN_ERR "PM: Cannot start CRC32 thread\n");
		ret = -ENOMEM;
		goto out_clean;
	}

	printk(KERN_INFO
		"PM: Using %u thread(s) for compression.\n"
		"PM: Compressing and saving image data (%u pages)...\n",
		nr_threads, nr_to_write);
	m = nr_to_write / 10;
	if (!m)
		m = 1;
	nr_pages = 0;
	bio = NULL;
	do_gettimeofday(&start);
	
	pfn_mi_index = 0;
	for (;;) {
		for (thr = 0; thr < nr_threads; thr++) {
			for (off = 0; off < PAGE_SIZE * pfn_merge_info[pfn_mi_index].info.merged_cnt; off += PAGE_SIZE) {
				ret = snapshot_read_next(snapshot);
				if (ret < 0)
					goto out_finish;

				if (!ret)
					break;

				memcpy(data[thr].unc + off,
				       data_of(*snapshot), PAGE_SIZE);

				if (!(nr_pages % m))
					printk(KERN_INFO
					       "PM: Image saving progress: "
					       "%3d%%\n",
				               nr_pages / m * 10);
				nr_pages++;
			}
			if (!off)
				break;

			data[thr].unc_len = off;

			atomic_set(&data[thr].ready, 1);
			wake_up(&data[thr].go);
			pfn_mi_index++;
		}

		if (!thr)
			break;

		for (run_threads = thr, thr = 0; thr < run_threads; thr++) {
			wait_event(data[thr].done,
			           atomic_read(&data[thr].stop));
			atomic_set(&data[thr].stop, 0);

			ret = data[thr].ret;

			if (ret < 0) {
				printk(KERN_ERR "PM: LZO compression failed\n");
				goto out_finish;
			}

			if (unlikely(!data[thr].cmp_len ||
			             data[thr].cmp_len >
			             lzo1x_worst_compress(data[thr].unc_len))) {
				printk(KERN_ERR
				       "PM: Invalid %s compressed length\n", compress_type);
				ret = -1;
				goto out_finish;
			}

			if(data[thr].cmp_len >= data[thr].unc_len) {
				len = data[thr].unc_len;
				buf = data[thr].unc;
				pfn_merge_info[pfn_mi_index - run_threads + thr].compressed = 0;
			} else {
				len = data[thr].cmp_len;
				buf = data[thr].cmp;
				pfn_merge_info[pfn_mi_index - run_threads + thr].compressed = 1;
			}

			if(thr != 0) {
				wait_event(crc->done, atomic_read(&crc->stop));
				atomic_set(&crc->stop, 0);
			}

			crc->buf = buf;
			crc->len = len;
			atomic_set(&crc->ready, 1);
			wake_up(&crc->go);

			/* update header */
			header->image_size += len;
			header->metadata_start_offset_for_comp += len;

			/* update metadata with compressed block size */
			pfn_merge_info[pfn_mi_index - run_threads + thr].info.compblock_len = len;
			
			/* Write payload (compressed page) */
			left_len = len;
			
			while (left_len > 0) {
				if(left_len > PAGE_SIZE - pgoff) {
					slice = PAGE_SIZE - pgoff;
					left_len -= slice;
				} else {
					slice = left_len;
					left_len = 0;
				}
				memcpy((void *)(page + pgoff), buf, slice);
				pgoff += slice;
				buf += slice;
				
				if(pgoff == PAGE_SIZE) {
					rawdev_write_page(handle, page, &bio);
					pgoff = 0;
				}
			}
		}

		wait_event(crc->done, atomic_read(&crc->stop));
		atomic_set(&crc->stop, 0);
	}

	for (pfn_mi_index = 0; pfn_mi_index < header->pfn_mi_cnt; pfn_mi_index++)
		header->crc = crc32_be(header->crc, (void *)&pfn_merge_info[pfn_mi_index], sizeof(struct pfn_merge_info));

	left_len = sizeof(struct pfn_merge_info) * header->pfn_mi_cnt;
	buf = (unsigned char *)pfn_merge_info;

	header->image_size += left_len;

	while(left_len > 0) {
		if(left_len > PAGE_SIZE - pgoff) {
			slice = PAGE_SIZE - pgoff;
			left_len -= slice;
		} else {
			slice = left_len;
			left_len = 0;
		}
		
		memcpy((void *)(page + pgoff), buf, slice);
		pgoff += slice;
		buf += slice;
		
		if(pgoff == PAGE_SIZE) {
			rawdev_write_page(handle, page, &bio);
			pgoff = 0;
		}
	}
	if(pgoff != 0)
		rawdev_write_page(handle, page, &bio);
	
out_finish:
	err2 = hib_wait_on_bio_chain(&bio);
	do_gettimeofday(&stop);
	if (!ret)
		ret = err2;
	if (!ret)
		printk(KERN_INFO "PM: Image saving done.\n");

	swsusp_show_speed(&start, &stop, nr_to_write, "Wrote");
out_clean:
	if (crc) {
		if (crc->thr)
			kthread_stop(crc->thr);
		kfree(crc);
	}
	if (data) {
		for (thr = 0; thr < nr_threads; thr++) {
			if (data[thr].thr)
				kthread_stop(data[thr].thr);
			
			crypto_free_comp(data[thr].tfm);
		}
		vfree(data);
	}
	if (page) free_page((unsigned long)page);

	return ret;

}

/* kernel/power/main.c */
extern char *snapshot_dep_parts;
int set_snapshot_header_dep_parts(struct snapshot_header *header)
{
	extern int lgemmc_get_partnum(const char *name);
	extern unsigned int lgemmc_get_filesize(int partnum);
	extern unsigned int lgemmc_get_sw_version(int partnum);
	
	struct dep_parts *deps = &header->deps;
	struct dep_part_info *info = &deps->dep_part_info[0];
	char *p, *copy, *name;
	int i;

	if (!snapshot_dep_parts) {
		deps->nr_dep_parts = 0;
		return 0;
	}

	strcpy(deps->part_type, "lgemmc");
	strcpy(deps->value_name[0], "file size");
	strcpy(deps->value_name[1], "sw ver");

	copy = kstrdup(snapshot_dep_parts, GFP_KERNEL);
	if (!copy)
		return -ENOMEM;

	i = 0;
	p = copy;
	while ((name = strsep(&p, ",")) != NULL) {
		int partnum = lgemmc_get_partnum(name);

		if (partnum < 0)
			continue;

		info[i].partnum = partnum;
		info[i].value[0] = lgemmc_get_filesize(partnum);
		info[i].value[1] = lgemmc_get_sw_version(partnum);
		i++;

		if (i >= MAX_DEP_PARTS) {
			printk("number of dependency partitions exceeds max\n");
			break;
		}
	}
	deps->nr_dep_parts = i;
	
	kfree(copy);
	return 0;
}

static enum compress_algorithm get_compress_algo(void)
{
	if(!strcmp(&compress_type[0], "lzo"))
		return LZO;
	else if(!strcmp(&compress_type[0], "lz4hc"))
		return LZ4HC;
	else if(!strcmp(&compress_type[0], "lz4"))
		return LZ4;

	return UNCOMPRESSED;
}

extern asmlinkage int swsusp_arch_resume_restore_nosave(void);
int preset_snapshot_header(struct snapshot_header *header)
{
	header->magic = 0;
	header->compress_algo = get_compress_algo();
	if(header->compress_algo)
		header->image_size = PAGE_SIZE;
	else
		header->image_size = 0;
		
	header->pfn_mi_cnt = 0;
	header->metadata_start_offset_for_comp = PAGE_SIZE;
	header->crc = ~0;
	header->pa_resume_func = __pa(swsusp_arch_resume_restore_nosave);

	return set_snapshot_header_dep_parts(header);
}

/**
 *	enough_rawdev - Make sure we have enough rawdev to save the image.
 *
 *	Returns TRUE or FALSE after checking the total amount of swap
 *	space avaiable from the resume partition.
 */

static int enough_rawdev(unsigned int nr_pages, unsigned int flags)
{
	unsigned int free_rawdev = get_capacity(hib_resume_bdev->bd_disk);
	unsigned int required;

	pr_debug("PM: Free raw dev pages: %u\n", free_rawdev);

	//TODO : check do we need PAGES_FOR_IO?
	required = PAGES_FOR_IO + nr_pages;
	return free_rawdev > required;
}

int lgsnap_write_magic(void)
{
	int error;
	struct swsusp_info *info;
	struct rawdev_handle handle;
	struct snapshot_header *header;

	error = get_rawdev_writer(&handle);

	info = kmalloc(sizeof(struct swsusp_info), GFP_KERNEL);
	if (info == NULL) {
		printk("%s[%d] Can`t allocate kernel buffer\n", __func__, __LINE__);
		error = -EFAULT;
		goto out_finish;
	}

	hib_bio_read_page(0, info, NULL);
	header = GET_SNAP_HEADER(info);
	header->magic = LG_SNAPSHOT_MAGIC_CODE;

	kfree(info);
out_finish:
	error = rawdev_writer_finish();

	return error;
}

#ifdef CONFIG_LG_SNAPSHOT_DENY_STATE
#define LGSNAP_SET_MAGIC(header)
#else
#define LGSNAP_SET_MAGIC(header) do { header->magic = LG_SNAPSHOT_MAGIC_CODE;  } while(0)
#endif

/**
 *  raw_dev_snapshot_write - Write entire data and metadata pages
 *  Open raw mmcblk directly and write snapshot image
 */
int rawdev_snapshot_write(unsigned int flags)
{
	struct rawdev_handle handle;
	struct snapshot_handle snapshot;
	struct swsusp_info *info;
	struct snapshot_header *header;
	unsigned long pages;
	int error;

	pages = snapshot_get_image_size();
	error = get_rawdev_writer(&handle);
	if (error) {
		printk(KERN_ERR "PM: Cannot get rawdev writer\n");
		return error;
	}
	if (flags & SF_NOCOMPRESS_MODE) {
		if (!enough_rawdev(pages, flags)) {
			printk(KERN_ERR "PM: Not enough raw dev space\n");
			error = -ENOSPC;
			goto out_finish;
		}
	}

	/* Write header first */
	memset(&snapshot, 0, sizeof(struct snapshot_handle));
	error = snapshot_read_next(&snapshot);
	if (error < PAGE_SIZE) {
		if (error >= 0)
			error = -EFAULT;

		goto out_finish;
	}

	/* Make header copy for last update */
	info = kmalloc(sizeof(struct swsusp_info), GFP_KERNEL);
	if (info == NULL) {
		printk("%s[%d] Can`t allocate kernel buffer\n", __func__, __LINE__);
		error = -EFAULT;
		goto out_finish;
	}
	
	memcpy((void *)info, data_of(snapshot), sizeof(struct swsusp_info));

	header = GET_SNAP_HEADER(info);

	/* Preset metadata in snapshot header */
	error = preset_snapshot_header(header);
	if (error < 0)
		goto free_info;

	/* -1 means header page, image_pages means payload pages except pages for metadata */
	nr_meta_pages = info->pages - info->image_pages - 1;

	rawdev_write_page(&handle, info, NULL);

	/* Actual, image write */
	if(!header->compress_algo)
		error = save_image(&handle, &snapshot, pages - 1, info);
	else
		error = save_image_compress(&handle, &snapshot, pages - 1, info);

	handle.cur_pos = 0;

	LGSNAP_SET_MAGIC(header);
	
	rawdev_write_page(&handle, info, NULL);

	print_header(info);
	
	lgsnap_image_size = header->image_size;

	printk("Start of resume sequence atfer making hibernation image\n");

free_info:
	kfree(info);
out_finish:
	error = rawdev_writer_finish();

	return error;
}

