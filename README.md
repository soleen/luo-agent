# Live Update Orchestrator Agent (luo-agent)

**luo-agent** provides the userspace components required to drive the Linux
Kernel Live Update Orchestrator (LUO) subsystem. It consists of the system
daemon (`luod`) and the administrative control utility (`luoctl`).

## Overview

LUO allows a running Linux system to transition to a new kernel version via
`kexec` while preserving the state of specific applications (such as VMMs,
databases, or container runtimes) in RAM.

**luo-agent** acts as the broker for this process. It implements a
**Delegation Architecture**:
1.  **luod** creates kernel sessions via `/dev/liveupdate`.
2.  **luod** delegates Session File Descriptors (FDs) to clients via Unix Domain
    Sockets.
3.  **Clients** preserve their specific resources (memfds, VFIO state) into
    those sessions.
4.  **luod** orchestrates the final kexec transition.

## Components

### `luod` (Daemon)
The singleton system daemon that:
*   Enforces exclusive ownership of `/dev/liveupdate`.
*   Manages the lifecycle of LUO sessions.
*   Loads and verifies the target kernel image.
*   Coordinates the synchronization barrier before reboot.
*   Acts as the custodian of preserved sessions during the reboot transition.

### `luoctl` (CLI)
The administrative command-line interface used to:
*   Load the next kernel image.
*   Trigger the Live Update workflow.
*   Inspect active sessions and subscriptions.

## Prerequisites

*   **Linux Kernel:** 6.19+ with `CONFIG_LIVEUPDATE` enabled.
*   **Build Tools:** `meson`, `ninja`, `gcc` (or `clang`).
*   **Libraries:** `libjson-c`.

## Build & Install

```bash
# 1. Install dependencies (Debian/Ubuntu)
sudo apt-get install meson ninja-build libjson-c-dev

# 2. Configure build
meson setup build

# 3. Compile
ninja -C build

# 4. Install (Default: /usr/sbin/luod, /usr/bin/luoctl)
sudo ninja -C build install
```

## Usage

### 1. Start the Daemon
`luod` is designed to run as a systemd service. It requires specific
configuration to survive the shutdown sequence.

```bash
sudo systemctl enable --now luod
```

### 2. Load the Next Kernel
Before an update can occur, the new kernel image must be loaded into memory.

```bash
# Load kernel using the current system's command line
sudo luoctl load /boot/vmlinuz --initrd /boot/initrd.img --reuse-cmdline

# OR specify a custom command line
sudo luoctl load /boot/vmlinuz --cmdline "console=ttyS0 root=/dev/sda1"
```

### 3. Client Subscription
Clients (e.g., QEMU) must connect to `/run/luod/liveupdate.sock` and issue a
`SUBSCRIBE` command.
Check connected clients:

```bash
sudo luoctl list
```

### 4. Trigger Live Update
This command signals `luod` to create sessions, push FDs to clients, wait for
clients to become `READY`, and finally execute the `kexec` reboot.

```bash
sudo luoctl kexec
```

## Client Integration

Clients communicate with `luod` via a Unix Domain Socket at
`/run/luod/liveupdate.sock`.

### Protocol Overview
The protocol uses newline-delimited JSON. File Descriptors are transferred using
`SCM_RIGHTS`.

1.  **Connect:** Open socket `SOCK_STREAM`.
2.  **Subscribe:** Send `{"cmd": "SUBSCRIBE", "id": "unique-id"}`.
3.  **Wait:** Block on read until `PRESERVATION_REQUEST` is received.
4.  **Preserve:**
    *   Receive `{"event": "START_PRESERVATION"}` + **Session FD** (ancillary
        data).
    *   Use `ioctl(SessionFD, LIVEUPDATE_SESSION_PRESERVE_FD, ...)` to save
        resources.
5.  **Signal Ready:** Send `{"cmd": "READY"}`.
6.  **Reboot Happens...**
7.  **Reconnect:** Connect to socket.
8.  **Claim:** Send `{"cmd": "CLAIM", "id": "unique-id"}`.
9.  **Restore:**
    *   Receive **Session FD** (ancillary data).
    *   Use `ioctl(SessionFD, LIVEUPDATE_SESSION_RETRIEVE_FD, ...)` to restore
        resources.
    *   Call `ioctl(SessionFD, LIVEUPDATE_SESSION_FINISH)`.
