    #define MBED
    #define WOLFSSL_CMSIS_RTOS    
    #define WOLFSSL_USER_IO
    #define NO_WRITEV
    #define NO_DEV_RANDOM
    #define HAVE_ECC
    #define NO_SESSION_CACHE // For Small RAM
    //#define IGNORE_KEY_EXTENSIONS
    #define NO_WOLFSSL_DIR  
    #define DEBUG_WOLFSSL

    /* Options for Sample program */
    //#define WOLFSSL_NO_VERIFYSERVER
    //#define NO_FILESYSTEM
    #ifndef WOLFSSL_NO_VERIFYSERVER
        #define TIME_OVERRIDES
        #define XTIME time
        #define XGMTIME localtime
    #endif