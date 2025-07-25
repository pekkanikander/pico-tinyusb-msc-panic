# Bootstrap Instructions: Reproducing the TinyUSB RP2040 MSC Endpoint Double-Arming Bug

## Context
- **Goal:** Create a minimal, public GitHub repository that demonstrates the "already active ep" (endpoint double-arming) bug in TinyUSB's Mass Storage Class (MSC) device implementation for the Raspberry Pi Pico (RP2040/RP2350).
- **Platform:** Raspberry Pi Pico (RP2040 or RP2350)
- **Firmware:** C or C++ using the Pico SDK and TinyUSB
- **Host Test Script:** Python script (e.g., `temp.py`) that reads raw sectors from the device

## Problem Summary
- The bug manifests as a panic or crash in the device firmware when the host issues repeated SCSI READ10 commands and the device attempts to start a new USB transfer on an endpoint that the hardware still considers busy.
- This is typically seen as a log message like "WARN: starting new transfer on already active ep" followed by a panic or device disconnect.
- The root cause is a mismatch between TinyUSB's optimistic transfer scheduling and the RP2040 hardware's inability to queue multiple IN transfers on the same endpoint.

## Key Technical Details
- **TinyUSB's MSC class** schedules the next transfer as soon as the previous one completes, without checking if the hardware buffer is truly available.
- **RP2040 USB hardware** requires that the buffer be fully consumed by the host before it can be re-armed; it does not support queuing multiple IN transfers.
- **The bug is most easily triggered** when the device returns a large partial read (e.g., `bufsize-1` bytes) in `tud_msc_read10_cb`, and the main loop calls `tud_task()` as fast as possible.
- **The host script** (e.g., `temp.py`) should repeatedly read sectors from the device, as this triggers the SCSI READ10 commands.

## Minimal Reproduction Plan
1. **Create a new Pico SDK project** with TinyUSB enabled and configured for MSC device class.
2. **Implement `tud_msc_read10_cb`** to always return a partial read (e.g., `bufsize-1` bytes, filled with dummy data).
3. **Main loop:** Call `tud_task()` in a tight loop (no delays).
4. **Host side:** Use a Python script (e.g., `temp.py`) to open the device as a raw block device and read sectors repeatedly.
5. **Expected result:** After a few reads, the device should log a warning about starting a new transfer on an already active endpoint, and may panic or disconnect.

## What to Provide to the Assistant
- The above context and goals.
- The Python host script (`temp.py`) for reading raw sectors.
- Any relevant error logs or device output.
- The specific Pico SDK/TinyUSB version if relevant.

## What to Ask the Assistant
- Help with:
  - Setting up the minimal Pico SDK + TinyUSB MSC project
  - Writing the correct `tud_msc_read10_cb` for partial reads
  - Ensuring the main loop is as fast as possible
  - Reviewing or improving the host Python script
  - Diagnosing and confirming the bug
  - Suggesting instrumentation or logging to confirm the race condition
  - Drafting a clear README and bug report for GitHub

## References
- [TinyUSB GitHub Issues: RP2040 endpoint panic](https://github.com/hathach/tinyusb/issues)
- [RP2040 Datasheet: USB Device Controller]
- [USB Mass Storage Class Bulk-Only Transport Spec]
