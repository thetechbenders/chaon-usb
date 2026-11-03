"""Compile the *generated* SDMMC post-write decision block with injected errors.

The test extracts the corrected SD-mode branch from the actual build-local
sdmmc_cmd.c and compiles it with a synthetic wait_for_idle provider.
It does not simulate SDMMC transactions, DMA or hardware timing.
"""
from pathlib import Path
import subprocess
import sys
import tempfile


START = "    /* SD mode: preserve the actual post-write status result."
END = "    /* SPI mode: although card busy indication"


def extract(source: str) -> str:
    if source.count(START) != 1 or source.count(END) != 1:
        raise ValueError("Generated N1 markers absent or duplicate")
    begin, end = source.index(START), source.index(END)
    if begin >= end:
        raise ValueError("Generated N1 branch moved")
    block = source[begin:end]
    if block.count("return idle_err;") != 1:
        raise ValueError("Generated N1 branch does not propagate status error")
    return block


HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define ESP_OK 0
#define ESP_ERR_TIMEOUT 0x107
#define ESP_ERR_INVALID_RESPONSE 0x108
#define ESP_ERR_INVALID_CRC 0x109
#define ESP_LOGE(...) ((void)0)
#define TAG "test"
typedef int esp_err_t;
typedef struct { int unused; } sdmmc_card_t;
static esp_err_t simulated_idle_result;
static unsigned int idle_calls;
static bool host_is_spi(const sdmmc_card_t *card) { (void)card; return false; }
static esp_err_t sdmmc_wait_for_idle(sdmmc_card_t *card, uint32_t status)
{
    (void)card; (void)status;
    ++idle_calls;
    return simulated_idle_result;
}
static esp_err_t invoke_post_write(sdmmc_card_t *card)
{
    uint32_t status = 0;
    esp_err_t err = ESP_OK;
    (void)err;
'''

FOOTER = r'''
    return ESP_OK;
}
int main(void)
{
    sdmmc_card_t card = {0};
    const esp_err_t cases[] = {
        ESP_OK, ESP_ERR_TIMEOUT, ESP_ERR_INVALID_RESPONSE,
        ESP_ERR_INVALID_CRC, 0x1234
    };
    for (unsigned i=0; i<sizeof(cases)/sizeof(cases[0]); ++i) {
        simulated_idle_result = cases[i];
        assert(invoke_post_write(&card) == cases[i]);
    }
    assert(idle_calls == sizeof(cases)/sizeof(cases[0]));
    puts("Generated SDMMC N1 fault propagation PASS");
    return 0;
}
'''


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_generated_idf_sdmmc_n1.py build/chaon_sdmmc_cmd_n1.c")
    source = Path(sys.argv[1]).read_text()
    block = extract(source)
    with tempfile.TemporaryDirectory(prefix="chaon-sdmmc-n1-") as tmp:
        src = Path(tmp) / "n1_test.c"
        executable = Path(tmp) / "n1_test"
        src.write_text(HEADER + block + FOOTER)
        subprocess.run(["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                        "-Wpedantic", "-Werror", "-fsanitize=undefined",
                        "-fno-omit-frame-pointer", str(src), "-o",
                        str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
