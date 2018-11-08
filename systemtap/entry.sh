#!/usr/bin/env stap
probe kernel.function("submit_bio")
{
	dev = $bio->bi_bdev->bd_dev
	if (dev == device_of_interest)
	printf ("[%s](%d) dev:0x%x rw:%d size:%d\n",
	execname(), pid(), dev, $rw, $bio->bi_size)
}
