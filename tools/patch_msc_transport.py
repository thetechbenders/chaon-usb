"""Source-gated, build-local geometry validation of TinyUSB MSC READ10/WRITE10.

Only the pinned espressif/tinyusb 0.21.0~2 MSC source is supported. No
managed component edits. This is an application-specific transport policy:
single logical unit, 512-byte sectors, no partial or out-of-range commands.

The original source is checked against a reviewed exact SHA-256 before patching.
"""
from pathlib import Path
import hashlib
import sys

# Filled after independent source digest verification.
SOURCE_SHA256 = "9bba988f5c1e83db1d565e55e934a8e9dd7f5e5085820b6c555ffa1484473ac5"


def replace_once(source: str, before: str, after: str) -> str:
    matches = source.count(before)
    if matches != 1:
        raise ValueError(f"MSC transport anchor drift: {before[:75]!r}, count={matches}")
    return source.replace(before, after, 1)


def patch_source(source: bytes, expected_sha: str = SOURCE_SHA256) -> str:
    digest = hashlib.sha256(source).hexdigest()
    if digest != expected_sha:
        raise ValueError(f"Pinned TinyUSB MSC source changed: {digest}")
    text = source.decode("utf-8")

    anchor = """static uint8_t rdwr10_validate_cmd(msc_cbw_t const* cbw) {
  uint8_t status = MSC_CSW_STATUS_PASSED;
  uint16_t const block_count = rdwr10_get_blockcount(cbw);
"""
    replacement = anchor + """
  /* ChaoN: reject incoherent host CBW geometry before any SD I/O. */
  uint32_t media_sectors = 0;
  uint16_t media_sector_size = 0;
  tud_msc_capacity_cb(cbw->lun, &media_sectors, &media_sector_size);
  uint32_t const lba = rdwr10_get_lba(cbw->command);
  if (media_sector_size != 512u || media_sectors == 0u) {
    TU_LOG_DRV("  ChaoN: READ10/WRITE10 media not ready\\r\\n");
    (void) tud_msc_set_sense(cbw->lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return MSC_CSW_STATUS_FAILED;
  }
  /* Avoid overflow by checking the input count against remaining capacity. */
  if (block_count != 0u && (lba >= media_sectors ||
       (uint32_t)block_count > media_sectors - lba)) {
    TU_LOG_DRV("  ChaoN: READ10/WRITE10 exceeds media capacity\\r\\n");
    (void) tud_msc_set_sense(cbw->lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
    return MSC_CSW_STATUS_FAILED;
  }
  /* SCSI READ10 count=0 with zero CBW payload is a legitimate no-op. */
  if ((block_count == 0u && cbw->total_bytes != 0u) ||
      (block_count != 0u && cbw->total_bytes != (uint32_t)block_count * 512u)) {
    TU_LOG_DRV("  ChaoN: inconsistent READ10/WRITE10 CBW geometry\\r\\n");
    (void) tud_msc_set_sense(cbw->lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x24, 0x00);
    return MSC_CSW_STATUS_PHASE_ERROR;
  }
"""
    text = replace_once(text, anchor, replacement)
    if text.count("ChaoN: inconsistent READ10/WRITE10 CBW geometry") != 1:
        raise ValueError("MSC transport guard not inserted exactly once")
    return text


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: patch_msc_transport.py PINNED_MSC.c BUILD_COPY.c")
    Path(sys.argv[2]).write_text(patch_source(Path(sys.argv[1]).read_bytes()))
