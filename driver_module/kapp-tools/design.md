# kapp-tools Design Documentation

## Overview

kapp-tools is a Linux kernel diagnostic and fault injection toolkit consisting of a kernel module (`ksys_tool.ko`) and a userspace control utility (`kapp-tool`). It provides comprehensive debugging, monitoring, and fault injection capabilities for Linux kernel subsystems.

## Architecture

```
kapp-tools/
├── src/
│   ├── module/          # Kernel module (ksys_tool.ko)
│   │   ├── kapp_entry.c         # Module entry point
│   │   ├── include/             # Public headers
│   │   ├── base/                # Core infrastructure
│   │   ├── kprobe/              # Kprobe tracing module
│   │   ├── ktrace/              # Kernel tracing module
│   │   ├── kinject/             # Fault injection module
│   │   ├── kmem/                # Memory diagnostics module
│   │   ├── kblock/              # Block I/O diagnostics module
│   │   ├── kdevice/             # Device diagnostics module
│   │   ├── kstack/              # Stack trace module
│   │   ├── krunlog/             # Runtime logging module
│   │   ├── ktree_list/          # Data structure diagnostics module
│   │   ├── ksched/              # Scheduler diagnostics module
│   │   └── knet/                # Network diagnostics module
│   │
│   └── app/             # Userspace utility (kapp-tool)
│       ├── entry.cpp             # Main entry point
│       ├── lib/                 # Helper libraries
│       └── [module].cpp          # Control handlers for each module
```

## Components

### 1. Kernel Module (`module/`)

#### Entry Point (`kapp_entry.c`)
- Main module initialization and cleanup
- Misc device registration (`/dev/ksysd`)
- IOCTL command dispatcher
- Unit module initialization
- Proc filesystem root (`/proc/ksys`)

**Key Functions:**
- `ksys_tool_init()` - Module initialization
- `ksys_tool_exit()` - Module cleanup
- `ksysd_template_unlocked_ioctl()` - IOCTL command routing

#### Core Infrastructure (`base/`)
- `base.c` - Base function initialization
- `percpu_variable.c` - Per-CPU variable management
- `trace.c` - Kernel trace support
- `buffer.c` - Buffer management
- `runlog.c` - Runtime logging
- `trace_buffer.c` - Trace buffer management
- `slab.c` - Slab allocator diagnostics
- `stack.c` - Stack trace utilities
- `scan.c` - Memory scanning utilities

#### Diagnostic Modules

##### kprobe (`kprobe/`)
- Kprobe-based function hooking
- Kernel function entry/exit tracing
- `kprobe_ioctl.h` - IOCTL commands for kprobe control

##### ktrace (`ktrace/`)
- Kernel event tracing
- Custom trace point support
- `ktrace_ioctl.h` - IOCTL commands for trace control

##### kinject (`kinject/`)
- Fault injection framework
- Supported injection types:
  - `kinject_slub.c` - SLUB allocator fault injection
  - `kinject_lock.c` - Lock-related fault injection
  - `kinject_timer.c` - Timer fault injection
  - `kinject_sem.c` - Semaphore fault injection
  - `kinject_workqeue.c` - Workqueue fault injection
  - `kinject_thread.c` - Thread fault injection
  - `kinject_rhashtable.c` - rhashtable fault injection
  - `kinject_jumplabe.c` - Jump label fault injection
  - `kinject_test.c` - Test utilities
- `kinject_ioctl.h` - IOCTL commands for fault injection

##### kmem (`kmem/`)
- Memory subsystem diagnostics
- `kmem_dump.c` - Memory dump utilities
- `cgroup/` - cgroup memory diagnostics
- `slab/` - SLAB allocator diagnostics
- `page/` - Page allocator diagnostics
- `kmem_ioctl.h` - IOCTL commands for memory operations

##### kblock (`kblock/`)
- Block I/O subsystem diagnostics
- `super.c` - Superblock operations
- `inode.c` - Inode operations
- `dentry.c` - Dentry operations
- `bio.c` - BIO operations
- `request.c` - Request handling
- `kblock_qos.c` - Block QoS monitoring
- `kblock_trace.c` - Block I/O tracing
- `kblock_ioctl.h` - IOCTL commands for block operations

##### kdevice (`kdevice/`)
- Device model diagnostics
- Device enumeration and inspection
- `kdevice_ioctl.h` - IOCTL commands for device operations

##### kstack (`kstack/`)
- Stack trace diagnostics
- Kernel/user space stack dumping
- `kstack_ioctl.h` - IOCTL commands for stack operations

##### krunlog (`krunlog/`)
- Runtime log collection and retrieval
- `krunlog_ioctl.h` - IOCTL commands for log operations

##### ktree_list (`ktree_list/`)
- Tree and list data structure diagnostics
- Red-black tree and list inspection
- `ktree_list_ioctl.h` - IOCTL commands for tree/list operations

##### ksched (`ksched/`)
- Scheduler diagnostics
- `ksched_domain.c` - Scheduling domain analysis
- `ksched_montor.c` - Scheduler monitoring
- `ksched_ioctl.h` - IOCTL commands for scheduler operations

##### knet (`knet/`)
- Network subsystem diagnostics
- Network device inspection
- Socket statistics
- `knet_ioctl.h` - IOCTL commands for network operations

### 2. Userspace Utility (`app/`)

#### Entry Point (`entry.cpp`)
- Command-line interface dispatcher
- Module command routing
- Help system

#### Module Control Handlers
Each kernel module has a corresponding control handler:
- `kinject.cpp` - Fault injection control
- `kmem.cpp` - Memory diagnostics control
- `kblock.cpp` - Block I/O diagnostics control
- `ksched.cpp` - Scheduler diagnostics control
- `krunlog.cpp` - Runtime log control
- `kstack.cpp` - Stack trace control
- `kprobe.cpp` - Kprobe control
- `ktrace.cpp` - Trace control

#### Helper Library (`lib/`)
- `symbole.cpp/h` - ELF symbol resolution

## Communication Interface

### IOCTL Commands

The kernel module communicates with userspace via IOCTL through `/dev/ksysd`.

**Base Command Structure:**
```c
struct ioctl_ksdata {
    enum ioctl_cmdtype cmd;  // Module type
    int subcmd;              // Sub-command
    void *data;              // Data pointer
    int len;                 // Data length
};
```

**Command Types (`ioctl_cmdtype`):**
- `IOCTL_KPROBE` - Kprobe operations
- `IOCTL_KTRACE` - Trace operations
- `IOCTL_INJECT` - Fault injection
- `IOCTL_KMEM` - Memory operations
- `IOCTL_KDEVICE` - Device operations
- `IOCTL_KBLOCK` - Block I/O operations
- `IOCTL_KRUNLOG` - Runtime log operations
- `IOCTL_KSTACK` - Stack operations
- `IOCTL_KTREE` - Tree/list operations
- `IOCTL_KSCHED` - Scheduler operations
- `IOCTL_KNET` - Network operations

### Proc Interface

Module exports a proc filesystem root at `/proc/ksys` for additional data retrieval.

## Usage

### Building

```bash
# Build kernel module
cd src/module
make

# Build userspace tool
cd ../app
make
```

### Loading Module

```bash
insmod ksys_tool.ko
```

### Running Userspace Tool

```bash
./kapp-tool [command] [args]

Commands:
  inject   - Fault injection
  kmem     - Memory diagnostics
  kblock   - Block I/O diagnostics
  ksched   - Scheduler diagnostics
  klog     - Runtime logs
  kstack   - Stack traces
  kprobe   - Kprobe tracing
  help     - Show help for all commands
```

## Design Patterns

### Unit Module Registration

Each diagnostic module registers itself using the `KSYSD_UNIT` macro:

```c
#define KSYSD_UNIT(name, utype) { \
    .u_name = #name, \
    .type = utype, \
    .ioctl = name##_unit_ioctl_func, \
    .init = name##_unit_init, \
    .exit = name##_unit_exit, \
}
```

This provides a uniform interface for all modules:
- `*_unit_init()` - Module initialization
- `*_unit_exit()` - Module cleanup
- `*_unit_ioctl_func()` - IOCTL command handler

### IOCTL Routing

The main entry point routes IOCTL commands to the appropriate module based on the `cmd` field in `struct ioctl_ksdata`.

## Key Features

1. **Modular Design** - Each subsystem has its own module
2. **Unified IOCTL Interface** - Single device node for all operations
3. **Extensible** - Easy to add new diagnostic modules
4. **Fault Injection** - Comprehensive fault injection framework
5. **Tracing Support** - Kprobe-based function tracing
6. **Runtime Diagnostics** - Live kernel state inspection

## Dependencies

- Linux kernel headers
- GCC for userspace tool
- Kernel build system for module compilation

## License

GPL
