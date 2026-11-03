"""Pin and correct ESP-IDF v5.5.3 SDMMC post-write status error propagation.

The original IDF source is verified by git-blob SHA1 (content-addressed),
and only a build-local copy is modified. For non-timeout CMD13/status polling
errors, propagate the error instead of returning the earlier ESP_OK.
"""
from pathlib import Path
import hashlib
import sys

UPSTREAM_GIT_BLOB_SHA1 = "a928dee6d2ecca70d3207a4f63f7352f3ef6e91b"

OLD = """    /* SD mode: wait for the card to become idle based on R1 status */
    if (!host_is_spi(card)) {
        switch (sdmmc_wait_for_idle(card, status)) {
            case ESP_OK:
                break;
            case ESP_ERR_TIMEOUT:
                ESP_LOGE(TAG, "%s: sdmmc_wait_for_idle timeout", __func__);
                return ESP_ERR_TIMEOUT;
            default:
                return err;
        }
    }
"""
NEW = """    /* SD mode: preserve the actual post-write status result. The upstream
     * default returned the earlier successful write-command err (ESP_OK)
     * even when CMD13/status polling failed. */
    if (!host_is_spi(card)) {
        esp_err_t const idle_err = sdmmc_wait_for_idle(card, status);
        if (idle_err != ESP_OK) {
            ESP_LOGE(TAG, "%s: post-write sdmmc_wait_for_idle failed: 0x%x",
                     __func__, idle_err);
            return idle_err;
        }
    }
"""


def git_blob_hash(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\x00" + data).hexdigest()


def patch_source(data: bytes, expected_blob: str = UPSTREAM_GIT_BLOB_SHA1) -> str:
    actual = git_blob_hash(data)
    if actual != expected_blob:
        raise ValueError(f"Pinned ESP-IDF SDMMC source changed: {actual}")
    text = data.decode("utf-8")
    if text.count(OLD) != 1:
        raise ValueError(f"Pinned SDMMC completion block changed: count={text.count(OLD)}")
    patched = text.replace(OLD, NEW, 1)
    if patched.count("return idle_err;") != 1 or OLD in patched:
        raise ValueError("SDMMC completion error propagation patch incomplete")
    return patched


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: patch_idf_sdmmc_n1.py ORIGINAL.c BUILD_COPY.c")
    Path(sys.argv[2]).write_text(patch_source(Path(sys.argv[1]).read_bytes()))
