CC ?= cc
CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -Imain
NATIVE_SRCS := main/chaon_script.c main/chaon_hid.c main/chaon_vm.c tests/test_native.c
VISUAL_SRCS := main/chaon_status.c main/chaon_sd_diag.c main/chaon_screen.c tests/test_visual.c
NATIVE_BIN := tests/chaon_tests
VISUAL_BIN := tests/chaon_visual_tests
CONTROL_SRCS := main/chaon_control.c tests/test_control.c
CONTROL_BIN := tests/chaon_control_tests
CONFIRM_SRCS := main/chaon_control.c tests/test_confirm_control.c
CONFIRM_BIN := tests/chaon_confirm_control_tests
AUTH_SRCS := main/chaon_exec_auth.c tests/test_exec_auth.c
AUTH_BIN := tests/chaon_exec_auth_tests
SELECTION_SRCS := main/chaon_selection.c tests/test_selection.c
SELECTION_BIN := tests/chaon_selection_tests
EXEC_SLOT_SRCS := main/chaon_execution_slot.c main/chaon_selection.c tests/test_execution_slot.c
EXEC_SLOT_BIN := tests/chaon_execution_slot_tests
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
MAILBOX_SRCS := main/chaon_inspect_mailbox.c tests/test_inspect_mailbox.c
MAILBOX_BIN := tests/chaon_inspect_mailbox_tests
STAGE_SRCS := main/chaon_program_stage.c main/chaon_inspect.c main/chaon_script.c tests/test_program_stage.c
STAGE_BIN := tests/chaon_program_stage_tests
SD_PROBE_SRCS := main/chaon_fat_ro.c main/chaon_fat_file.c main/chaon_inspect.c main/chaon_script.c main/chaon_inspect_mailbox.c main/chaon_program_stage.c main/chaon_sd_sector.c tests/test_sd_probe_inspection.c
SD_PROBE_BIN := tests/chaon_sd_probe_inspection_tests
SD_PROBE_CPPFLAGS := -Itests/sd_probe_stubs

.PHONY: test clean
test: $(NATIVE_BIN) $(VISUAL_BIN) $(CONTROL_BIN) $(CONFIRM_BIN) $(AUTH_BIN) $(SELECTION_BIN) $(EXEC_SLOT_BIN) $(SD_SECTOR_BIN) $(SD_DIAG_BIN) $(FAT_ROOT_BIN) $(FAT_UI_BIN) $(INSPECT_BIN) $(MAILBOX_BIN) $(STAGE_BIN) $(SD_PROBE_BIN)
	./$(NATIVE_BIN)
	./$(VISUAL_BIN)
	./$(CONTROL_BIN)
	./$(CONFIRM_BIN)
	./$(AUTH_BIN)
	./$(SELECTION_BIN)
	./$(EXEC_SLOT_BIN)
	./$(SD_SECTOR_BIN)
	./$(SD_DIAG_BIN)
	./$(FAT_ROOT_BIN)
	./$(FAT_UI_BIN)
	./$(INSPECT_BIN)
	./$(MAILBOX_BIN)
	./$(STAGE_BIN)
	./$(SD_PROBE_BIN)

$(NATIVE_BIN): $(NATIVE_SRCS) main/chaon_script.h main/chaon_hid.h main/chaon_vm.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(NATIVE_SRCS) -o $(NATIVE_BIN)

$(VISUAL_BIN): $(VISUAL_SRCS) main/chaon_status.h main/chaon_screen.h main/chaon_sd_diag.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(VISUAL_SRCS) -o $(VISUAL_BIN)

$(CONTROL_BIN): $(CONTROL_SRCS) main/chaon_control.h main/chaon_status.h main/chaon_vm.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CONTROL_SRCS) -o $(CONTROL_BIN)

$(CONFIRM_BIN): $(CONFIRM_SRCS) main/chaon_control.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CONFIRM_SRCS) -o $(CONFIRM_BIN)

$(AUTH_BIN): $(AUTH_SRCS) main/chaon_exec_auth.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(AUTH_SRCS) -o $(AUTH_BIN)

$(SELECTION_BIN): $(SELECTION_SRCS) main/chaon_selection.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SELECTION_SRCS) -o $(SELECTION_BIN)

$(EXEC_SLOT_BIN): $(EXEC_SLOT_SRCS) main/chaon_execution_slot.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(EXEC_SLOT_SRCS) -o $(EXEC_SLOT_BIN)

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

$(MAILBOX_BIN): $(MAILBOX_SRCS) main/chaon_inspect_mailbox.h main/chaon_inspect.h main/chaon_fat_ro.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(MAILBOX_SRCS) -o $(MAILBOX_BIN)

$(STAGE_BIN): $(STAGE_SRCS) main/chaon_program_stage.h main/chaon_inspect.h main/chaon_script.h main/chaon_fat_ro.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(STAGE_SRCS) -o $(STAGE_BIN)

$(SD_PROBE_BIN): $(SD_PROBE_SRCS) main/chaon_sd_probe.c main/chaon_sd_probe.h main/chaon_fat_ro.h main/chaon_inspect_mailbox.h main/chaon_program_stage.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SD_PROBE_CPPFLAGS) $(SD_PROBE_SRCS) -o $(SD_PROBE_BIN)

clean:
	rm -f $(NATIVE_BIN) $(VISUAL_BIN) $(CONTROL_BIN) $(CONFIRM_BIN) $(AUTH_BIN) $(SELECTION_BIN) $(EXEC_SLOT_BIN) $(SD_SECTOR_BIN) $(SD_DIAG_BIN) $(FAT_ROOT_BIN) $(FAT_UI_BIN) $(INSPECT_BIN) $(MAILBOX_BIN) $(STAGE_BIN) $(SD_PROBE_BIN)
