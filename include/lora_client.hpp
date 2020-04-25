#ifndef __LORA_CLIENT_HPP
#define __LORA_CLIENT_HPP


#include <bcm2835.h>
#include <unistd.h>

#include <packet.hpp>

#define BOARD_DRAGINO_PIHAT
#include <RasPiBoards.h>
#include <client.hpp>
#include <RH_RF95.h>




#define DEVICE_ID                       2
#define REPLY_TIMEOUT                   1
#define ACTIVITY_DETECTION_TIMEOUT_MS   0
#define DATA_SEND_INTERVAL              (6 * 1000)


#define DATA_RECEIVER_PUBLIC_KEY        "2373b4ce20e158ce43811df6ce3a71028649ec21b3b9737a45f04b84985c3652"
#define DATA_RECEIVER_SIGN_KEY          "e58ae89e63828740a57f1e9953145bc3cdbc6e4912c99a5ed97af65f6331e580"

#define PUBLIC_KEY                      "A5C00B0F882A2570BB87B6E0477DF0FE922A9031F0D42F6B92F29985AD257A2E"
#define PRIVATE_KEY                     "8603D008C35798AC03559839773B94346D3F42AF11A68F7F843197F73FE16063"

#define SIGN_PUBLIC_KEY                 "EDB41C98A9946FFE98F6D6FB10B8AF28EE73FCF92ACCE2EC64BF1C72AF07971D"
#define SIGN_PRIVATE_KEY                "86E16FA291865AEC259F80C3FB6F495340EE632F3831CCB0B0665AABED49125AEDB41C98A9946FFE98F6D6FB10B8AF28EE73FCF92ACCE2EC64BF1C72AF07971D"

#define BLOCKCHAINSEC_CONFIG_F          "blockchainsec.conf"
#define RF_FREQUENCY                    915.10

//#define VERBOSE


void encryptData(struct packet_data *pd, const char* data, uint8_t dataLen);
void signPacket(struct packet *p);
void transmitData(struct packet *p);
void processReply(struct packet *p);
bool sendData(const char *data, uint8_t dataLen);
void generateKeys(void);
bool generateSharedKeys(void);


#endif //__LORA_CLIENT_HPP
