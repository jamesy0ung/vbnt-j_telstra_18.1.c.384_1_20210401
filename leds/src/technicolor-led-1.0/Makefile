
obj-${CONFIG_TECHNICOLOR_LED} += technicolor_led.o

SELECTED_OBJS := main.o aggr_leds.o

BOARD_LED_DEFINES_FILE :=

ifneq (,$(findstring ENABLE_SHIFT_LED_DRIVER,$(EXTRA_CFLAGS)))
	SELECTED_OBJS += sr_led.o
	BOARD_LED_DEFINES_FILE := board_led_defines.o
endif

ifneq (,$(findstring ENABLE_GPIO_LED_DRIVER,$(EXTRA_CFLAGS)))
	BOARD_LED_DEFINES_FILE := board_led_defines.o
endif

ifneq (,$(findstring ENABLE_BCM_DRIVER,$(EXTRA_CFLAGS)))
	SELECTED_OBJS += bcm_led.o
	BOARD_LED_DEFINES_FILE := board_led_defines.o
endif

technicolor_led-y := $(SELECTED_OBJS) $(BOARD_LED_DEFINES_FILE)

ccflags-y := -Idrivers/leds -Werror $(EXTRA_CFLAGS)

clean:
	rm -f core *.o *.a *.s *~

