#ifndef __FEATHER_M0_H
#define __FEATHER_M0_H

#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

#define RF95_FREQ 915.1


#define DEVICE_ID                    2
#define REPLY_TIMEOUT                2000
#define DATA_SEND_INTERVAL           (5 * 1000)


#define DATA_RECEIVER_PUBLIC_KEY     "AF7277BE41770B93DEDEE8806DAC6B116EFB878DE51C4E13BDBDF8716AEED1CF"

#define PUBLIC_KEY                   "A5C00B0F882A2570BB87B6E0477DF0FE922A9031F0D42F6B92F29985AD257A2E"
#define PRIVATE_KEY                  "8603D008C35798AC03559839773B94346D3F42AF11A68F7F843197F73FE16063"

#define SIGN_PUBLIC_KEY              "EDB41C98A9946FFE98F6D6FB10B8AF28EE73FCF92ACCE2EC64BF1C72AF07971D"
#define SIGN_PRIVATE_KEY             "86E16FA291865AEC259F80C3FB6F495340EE632F3831CCB0B0665AABED49125AEDB41C98A9946FFE98F6D6FB10B8AF28EE73FCF92ACCE2EC64BF1C72AF07971D"



void encryptData(struct packet_data *pd, char* data, uint8_t dataLen);
void signPacket(struct packet *p);
void transmitData(struct packet *p);
void processReply(struct packet *p);
bool sendData(char *data, uint8_t dataLen);
void generateKeys(void);
bool generateSharedKeys(void);
void customRandSeed(void);
uint32_t customRand(void);
const char *customRandName(void);
void customRandBytes(void *buf, const size_t size);


#endif //__FEATHER_M0_H