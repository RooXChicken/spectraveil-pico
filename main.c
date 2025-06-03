#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>

#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/altcp.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"
#include "creds.h"

// conversion tables
static uint8_t const keycode_to_ascii[128][2] =  { HID_KEYCODE_TO_ASCII };
static uint8_t const ascii_to_keycode[128][2] =  { HID_ASCII_TO_KEYCODE };

#define BUFFER_SIZE 65535
static char script_buffer[BUFFER_SIZE];
static int script_pos = 0;

// void hid_task(void);
bool empty_or_char(char, char);

void cache_str(char*);
void send_cache(void);

void send_str(char*);

void pico_set_led(bool);

// extern void core1_main();

// handles http requests :D
static const char *cgi_send_str(int _index, int _paramater_count, char *_paramaters[], char *_values[]) {
	sleep_ms(1000);

	if(strcmp(_paramaters[0], "str") == 0) {
		cache_str(_values[0]);
	}
	else if(strcmp(_paramaters[0], "fin") == 0 && strcmp(_values[0], "true") == 0) {
		// sleep_ms(1000); // temporary delay while i am testing on my own pc
		send_cache();
	}

    return "index.html";
}

static const tCGI cgi_handlers[] = {
	{
	  "/send_str",
	  cgi_send_str
	}
};

int main(void) {
	// initialize pi
	stdio_init_all();
	
	cyw43_arch_init();
	cyw43_arch_enable_sta_mode();
	board_init();

	sleep_ms(10);

// 	multicore_reset_core1();
//   // all USB task run in core1
// 	multicore_launch_core1(core1_main);
	
	tud_init(0);

	// cyw43_gpio_set(&cyw43_state, 36, true);
	
	// turn led on so i know it is doing net setup
	pico_set_led(true);

	// init wifi
	while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) { }
	
	// initialize web server
	httpd_init();
	http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
	
	// complete net setup
	pico_set_led(false);

	// set system clock to multiple of 12MHz
	// set_sys_clock_khz(120000, true);
	
	// init device stack on configured roothub port
	
	if (board_init_after_tusb) {
		board_init_after_tusb();
	}
	
	bool btn_pressed = false;

	while(true) {
		// update and poll USB
		tud_task();
		// tud_cdc_write_flush();

		if(board_button_read()) {
			if(!btn_pressed) {
				btn_pressed = true;

				char* _ip = ip4addr_ntoa(netif_ip4_addr(netif_list));
				send_str(_ip);
			}
		}
		else {
			btn_pressed = false;
		}
		
		tud_cdc_write_flush();
	}
}

void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;

  char buf[64];
  uint32_t count = tud_cdc_read(buf, sizeof(buf));

  // TODO control LED on keyboard of host stack
  (void) count;
}


// sends a specific HID_<X> key and modifier
static void send_hid_key(uint8_t _key, uint8_t _modifier) {
	// skip if hid is not ready yet
	if(!tud_hid_ready()) {
		return;
	}
	
	uint8_t keycode[6] = { 0 };
	keycode[0] = _key;
	
	// send keystroke
	tud_hid_keyboard_report(REPORT_ID_KEYBOARD, _modifier, keycode);
	sleep_ms(6);

	// after 6ms, update the usb device
	tud_task();
}

void cache_str(char* _str) {
	int _str_length = 0;
	while(_str[_str_length] != '\0') {
		_str_length++;
	}

	char* _length;
	sprintf(_length, "%d", _str_length);

	// static int _pos = 0;
	// while(script_buffer[_pos] != '\0') {
	// 	_pos++;
	// 	if(_pos >= BUFFER_SIZE) {
	// 		return;
	// 	}
	// }

	for(int i = 0; i < _str_length; i++) {
		int _cache_pos = script_pos + i;
		if(_cache_pos >= BUFFER_SIZE) {
			return;
		}

		script_buffer[_cache_pos] = _str[i];
	}

	script_pos += _str_length;
}

void send_cache() {
	send_str(script_buffer);

	script_pos = 0;
	for(int i = 0; i < BUFFER_SIZE; i++) {
		script_buffer[i] = '\0';
	}
}

// sends a series of keys to the pico
void send_str(char* _keys) {
	uint8_t _mod = 0;

	int i = 0;
	while(_keys[i] != '\0') {
		pico_set_led(true);
		
		char _key = _keys[i];
		uint8_t _keycode;

		bool _ascii = true;
		bool _send = true;

		if(_key == '%') {
			// handle special characters
			char _control = _keys[i+1];
			if(_control == '\0') {
				continue;
			}

			if(_control == 'k') {
				switch(_keys[i+2]) {
					case 's':
						_key = ' ';
						break;
					case 'm':
						_key = '\"';
						break;
					case 'a':
						_key = '\'';
						break;
					case 'p':
						_key = '&';
						break;
					case 'f':
						_key = '/';
						break;
					case 'b':
						_key = '\\';
						break;
					case 'q':
						_key = '?';
						break;
					case 'c':
						_key = ':';
						break;
					case 'n':
						_keycode = HID_KEY_ENTER;
						_ascii = false;
						break;
				}

				i += 2;
			}
			else if(_control == 's') {
				char _msStr[8];
				int k = 0;
				while(_keys[i+2+k] != '}') {
					_msStr[k] = _keys[i+2+k];
					k++;
				}
				
				int _ms = atoi(_msStr);
				sleep_ms(_ms);
				
				i += 2+k;
				_send = false;
				_ascii = false;
			}
			else if(_control == 'c') {
				_ascii = false;
	
				int _state = _keys[i+3];
				uint8_t _mode = 0;
	
				_key = _keys[i+2];
				if(_key == '\0')
					continue;
	
				switch(_key) {
					case 'c':
						_keycode = HID_KEY_CONTROL_LEFT;
						_mode = KEYBOARD_MODIFIER_LEFTCTRL;
						break;
					case 'a':
						_keycode = HID_KEY_ALT_LEFT;
						_mode = KEYBOARD_MODIFIER_LEFTALT;
						break;
					case 's':
						_keycode = HID_KEY_SHIFT_LEFT;
						_mode = KEYBOARD_MODIFIER_LEFTSHIFT;
						break;
					case 'w':
						_keycode = HID_KEY_GUI_LEFT;
						_mode = KEYBOARD_MODIFIER_LEFTGUI;
						break;
					case 'd':
						_keycode = HID_KEY_DELETE;
						break;
						_keycode = HID_KEY_BACKSPACE;
						break;
					case 't':
						_keycode = HID_KEY_TAB;
						break;
				}
	
				if(_mode != 0) {
					if(_state != '0') {
						_send = false;
					}
	
					if(_state == '1') {
						_mod |= _mode;
					}
					else if(_state == '2') {
						_mod &= ~_mode;
					}
				}
	
				i += 3;
			}
		}

		if(_ascii) {
			// default case (ascii key)
			_keycode = ascii_to_keycode[(int)_key][1];
			if(ascii_to_keycode[(int)_key][0] == 1) {
				_mod |= KEYBOARD_MODIFIER_LEFTSHIFT;
			}
			else {
				_mod &= ~KEYBOARD_MODIFIER_LEFTSHIFT;
			}
		}

		if(_send) {
			send_hid_key(_keycode, _mod);
			send_hid_key(HID_KEY_NONE, 0);
		}
			
		pico_set_led(false);
		i++;
	}
}

// used to determine if a character isn't null terminated and is the correct character
bool empty_or_char(char _char, char _comp) {
	return (_char != '\0' && _char == _comp);
}

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
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
}