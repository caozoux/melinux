
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/tracepoint.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"
#include "ktrace.h"

int register_tracepoint(const char *name, void *probe, void *data)
{
    struct tracepoint *tr_point;

    tr_point = find_tracepoint(name);
    if (!tr_point)
        return 0;

    return tracepoint_probe_register(tr_point, probe, data);
}                                                                                                                                                                                                                                        
int unregister_tracepoint(const char *name, void *probe, void *data)
{
    struct tracepoint *tr_point;
    int ret = 0;

    tr_point = find_tracepoint(name);
    if (!tr_point)
        return 0;

    do {
        ret = tracepoint_probe_unregister(tr_point, probe, data);
    } while (ret == -ENOMEM);

    return ret;
}

