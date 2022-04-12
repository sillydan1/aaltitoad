execute_process(
        COMMAND git describe --long --always --dirty --exclude=* --abbrev=8
        RESULT_VARIABLE SHORT_HASH_RESULT
        OUTPUT_VARIABLE GIT_SLUG
        OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(${CONFIG_IN_FILE} ${CONFIG_OUT_FILE})
