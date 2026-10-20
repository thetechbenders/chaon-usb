CC ?= cc
CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -Imain
NATIVE_SRCS := main/chaon_script.c main/chaon_hid.c main/chaon_vm.c tests/test_native.c
VISUAL_SRCS := main/chaon_status.c main/chaon_sd_diag.c main/chaon_screen.c tests/test_visual.c
NATIVE_BIN := tests/chaon_tests
VISUAL_BIN := tests/chaon_visual_tests
CONTROL_SRCS := main/chaon_control.c tests/test_control.c
CONTROL_BIN := tests/chaon_control_tests
SD_SECTOR_SRCS := main/chaon_sd_sector.c tests/test_sd_sector.c
SD_SECTOR_BIN := tests/chaon_sd_sector_tests

SD_DIAG_SRCS := main/chaon_sd_diag.c main/chaon_status.c main/chaon_screen.c tests/test_sd_diag.c
SD_DIAG_BIN := tests/chaon_sd_diag_tests
FAT_ROOT_SRCS := main/chaon_fat_ro.c tests/test_fat_ro.c
FAT_ROOT_BIN := tests/chaon_fat_ro_tests
FAT_UI_SRCS := main/chaon_screen.c main/chaon_status.c main/chaon_sd_diag.c tests/test_fat_ui.c
FAT_UI_BIN := tests/chaon_fat_ui_tests
INSPECT_SRCS := main/chaon_fat_ro.c main/chaon_fat_file.c main/chaon_inspect.c main/chaon_script.c tests/test_inspection.c
INSPECT_BIN := tests/chaon_script_inspect_tests

.PHONY: test clean
test: $(NATIVE_BIN) $(VISUAL_BIN) $(CONTROL_BIN) $(SD_SECTOR_BIN) $(SD_DIAG_BIN) $(FAT_ROOT_BIN) $(FAT_UI_BIN) $(INSPECT_BIN)
	./$(NATIVE_BIN)
	./$(VISUAL_BIN)
	./$(CONTROL_BIN)
	./$(SD_SECTOR_BIN)
	./$(SD_DIAG_BIN)
	./$(FAT_ROOT_BIN)
	./$(FAT_UI_BIN)
	./$(INSPECT_BIN)

$(NATIVE_BIN): $(NATIVE_SRCS) main/chaon_script.h main/chaon_hid.h main/chaon_vm.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(NATIVE_SRCS) -o $(NATIVE_BIN)

$(VISUAL_BIN): $(VISUAL_SRCS) main/chaon_status.h main/chaon_screen.h main/chaon_sd_diag.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(VISUAL_SRCS) -o $(VISUAL_BIN)

$(CONTROL_BIN): $(CONTROL_SRCS) main/chaon_control.h main/chaon_status.h main/chaon_vm.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CONTROL_SRCS) -o $(CONTROL_BIN)

$(SD_SECTOR_BIN): $(SD_SECTOR_SRCS) main/chaon_sd_sector.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SD_SECTOR_SRCS) -o $(SD_SECTOR_BIN)

$(SD_DIAG_BIN): $(SD_DIAG_SRCS) main/chaon_sd_diag.h main/chaon_screen.h main/chaon_status.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SD_DIAG_SRCS) -o $(SD_DIAG_BIN)


$(FAT_ROOT_BIN): $(FAT_ROOT_SRCS) main/chaon_fat_ro.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(FAT_ROOT_SRCS) -o $(FAT_ROOT_BIN)


$(FAT_UI_BIN): $(FAT_UI_SRCS) main/chaon_screen.h main/chaon_fat_ro.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(FAT_UI_SRCS) -o $(FAT_UI_BIN)

$(INSPECT_BIN): $(INSPECT_SRCS) main/chaon_inspect.h main/chaon_fat_ro.h main/chaon_script.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INSPECT_SRCS) -o $(INSPECT_BIN)

clean:
	rm -f $(NATIVE_BIN) $(VISUAL_BIN) $(CONTROL_BIN) $(SD_SECTOR_BIN) $(SD_DIAG_BIN) $(FAT_ROOT_BIN) $(FAT_UI_BIN) $(INSPECT_BIN)
