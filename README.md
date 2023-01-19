# Snapshot safety with virtio-rng

This is a demo repo for demonstrating the new [virtio-rng](https://docs.oasis-open.org/virtio/virtio/v1.2/cs01/virtio-v1.2-cs01.html#x1-3050004)
proposed feature for "[entropy leak detection](https://www.mail-archive.com/virtio-dev@lists.oasis-open.org/msg09016.html)", designed to enable snapshot safety for Virtual Machines.

The demo includes a patch-set for Linux 6.1 with a PoC implementation of the new feature for the virtio-rng driver and
the corresponding [device implementation](https://github.com/bchalios/firecracker/tree/feat_snapsafety) on the [Firecracker Virtual Machine Monitor](https://github.com/firecracker-microvm/firecracker).
It also includes a set of user-space test applications for show-casing the implemented functionality.

## virtio-rng entropy leak reporting

The new feature introduces the concept of leak queues for the virtio-rng device. Leak queues are a mechanism for the device
to notify the guest kernel for "leaks" of entropy, which, for example, can happen when we take VM snapshot or restore a VM from
one.

The notification can be used from the guest for re-creating state that is supposed to be unique and or secret immediately after
snapshoting events. For example, the kernel might use the notification for re-seeding its RNG.

The Linux patch-set builds on top of the virtio feature to expose notification APIs to the guest user-space. It exposes a sysfs
file under `/sys/virtio-rng/<device-name>/vm_gen_counter` which allows `mmap` and `poll` and `read` operations. The file includes
a word-size unsigned integer which increases with every entropy leak event.

## Running the demo

We will launch a Firecracker uVM and start the `test_mmap` application. The application reads the sysfs file and caches the
value of the generation counter. Then, it periodically monitors the `mmap`ed memory and reports changes in the value.


### Preparation

We need to apply the patches from `patches` and build a Linux kernel and as well build the Firecracker binary from the [fork](https://github.com/bchalios/firecracker/tree/feat_snapsafety) that
implementations the new feature.

Otherwise, we can download pre-built binaries:

```shell
$ mkdir bin
$ wget https://s3.amazonaws.com/spec.ccfc.min/snapsafe_demo/firecracker-snapsafe -O bin/firecracker-snapsafe
$ wget https://s3.amazonaws.com/spec.ccfc.min/snapsafe_demo/vmlinux-6.1-snapsafe -O bin/vmlinux-6.1-snapsafe
```

### Launch Firecracker and start test
```
# Grant access to /dev/kvm for your user. In my distro, you can do that with file ACLs.
$ sudo setfacl -m u:${USER}:rw /dev/kvm

# Launch the Firecracker uVM
./bin/firecracker-snapsafe --api-sock /tmp/firecracker.sock --config-file share/fc_config.json
```

This should launch the uVM and give us a command prompt. Login the shell with using `root` both for login and password.
Login and start `test_mmap`:

```shell
f8fbc143a11c login: root
Password:
Last login: Thu Jan 19 11:32:51 on ttyS0
-bash-4.2# ./test_mmap /sys/virtio-rng/virtio_rng.0/vm_gen_counter
```

### Snapshot VM

From another terminal on the host machine take a snapshot of the Firecracker uVM:

```shell
# First we pause the microVM
$ curl --unix-socket /tmp/firecracker.sock -i \
    -X PATCH 'http://localhost/vm' \
    -H 'Accept: application/json' \
    -H 'Content-Type: application/json' \
    -d '{
            "state": "Paused"
    }'
HTTP/1.1 204
Server: Firecracker API
Connection: keep-aliv

# Then we take the snapshot
$ curl --unix-socket /tmp/firecracker.sock -i \
    -X PUT 'http://localhost/snapshot/create' \
    -H  'Accept: application/json' \
    -H  'Content-Type: application/json' \
    -d '{
            "snapshot_type": "Full",
            "snapshot_path": "./snapshot_file_2",
            "mem_file_path": "./mem_file_2"
    }'
HTTP/1.1 204
Server: Firecracker API
Connection: keep-alive

# Finally, resume the microVM
$ curl --unix-socket /tmp/firecracker.sock -i \
    -X PATCH 'http://localhost/vm' \
    -H 'Accept: application/json' \
    -H 'Content-Type: application/json' \
    -d '{
            "state": "Resumed"
    }'
HTTP/1.1 204
Server: Firecracker API
Connection: keep-alive
```

At this point, in the initial console `test_mmap` should report the change in the generation counter
```shell
bash-4.2# ./test_mmap /sys/virtio-rng/virtio_rng.0/vm_gen_counter
2023-01-19T11:52:17.230990483 [anonymous-instance:main:WARN:src/logger/src/lib.rs:36] [DevPreview] Virtual machine snapshots is in development preview.
VM generation counter changed! Old: 0 New: 1
```
