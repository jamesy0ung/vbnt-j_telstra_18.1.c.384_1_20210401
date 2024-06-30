

obj-m += bankmgr.o 

bankmgr-y := bankmgr_proc.o main.o

ifneq ($(CONFIG_MTD_SPEEDTOUCH_BTAB),)
bankmgr-y += bankmgr_checksum.o storage_kernel.o storage_core.o storage_crc32.o bankmgr_rawstorage_core.o bankmgr_rawstorage_os.o legacy_upgrade_info.o
else
bankmgr-y += bankmgr_ubi.o
endif

clean:
	rm -f core *.o *.a *.s *~

TEST_BINARY := test/rs_host_test

.PHONY: ${TEST_BINARY}

run_test: ${TEST_BINARY}
	$^

${TEST_BINARY}:
	make -C test rs_host_test

