#ifndef __FEATHER_M0_H
#define __FEATHER_M0_H

#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

#define RF95_FREQ 915.1


#define DEVICE_ID                    10
#define REPLY_TIMEOUT                2000
#define DATA_SEND_INTERVAL           (5 * 1000)

#define DATA_RECEIVER_PUBLIC_KEY     ""
#define PUBLIC_KEY                   "218C1B7978BED009F278B86C48396C2F413E09D0233556D8F6D8326842B9F81B"
#define PRIVATE_KEY                  "6AE0BFC4040C507194450C55D6713C3ED8B897BC5E25754582F87883F3634EEF"



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