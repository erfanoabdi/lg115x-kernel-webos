/*
 * drivers/gpu/lg115x/lg115x_ion.c
 *
 * Description : ION Driver
 *
 */

#include <linux/err.h>
#include <linux/ion.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/export.h>
#include "../ion_priv.h"

struct ion_device *idev;
struct ion_mapper *lg115x_user_mapper;
int num_heaps;
struct ion_heap **heaps;

int lg115x_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = pdev->dev.platform_data;
	int err;
	int i;

	num_heaps = pdata->nr;

	heaps = kzalloc(sizeof(struct ion_heap *) * pdata->nr, GFP_KERNEL);

	idev = ion_device_create(NULL);
	if (IS_ERR_OR_NULL(idev)) {
		kfree(heaps);
		return PTR_ERR(idev);
	}

	/* create the heaps as specified in the board file */
	for (i = 0; i < num_heaps; i++) {
		struct ion_platform_heap *heap_data = &pdata->heaps[i];

		heaps[i] = ion_heap_create(heap_data);
		if (IS_ERR_OR_NULL(heaps[i])) {
			err = PTR_ERR(heaps[i]);
			goto err;
		}
		ion_device_add_heap(idev, heaps[i]);
	}

	platform_set_drvdata(pdev, idev);

	dev_info(&pdev->dev, "registered ion driver\n");
	return 0;
err:
	for (i = 0; i < num_heaps; i++) {
		if (heaps[i])
			ion_heap_destroy(heaps[i]);
	}
	kfree(heaps);
	return err;
}

int lg115x_ion_remove(struct platform_device *pdev)
{
	struct ion_device *idev = platform_get_drvdata(pdev);
	int i;

	ion_device_destroy(idev);
	for (i = 0; i < num_heaps; i++)
		ion_heap_destroy(heaps[i]);
	kfree(heaps);

	dev_info(&pdev->dev, "removed ion driver\n");
	return 0;
}

struct ion_client *lg115x_ion_client_create(unsigned int heap_mask,
					const char *name)
{
	return ion_client_create(idev, name);
}
EXPORT_SYMBOL(lg115x_ion_client_create);

static struct platform_driver ion_driver = {
	.probe = lg115x_ion_probe,
	.remove = lg115x_ion_remove,
	.driver = { .name = "ion-lg115x" }
};

static int __init ion_init(void)
{
	return platform_driver_register(&ion_driver);
}

static void __exit ion_exit(void)
{
	platform_driver_unregister(&ion_driver);
}

module_init(ion_init);
module_exit(ion_exit);
