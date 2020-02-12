#ifndef __LORA_PACKET_HPP
#define __LORA_PACKET_HPP


#define LORA_MSG_ACK				1

#define LORA_MSG_DATA				2
#define LORA_MSG_UPDATE_KEY			3

#define LORA_MSG_GET_RECEIVER_KEY	4
#define LORA_MSG_GET_MY_DEVICE_ID	5


typedef uint8_t lora_message_type_t;
typedef uint32_t lora_mac_t;


struct lora_packet {
	lora_message_type_t message_type; // rf95.setHeaderFlags(uint8_t)
	lora_mac_t mac; // device_id?
	uint8_t message_sequence; // rf95.setHeaderId(uint8_t)
	uint8_t payload_bytes;
	//uint8_t split_packet_total;
	uint32_t signature;
	uint16_t checksum; // Not needed?
}


#endif // __LORA_PACKET_HP