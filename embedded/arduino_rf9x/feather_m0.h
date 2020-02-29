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
#define PUBLIC_KEY                   "D6F948EC141B2F0888A7548E223422254C7BDDCCA111BAB57097BF683E3A8159"
#define PRIVATE_KEY                  "D4E1A84FEF8A6E450B9C9E07A5133B42CFE647896545FA111424B9B9DA04EAD3D6F948EC141B2F0888A7548E223422254C7BDDCCA111BAB57097BF683E3A8159"



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

#endif // __FEATHER_M0_H