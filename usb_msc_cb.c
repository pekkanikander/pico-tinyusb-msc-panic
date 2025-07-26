/**
 * @file usb_msc_cb.c
 * @brief Minimal TinyUSB MSC callbacks for bug reproduction
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <tusb.h>
#include <class/msc/msc.h>

#if CFG_TUD_MSC

// READ10 callback: Always return partial read to trigger the bug
int32_t tud_msc_read10_cb(uint8_t lun __unused,
                          uint32_t lba __unused,
                          uint32_t offset __unused,
                          void* buffer __unused,
                          uint32_t bufsize)
{
    return bufsize - 1;  // Partial read triggers endpoint double-arming bug
}

// SCSI Inquiry: return basic device info
void tud_msc_inquiry_cb(uint8_t lun __unused,
                        uint8_t vendor_id[8],
                        uint8_t product_id[16],
                        uint8_t product_rev[4])
{
    memcpy(vendor_id,  "Raspberry",    8);
    memcpy(product_id, "Pico MSC BUG\0\0\0",16);
    memcpy(product_rev,"0.1 ",         4);
}

// Capacity callback: return block count and block size
void tud_msc_capacity_cb(uint8_t lun __unused,
                         uint32_t* block_count,
                         uint16_t* block_size)
{
    *block_count = 1024;  // 512KB total size
    *block_size  = 512;
}

// Test Unit Ready: always ready
bool tud_msc_test_unit_ready_cb(uint8_t lun __unused)
{
    return true;
}

// Write10: always fail (read-only)
int32_t tud_msc_write10_cb(uint8_t lun __unused,
                           uint32_t lba __unused,
                           uint32_t offset __unused,
                           uint8_t* buffer __unused,
                           uint32_t bufsize __unused)
{
    return TUD_MSC_RET_ERROR;
}

// Prevent/Allow Medium Removal: always succeed
bool tud_msc_prevent_allow_medium_removal_cb(uint8_t lun __unused,
                                            uint8_t prevent __unused,
                                            uint8_t control __unused)
{
    return true;
}

// Start/Stop: always succeed
bool tud_msc_start_stop_cb(uint8_t lun __unused,
                           uint8_t power_condition __unused,
                           bool start __unused,
                           bool load_eject __unused)
{
    return true;
}

// Is writable: always false (read-only)
bool tud_msc_is_writable_cb(uint8_t lun __unused) {
    return false;
}

// Generic SCSI command handler - return -1 for unsupported commands
int32_t tud_msc_scsi_cb(uint8_t lun __unused,
                        uint8_t const scsi_cmd[16] __unused,
                        void* buffer __unused,
                        uint16_t bufsize __unused)
{
    return -1;  // Command not supported
}

#endif // CFG_TUD_MSC
