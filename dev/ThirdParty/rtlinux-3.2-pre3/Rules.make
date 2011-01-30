ifneq ($(wildcard .depend),)
include .depend
endif
ifneq ($(wildcard $(RTL_DIR)/.hdepend),)
include $(RTL_DIR)/.hdepend
endif
