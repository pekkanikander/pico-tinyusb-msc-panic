import os
import sys
import plistlib
import subprocess
import struct

def find_msc_disk():
    if sys.platform == "darwin":
        proc = subprocess.run(
            ["diskutil", "list", "-plist", "external", "physical"],
            check=True, stdout=subprocess.PIPE
        )
        doc = plistlib.loads(proc.stdout)
        whole = doc.get("WholeDisks", [])
        if not whole:
            raise IOError("No external physical disks found")
        disk_id = whole[0]
        device = f"/dev/rdisk{disk_id.lstrip('disk')}"
    elif sys.platform.startswith("linux"):
        proc = subprocess.run(
            ["lsblk", "-o", "NAME,TRAN"],
            check=True, stdout=subprocess.PIPE, text=True
        )
        lines = proc.stdout.strip().split('\n')[1:]
        for line in lines:
            parts = line.split()
            if len(parts) != 2:
                continue
            name, tran = parts
            if tran == "usb":
                device = f"/dev/{name}"
                break
        else:
            raise IOError("No USB disks found")
    else:
        raise NotImplementedError("This function only supports macOS and Linux")
    if not os.access(device, os.R_OK):
        raise PermissionError(f"Device {device} is not readable: permission denied")
    return device

def read_sector(device, lba, sector_size=512):
    with open(device, "rb") as f:
        f.seek(lba * sector_size)
        data = f.read(sector_size)
        print(f"Read LBA {lba}: {len(data)} bytes")
        return data

if __name__ == "__main__":
    device = find_msc_disk()
    with open(device, "rb") as f:
        bootsector = f.read(512)
    cluster_heap_offset = struct.unpack_from('<I', bootsector, 88)[0]
    print("ClusterHeapOffset LBA:", cluster_heap_offset)
    with open(device, "rb") as f:
        f.seek(cluster_heap_offset * 512)
        data = f.read(512)
        print(f"Read {len(data)} bytes from LBA {cluster_heap_offset}")
