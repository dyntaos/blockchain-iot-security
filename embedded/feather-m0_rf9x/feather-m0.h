#ifndef __FEATHER_M0_H
#define __FEATHER_M0_H

#include <SPI.h>
#include <RH_RF95.h>

#include "device1.h"

#define BATT_VOLTAGE_PIN A7
#define RFM95_CS  8
#define RFM95_RST 4
#define RFM95_INT 3

#define RF95_FREQ 915.1


#define REPLY_TIMEOUT                2000
#define DATA_SEND_INTERVAL           (5 * 1000)


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