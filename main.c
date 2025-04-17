#include "pico/stdlib.h"
#include <stdio.h>

#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/altcp.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"
#include "creds.h"

static uint8_t const keycode_to_ascii[128][2] =  { HID_KEYCODE_TO_ASCII };
static uint8_t const ascii_to_keycode[128][2] =  { HID_ASCII_TO_KEYCODE };

// void hid_task(void);
bool empty_or_char(char, char);
void send_str(char*);

void pico_set_led(bool);

static const char *cgi_send_str(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
	sleep_ms(1000); // temporary delay while i am testing on my own pc
	if(strcmp(pcParam[0], "str") == 0) {
		send_str(pcValue[0]);
	}

    return "";
}

static const tCGI cgi_handlers[] = {
	{
	  "/send_str",
	  cgi_send_str
	}
};

/*------------- MAIN -------------*/
int main(void)
{
	stdio_init_all();
	
	cyw43_arch_init();
	cyw43_arch_enable_sta_mode();
	
	pico_set_led(true);
	while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) { }	
	
	httpd_init();
	http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
	
	pico_set_led(false);
	
	// init device stack on configured roothub port
	board_init();
	tud_init(BOARD_TUD_RHPORT);
	
	if (board_init_after_tusb) {
		board_init_after_tusb();
	}
	
	while(true) {
		// update and poll USB
		tud_task();
	}
}

static void send_hid_key(uint8_t _key, uint8_t _modifier) {
	// skip if hid is not ready yet
	if(!tud_hid_ready()) {
		return;
	}
	
	uint8_t keycode[6] = { 0 };
	keycode[0] = _key;
	
	tud_hid_keyboard_report(REPORT_ID_KEYBOARD, _modifier, keycode);
	sleep_ms(5);
	tud_task();
}

void send_str(char* _keys) {
	int i = 0;
	while(_keys[i] != '\0') {
		pico_set_led(true);
		
		char _key = _keys[i];

		// handle special http characters
		if(_key == '%' && empty_or_char(_keys[i+1], '2') && empty_or_char(_keys[i+2], '0')) { // %20 -> space
			_key = ' ';
			i += 2;
		}
		else if(_key == '%' && empty_or_char(_keys[i+1], '2') && empty_or_char(_keys[i+2], '1')) { // %21 -> '
			_key = '\'';
			i += 2;
		}
		else if(_key == '%' && empty_or_char(_keys[i+1], '2') && empty_or_char(_keys[i+2], '2')) { // %22 -> "
			_key = '\"';
			i += 2;
		}
		else if(_key == '%' && empty_or_char(_keys[i+1], 'n') && empty_or_char(_keys[i+2], 'l')) { // %nl -> \n (newline)
			_key = '\n';
			i += 2;
		}

		uint8_t _keycode = ascii_to_keycode[(int)_key][1];

		send_hid_key(_keycode, ascii_to_keycode[(int)_key][0] == 1 ? KEYBOARD_MODIFIER_LEFTSHIFT : 0);
		send_hid_key(HID_KEY_NONE, 0);
		
		pico_set_led(false);
		i++;
	}
}

bool empty_or_char(char _char, char _comp) {
	return (_char != '\0' && _char == _comp);
}

// void hid_task(void) {
// 	// polling interval
// 	const uint32_t interval_ms = 10;
// 	static uint32_t start_ms = 0;
	
// 	static bool pressed = false;
	
// 	if ( board_millis() - start_ms < interval_ms) return; // not enough time
// 	start_ms += interval_ms;
	
// 	uint32_t const btn = board_button_read();
	
// 	if(btn) {
// 		if(!pressed) {
// 			pressed = true;
// 			send_str("bleh");
// 		}
// 	}
// 	else {
// 		pressed = false;
// 	}
// }

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
	(void) instance;
	(void) len;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
	// TODO not Implemented
	(void) instance;
	(void) report_id;
	(void) report_type;
	(void) buffer;
	(void) reqlen;
	
	return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
	(void) instance;
	
	// if (report_type == HID_REPORT_TYPE_OUTPUT)
	// {
	// 	// Set keyboard LED e.g Capslock, Numlock etc...
	// 	if (report_id == REPORT_ID_KEYBOARD)
	// 	{
	// 		// bufsize should be (at least) 1
	// 		if ( bufsize < 1 ) return;
	
	// 		uint8_t const kbd_leds = buffer[0];
	
	// 		if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
	// 		{
	// 			// Capslock On: disable blink, turn led on
	// 			blink_interval_ms = 0;
	// 			board_led_write(true);
	// 		}else
	// 		{
	// 			// Caplocks Off: back to normal blink
	// 			board_led_write(false);
	// 			blink_interval_ms = BLINK_MOUNTED;
	// 		}
	// 	}
	// }
}

void pico_set_led(bool led_on) {
	#if defined(PICO_DEFAULT_LED_PIN)
	// Just set the GPIO on or off
	gpio_put(PICO_DEFAULT_LED_PIN, led_on);
	#elif defined(CYW43_WL_GPIO_LED_PIN)
	// Ask the wifi "driver" to set the GPIO on or off
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
	#endif
}