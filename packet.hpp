#ifndef __PACKET_HPP
#define __PACKET_HPP

#include <cstdint>
#include <sodium.h>
#include <RH_RF95.h>

#define PACKET_TYPE_ACK					0
#define PACKET_TYPE_DATA_ONLY			1
#define PACKET_TYPE_DATA_FIRST			2
#define PACKET_TYPE_DATA_INTERMEDIARY	3
#define PACKET_TYPE_DATA_LAST			4
#define PACKET_TYPE_UPDATE_KEY			5
#define PACKET_TYPE_GET_RECEIVER_KEY	6
//#define PACKET_TYPE_GET_MY_DEVICE_ID	7


struct packet_data {
	unsigned char signature[crypto_sign_BYTES]; // 64 Bytes
	unsigned char crypto_nonce[crypto_stream_xchacha20_NONCEBYTES]; // 24 Bytes
	uint8_t       data[RH_RF95_MAX_MESSAGE_LEN - crypto_stream_xchacha20_NONCEBYTES]; // 255 - 11 - 64 - 24 = 156 Bytes
};

struct packet_data_sequential {
	uint8_t       data[RH_RF95_MAX_MESSAGE_LEN]; // 244 Bytes
};

struct packet_update_key {
	uint8_t       data[RH_RF95_MAX_MESSAGE_LEN]; // TODO
};


struct packet {
	uint32_t      to;
	uint32_t      from;
	uint8_t       id;
	uint8_t       fragment;
	uint8_t       flags;
	uint8_t       rssi;
	uint8_t       len;
	union {
		uint8_t                        bytes[RH_RF95_MAX_MESSAGE_LEN]; // 244 Bytes
		struct packet_data             data;
		struct packet_data_sequential  data_sequential;
		struct packet_update_key       update_key;
		struct packet_get_my_device_id get_my_device_id;
	} payload;
};



#endif // __PACKET_HPP