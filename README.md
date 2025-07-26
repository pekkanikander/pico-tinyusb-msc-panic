# TinyUSB RP2040/RP2350 MSC Endpoint Double-Arming Bug Reproduction

This repository contains a minimal reproduction of a bug that may
surface when TinyUSB's Mass Storage Class (MSC) implementation
is used on a the Raspberry Pi Pico (RP2040/RP2350).
The bug manifests as a panic or device disconnect when the device
attempts to start a new USB transfer on an endpoint.

## Quick Start

### Prerequisites
- Raspberry Pi Pico (RP2040) or Pico 2 (RP2350)
- Pico SDK 2.11 or later
- macOS (for reliable reproduction)
- Python 3.6+ (optional)

### Building and Flashing

1. **Clone this repository with submodules:**
   ```bash
   git clone --recursive https://github.com/pekkanikander/pico-tinyusb-msc-panic.git
   cd pico-tinyusb-msc-panic
   ```

2. **Build the firmware:**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. **Flash to your Pico:**
   ```bash
   cp pico-tinyusb-msc-panic.uf2 /Volumes/RP2350/
   ```

### Reproducing the Bug

**Method 1: Automatic (Recommended)**

Simply flash the Pico.
The device will appear as a USB mass storage device,
and macOS will attempt to read the boot sector.
This should trigger the bug immediately,
causing the device to panic and disconnect.

**Method 2: Manual**

If the automatic method doesn't work, run the provided Python script:
```bash
python3 trigger.py
```

**Expected Result:**

I'm not sure. But at minimum there shouldn't be a panic.

**Actual Result:**

- The device will panic/crash and disconnect from USB
- You may see panic messages on the UART (if connected):
```
heartbeat
WARN: starting new transfer on already active ep 83

*** PANIC ***

ep 83 was already available
```
- The device will become unresponsive and require a reset

## Technical Details

### The Bug

The specific bug occurs with TinyUSB's MSC implementation when handling
MSC SCSI READ10 commands on the RP2040 or RP2350.
The root cause appears to be how the SCSI layer handles so-called
short reads (which the device should not do) and that
the RP2040 port does not support packet queueing.

Specifically:

1. **Root Cause**

TinyUSB schedules the next IN transfer optimistically
before the previous one has a chance to completes,
without checking if the RP2040 hardware buffer is
truly available for re-arming.

2. **Condition**:

When a piece of device callback at `tud_msc_read10_cb` returns
a partial read (e.g., `bufsize-1` bytes),
TinyUSB schedules the next transfer via `tud_task`
but the hardware may still consider the endpoint busy.

3. **Driver Limitation**

The RP2040 port driver does not handle nicely a situation
where code called from `tud_task` tries to queue a on a
still-busy IN transfer.

### Why MSC Partial Reads Trigger the Bug

The triggering condition occurs in the MSC class driver's `proc_read10_cmd()` function.
Here's the sequence:

1. **Initial READ10 Command**: Host sends a SCSI READ10 command for multiple blocks
2. **First Transfer**: `proc_read10_cmd()` calls `tud_msc_read10_cb()` which returns `bufsize-1` (partial read)
3. **Transfer Scheduling**: `proc_read_io_data()` calls `usbd_edpt_xfer()` to send the partial data
4. **Transfer Completion**: When the transfer completes, `mscd_xfer_cb()` is called in the DATA stage
5. **Race Condition**: The completion handler checks if more data is needed:
   ```c
   if (p_msc->xferred_len >= p_msc->total_len) {
     // Data Stage is complete
     p_msc->stage = MSC_STAGE_STATUS;
   } else {
     proc_read10_cmd(p_msc);  // ← This schedules the NEXT transfer immediately
   }
   ```
6. **Immediate Re-scheduling**: Since `xferred_len < total_len` (due to partial read),
   `proc_read10_cmd()` is called again immediately.
7. **Double-Arming**: This schedules another transfer on the same endpoint
   before the hardware has finished processing the previous one

The key issue is that `proc_read10_cmd()` is called from the transfer completion handler (`mscd_xfer_cb()`).
This creates a tight loop where:

- Transfer completes → `mscd_xfer_cb()` → `proc_read10_cmd()` → `usbd_edpt_xfer()` → Hardware still busy → Panic

This condition is particularly problematic on the RP2040 because:
- The hardware buffer state is not immediately synchronized with the software state
- The driver's optimistic scheduling assumes the endpoint will be available
- The hardware panic occurs when the buffer control register indicates the endpoint is still busy

### Summarising trigger Conditions

The bug is triggered when:
- The device implements `tud_msc_read10_cb` to return partial reads
- The main loop calls `tud_task()` frequently (short or no delays)
- The host issues SCSI READ10 commands that trigger partial reads

In this reproduction, the trigger is simplified to:
- Always return `bufsize-1` from `tud_msc_read10_cb`
- Call `tud_task()` in a tight loop
- Let macOS read the boot sector during device enumeration
  or use a separate Python script to read

### Panic Call Path

```
Host SCSI READ10 → tud_msc_read10_cb() → return bufsize-1
                ↓
TinyUSB schedules next transfer: tud_task() → ... → hw_endpoint_xfer_start()
                ↓
"WARN: starting new transfer on already active ep 83"
                ↓
Panic "ep 83 was already available" → Device Disconnect
```

#### Call path details

* [usbd.h:L69 tud_task() → tud_task_ext()](https://github.com/hathach/tinyusb/blob/master/src/device/usbd.h#L69)
* [usbd.c:L689 tud_task_ext() → driver->xfer_cb(...)](https://github.com/hathach/tinyusb/blob/master/src/device/usbd.c#L689)
* [msc_device.c:L471 mscd_xfer_cb() → proc_read10_cmd()](https://github.com/hathach/tinyusb/blob/master/src/class/msc/msc_device.c#L471)
* [msc_device.c:L827 proc_read10_cmd() → proc_read_io_data()](https://github.com/hathach/tinyusb/blob/master/src/class/msc/msc_device.c#L827)
* [msc_device.c:L834 proc_read_io_data() → usbd_edpt_xfer()](https://github.com/hathach/tinyusb/blob/master/src/class/msc/msc_device.c#L834)
* [usbd.c:L1412 usbd_edpt_xfer() → dcd_edpt_xfer()](https://github.com/hathach/tinyusb/blob/master/src/device/usbd.c#L1412)
* [dcd_rp2040.c:L530 dcd_edpt_xfer() → hw_endpoint_xfer()](https://github.com/hathach/tinyusb/blob/master/src/portable/raspberrypi/rp2040/dcd_rp2040.c#L530)
* [dcd_rp2040.c:L148 hw_endpoint_xfer() → hw_endpoint_xfer_start()](https://github.com/hathach/tinyusb/blob/master/src/portable/raspberrypi/rp2040/dcd_rp2040.c#L148)
* [rp2040_usb.c:L199 hw_endpoint_xfer_start() → WARNING](https://github.com/hathach/tinyusb/blob/master/src/portable/raspberrypi/rp2040/rp2040_usb.c#L199)
* [rp2040_usb.c:L216 hw_endpoint_xfer_start() → hw_endpoint_start_next_buffer()](https://github.com/hathach/tinyusb/blob/master/src/portable/raspberrypi/rp2040/rp2040_usb.c#L216)
* [rp2040_usb.c:L191 _hw_endpoint_start_next_buffer() → _hw_endpoint_buffer_control_set_value32()](https://github.com/hathach/tinyusb/blob/master/src/portable/raspberrypi/rp2040/rp2040_usb.c#L191)
* [rp2040_usb.h:L122 _hw_endpoint_buffer_control_set_value32() → _hw_endpoint_buffer_control_update32()](https://github.com/hathach/tinyusb/blob/master/src/portable/raspberrypi/rp2040/rp2040_usb.h#L122)
* [rp2040_usb.c:L108 _hw_endpoint_buffer_control_update32() → panic("ep %02X was already available")](https://github.com/hathach/tinyusb/blob/master/src/portable/raspberrypi/rp2040/rp2040_usb.c#L108)

At `hw_endpoint_xfer_start`, the driver first notices that the ep is active and gives a warning:
```c
if (ep->active) {
    // TODO: Is this acceptable for interrupt packets?
    TU_LOG(1, "WARN: starting new transfer on already active ep %02X\r\n", ep->ep_addr);
    hw_endpoint_reset_transfer(ep);
  }
```
then, a little bit later, at L216, it schedules the next transfer:
```c
    hw_endpoint_start_next_buffer(ep);
```
which in turn eventually calls `_hw_endpoint_buffer_control_update32()`,
which notices that the HW buffer flag is not consistent with its expectations and panics:
```c
      if (*ep->buffer_control & USB_BUF_CTRL_AVAIL) {
        panic("ep %02X was already available", ep->ep_addr);
      }
```

### Files Overview

- **`pico-tinyusb-msc-panic.c`**: Main firmware with TinyUSB initialization and tight task loop
- **`usb_msc_cb.c`**: MSC callbacks, including the critical `tud_msc_read10_cb` that returns partial reads
- **`usb_descriptors.c`**: USB descriptors for MSC device configuration
- **`tusb_config.h`**: TinyUSB configuration
- **`trigger.py`**: Host script for manual bug triggering
- **`CMakeLists.txt`**: Build configuration

### Key Code Sections to trigger the bug

**Partial Read Implementation:**
```c
int32_t tud_msc_read10_cb(uint8_t lun __unused,
                          uint32_t lba __unused,
                          uint32_t offset __unused,
                          void* buffer __unused,
                          uint32_t bufsize)
{
    return bufsize - 1;  // Partial read triggers endpoint double-arming bug
}
```

**Tight Task Loop:**
```c
while (true) {
    tud_task();  // Called as fast as possible
}
```

## Related Issues

This bug has been reported in the TinyUSB repository:
- [Issue #XXXX: RP2040 MSC endpoint double-arming panic](https://github.com/hathach/tinyusb/issues/XXXX)

## Contributing

If you find this bug in other contexts or have additional reproduction methods, please:
1. Test with this minimal reproduction first
2. Document any variations or additional trigger conditions
3. Consider contributing to the TinyUSB, fixing the underlying bugs

## License

This project is in the public domain. Feel free to use, modify, and distribute it as needed.

## References

- [TinyUSB GitHub Repository](https://github.com/hathach/tinyusb)
- [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [USB Mass Storage Class Specification](https://www.usb.org/sites/default/files/usbmassbulk_10.pdf)
