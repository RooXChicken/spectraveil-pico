 #ifndef _TUSB_CONFIG_H_
 #define _TUSB_CONFIG_H_
 
 #ifdef __cplusplus
 extern "C" {
    #endif
    
    //--------------------------------------------------------------------+
    // Board Specific Configuration
    //--------------------------------------------------------------------+
    
    // RHPort number used for device can be defined by board.mk, default to port 0
    #ifndef BOARD_TUD_RHPORT
    #define BOARD_TUD_RHPORT      0
    #endif
    
    // RHPort max operational speed can defined by board.mk
    #ifndef BOARD_TUD_MAX_SPEED
    #define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
    #endif
    
    //--------------------------------------------------------------------
    // COMMON CONFIGURATION
    //--------------------------------------------------------------------
    
    // defined by compiler flags for flexibility
    #ifndef CFG_TUSB_MCU
    #error CFG_TUSB_MCU must be defined
    #endif
    
    #ifndef CFG_TUSB_OS
    #define CFG_TUSB_OS           OPT_OS_PICO
    #endif

    #define CFG_TUH_ENABLED     1
    #define CFG_TUH_RPI_PIO_USB 1
    
    #ifndef CFG_TUSB_DEBUG
    #define CFG_TUSB_DEBUG        0
    #endif
    
    // Enable Device stack
    #define CFG_TUD_ENABLED       1
    
    // Default is max speed that hardware controller could support with on-chip PHY
    #define CFG_TUD_MAX_SPEED     BOARD_TUD_MAX_SPEED
    
    /* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
    * Tinyusb use follows macros to declare transferring memory so that they can be put
    * into those specific section.
    * e.g
    * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
    * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
    */
    #ifndef CFG_TUSB_MEM_SECTION
    #define CFG_TUSB_MEM_SECTION
    #endif
    
    #ifndef CFG_TUSB_MEM_ALIGN
    #define CFG_TUSB_MEM_ALIGN          __attribute__ ((aligned(4)))
    #endif
    
    //--------------------------------------------------------------------
    // DEVICE CONFIGURATION
    //--------------------------------------------------------------------
    
    #ifndef CFG_TUD_ENDPOINT0_SIZE
    #define CFG_TUD_ENDPOINT0_SIZE    64
    #endif
    
    //------------- CLASS -------------//
    #define CFG_TUD_HID               1
    #define CFG_TUD_CDC               1
    #define CFG_TUD_MSC               0
    #define CFG_TUD_MIDI              0
    #define CFG_TUD_VENDOR            0
    
    // HID buffer size Should be sufficient to hold ID (if any) + Data
    #define CFG_TUD_HID_EP_BUFSIZE    16

    #define CFG_TUH_HUB                 1
    // max device support (excluding hub device)
    #define CFG_TUH_DEVICE_MAX          (CFG_TUH_HUB ? 4 : 1) // hub typically has 4 ports

    #define CFG_TUH_HID                  4
    #define CFG_TUH_HID_EPIN_BUFSIZE    64
    #define CFG_TUH_HID_EPOUT_BUFSIZE   64

    #define CFG_TUD_CDC_RX_BUFSIZE   256
    #define CFG_TUD_CDC_TX_BUFSIZE   256

    #define CFG_TUH_ENUMERATION_BUFSIZE 256
    #define BOARD_TUH_RHPORT 0

    // CDC Endpoint transfer buffer size, more is faster
    #define CFG_TUD_CDC_EP_BUFSIZE   64


    
    #ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
