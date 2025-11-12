# Process Execution Transitions: Executive Summary
The `/proc/<pid>/exe` symlink transitions through two distinct phases during process execution, not atomically as commonly assumed. This has direct implications for security policy enforcement systems that rely on process identity.

## What Actually Happens During exec
```
Step 1: Kernel validates new binary
  ├─ Opens /usr/bin/ls
  ├─ Reads ELF header
  ├─ Finds .interp section: /lib64/ld-linux-x86-64.so.2
  └─ Status: exe=/usr/bin/bash (unchanged)

Step 2: Kernel prepares dynamic linker
  ├─ Opens /lib64/ld-linux-x86-64.so.2
  ├─ Validates linker binary
  ├─ Still in "old" process context
  └─ Status: exe=/usr/bin/bash (unchanged)

Step 3: Kernel commits to new process
  ├─ begin_new_exec() - point of no return
  ├─ exec_mmap() - new memory space
  ├─ set_mm_exe_file(bprm->file)
  │  ▼
  │  mm->exe_file = /usr/bin/ls
  │  ▼
  └─ Status: exe=/usr/bin/ls (CHANGED!)

Step 4: Dynamic linker executes
  ├─ Opens /etc/ld.so.cache
  ├─ Resolves library dependencies
  ├─ Opens each required library
  ├─ Performs relocations
  └─ Status: exe=/usr/bin/ls (stable)

Step 5: Transfer to program
  ├─ Dynamic linker jumps to ls entry point
  └─ ls begins execution
```

## Tools Provided

- `fanotify_monitor` - Demonstrates the two-phase behavior
- Outputs in fapolicyd format
- Supports syslog for background monitoring
- Handles both regular processes and kernel threads

## Quick start
This contains a program that can help study how the kernel executes files. Just run make and then as root
```
 ./fanotify_monitor
fanotify monitor - fapolicyd format output
Usage: fanotify_monitor [--syslog] [mount_point]
Monitoring: /
Output: stdout

perm=execute pid=20737 ppid=19425 exe=/usr/bin/bash : path=/usr/bin/ls
perm=open pid=20737 ppid=19425 exe=/usr/bin/bash : path=/usr/bin/ls
perm=execute pid=20737 ppid=19425 exe=/usr/bin/bash : path=/usr/lib64/ld-linux-x86-64.so.2
perm=open pid=20737 ppid=19425 exe=/usr/bin/bash : path=/usr/lib64/ld-linux-x86-64.so.2
perm=open pid=20737 ppid=19425 exe=/usr/bin/ls : path=/etc/ld.so.cache
perm=open pid=20737 ppid=19425 exe=/usr/bin/ls : path=/usr/lib64/libselinux.so.1
perm=open pid=20737 ppid=19425 exe=/usr/bin/ls : path=/usr/lib64/libcap.so.2.73
perm=open pid=20737 ppid=19425 exe=/usr/bin/ls : path=/usr/lib64/libc.so.6
perm=open pid=20737 ppid=19425 exe=/usr/bin/ls : path=/usr/lib64/libpcre2-8.so.0.14.0
perm=open pid=20737 ppid=19425 exe=/usr/bin/ls : path=/usr/lib/locale/locale-archive
```

## How to reliably create the various forms of process execution

### Normal Execution
```
ls
```

### LD_PRELOAD
```
LD_PRELOAD=/usr/lib64/libaudit.so.1 ls
```

### Runtime Linker
```
/usr/lib64/ld-linux-x86-64.so.2 /usr/bin/ls
```

### Kworker thread
```
As root:
PATTERN=$(cat /proc/sys/kernel/core_pattern)
echo "|/usr/bin/cat > /tmp/core.%p" > /proc/sys/kernel/core_pattern
bash -c 'kill -SEGV $$'
echo $PATTERN > /proc/sys/kernel/core_pattern
```

Note: fanotify_monitor is unsupported

