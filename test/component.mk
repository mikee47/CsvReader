COMPONENT_DEPENDS := \
	SmingTest \
	CsvReader

HWCONFIG := csvtest

# Don't need network
HOST_NETWORK_OPTIONS := --nonet
DISABLE_NETWORK := 1

.PHONY: execute
execute: flash run
