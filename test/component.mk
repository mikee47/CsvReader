COMPONENT_DEPENDS := \
	SmingTest \
	CsvReader

# Don't need network
HOST_NETWORK_OPTIONS := --nonet
DISABLE_NETWORK := 1

.PHONY: execute
execute: flash run
