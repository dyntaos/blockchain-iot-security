#ifndef __PACKET_HPP
#define __PACKET_HPP

#include <cstdint>
#include <sodium.h>

#define PACKET_TYPE_ACK					0
#define PACKET_TYPE_DATA_ONLY			1
#define PACKET_TYPE_DATA_FIRST			2
#define PACKET_TYPE_DATA_INTERMEDIARY	3
#define PACKET_TYPE_DATA_LAST			4
#define PACKET_TYPE_UPDATE_KEY			5
#define PACKET_TYPE_GET_RECEIVER_KEY	6
//#define PACKET_TYPE_GET_MY_DEVICE_ID	7


struct packet_ack;
struct packet_data;
struct packet_data_sequential;
struct packet_update_key;
struct packet_get_receiver_key;
//struct packet_get_my_device_id;

struct packet_header {
	uint8_t       id;
	uint32_t      to;
	uint32_t      from;
	uint8_t       flags;
	uint8_t       len;
	unsigned char signature[crypto_sign_BYTES]; // 64 Bytes
};


struct packet_data {
	unsigned char crypto_nonce[crypto_stream_xchacha20_NONCEBYTES]; // 24 Bytes
	uint8_t *data;
};

struct packet_data_sequential {
	uint8_t *data;
};

struct packet_update_key {

};


#endif // __PACKET_HPP