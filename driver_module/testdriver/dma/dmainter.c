#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/freezer.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/wait.h>


/**
 * struct dmatest_params - test parameters.
 * @buf_size:		size of the memcpy test buffer
 * @channel:		bus ID of the channel to test
 * @device:		bus ID of the DMA Engine to test
 * @threads_per_chan:	number of threads to start per channel
 * @max_channels:	maximum number of channels to use
 * @iterations:		iterations before stopping test
 * @xor_sources:	number of xor source buffers
 * @pq_sources:		number of p+q source buffers
 * @timeout:		transfer timeout in msec, -1 for infinite timeout
 */

struct dmatest_params {
	unsigned int	buf_size;
	char		channel[20];
	char		device[32];
	unsigned int	threads_per_chan;
	unsigned int	max_channels;
	unsigned int	iterations;
	unsigned int	xor_sources;
	unsigned int	pq_sources;
	int		timeout;
	bool		noverify;
};

void dmachannel_request(int channel)
{
	struct dmatest_params *params = &info->params;
	dma_cap_mask_t mask;

	dma_cap_zero(mask);
	dma_cap_set(type, mask);

	params->buf_size = test_buf_size;
	strlcpy(params->channel, strim(test_channel), sizeof(params->channel));
	strlcpy(params->device, strim(test_device), sizeof(params->device));
	params->threads_per_chan = threads_per_chan;
	params->max_channels = max_channels;
	params->iterations = iterations;
	params->xor_sources = xor_sources;
	params->pq_sources = pq_sources;
	params->timeout = timeout;
	params->noverify = noverify;

	struct dma_chan *chan;
		chan = dma_request_channel(mask, filter, params);
		if (chan) {
			dev_dbg(NULL，" dma channel %d requested\n");
		} else {
			dev_dbg(NULL，" dma channel %d request failed\n");
		}
	if (dma_has_cap(DMA_MEMCPY, dma_dev->cap_mask)) {
		dev_info(NULL，" dma channel %d requested\n");
	}
}
