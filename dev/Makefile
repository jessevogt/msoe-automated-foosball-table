export FLU_ROOT_DIR = /root/code
export FLU_COMMON_DIR = ${FLU_ROOT_DIR}/Common
export FLU_COMMON_HEADERS = ${FLU_COMMON_DIR}/FLUDefs.h ${FLU_COMMON_DIR}/FLUTypes.h

export FLU_REL = ${FLU_ROOT_DIR}/release
export FLU_REL_LIB = ${FLU_REL}/lib
export FLU_REL_BIN = ${FLU_REL}/bin

export FLU_UI_DIR = ${FLU_ROOT_DIR}/FoosLogicUI
export FLU_RT_DIR = ${FLU_ROOT_DIR}/FoosLogicRT

export DEBUG = -D_DEBUG_

ui:
	mkdir -p ${FLU_REL_LIB}
	mkdir -p ${FLU_REL_BIN}
	cd ${FLU_UI_DIR} && ${MAKE}

rt:
	mkdir -p ${FLU_REL_LIB}
	mkdir -p ${FLU_REL_BIN}
	cd ${FLU_RT_DIR} && ${MAKE}

clean_all:
	rm -rf ${FLU_REL}
