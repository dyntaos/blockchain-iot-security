/*******************************************************************************
 *
 * Copyright (c) 2018 Dragino
 *
 * http://www.dragino.com
 * https://github.com/dragino/rpi-lora-tranceiver
 *
 *******************************************************************************/

#include <iostream>
#include <string>
#include <sys/types.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include <lora_trx.h>


using namespace std;



void LoraTrx::selectreceiver(void) {
	digitalWrite(ssPin, LOW);
}



void LoraTrx::unselectreceiver(void) {
	digitalWrite(ssPin, HIGH);
}



byte LoraTrx::readReg(byte addr) {
	unsigned char spibuf[2];

	selectreceiver();
	spibuf[0] = addr & 0x7F;
	spibuf[1] = 0x00;
	wiringPiSPIDataRW(CHANNEL, spibuf, 2);
	unselectreceiver();

	return spibuf[1];
}



void LoraTrx::writeReg(byte addr, byte value) {
	unsigned char spibuf[2];

	spibuf[0] = addr | 0x80;
	spibuf[1] = value;
	selectreceiver();
	wiringPiSPIDataRW(CHANNEL, spibuf, 2);

	unselectreceiver();
}



void LoraTrx::opmode (uint8_t mode) {
	writeReg(REG_OPMODE, (readReg(REG_OPMODE) & ~OPMODE_MASK) | mode);
}



void LoraTrx::opmodeLora(void) {
	uint8_t u = OPMODE_LORA;
	if (sx1272 == false) {
		u |= 0x8;   // TBD: sx1276 high freq
	}
	writeReg(REG_OPMODE, u);
}



LoraTrx::LoraTrx(void) {

	wiringPiSetup () ;
	pinMode(ssPin, OUTPUT);
	pinMode(dio0, INPUT);
	pinMode(RST, OUTPUT);

	wiringPiSPISetup(CHANNEL, 500000);

	digitalWrite(RST, HIGH);
	delay(100);
	digitalWrite(RST, LOW);
	delay(100);

	byte version = readReg(REG_VERSION);

	if (version == 0x22) {
		// sx1272
		cout << "SX1272 detected, starting." << endl;
		sx1272 = true;
	} else {
		// sx1276?
		digitalWrite(RST, LOW);
		delay(100);
		digitalWrite(RST, HIGH);
		delay(100);
		version = readReg(REG_VERSION);
		if (version == 0x12) {
			// sx1276
			cout << "SX1276 detected, starting." << endl;
			sx1272 = false;
		} else {
			cerr << "Unrecognized transceiver [version " << version << "]" << endl;
			exit(EXIT_FAILURE);
		}
	}

	opmode(OPMODE_SLEEP);

	// set frequency
	uint64_t frf = ((uint64_t)freq << 19) / 32000000;
	writeReg(REG_FRF_MSB, (uint8_t)(frf>>16) );
	writeReg(REG_FRF_MID, (uint8_t)(frf>> 8) );
	writeReg(REG_FRF_LSB, (uint8_t)(frf>> 0) );

	writeReg(REG_SYNC_WORD, 0x34); // LoRaWAN public sync word

	if (sx1272) {
		if (sf == SF11 || sf == SF12) {
			writeReg(REG_MODEM_CONFIG,0x0B);
		} else {
			writeReg(REG_MODEM_CONFIG,0x0A);
		}
		writeReg(REG_MODEM_CONFIG2,(sf<<4) | 0x04);
	} else {
		if (sf == SF11 || sf == SF12) {
			writeReg(REG_MODEM_CONFIG3,0x0C);
		} else {
			writeReg(REG_MODEM_CONFIG3,0x04);
		}
		writeReg(REG_MODEM_CONFIG,0x72);
		writeReg(REG_MODEM_CONFIG2,(sf<<4) | 0x04);
	}

	if (sf == SF10 || sf == SF11 || sf == SF12) {
		writeReg(REG_SYMB_TIMEOUT_LSB,0x05);
	} else {
		writeReg(REG_SYMB_TIMEOUT_LSB,0x08);
	}
	writeReg(REG_MAX_PAYLOAD_LENGTH,0x80);
	writeReg(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);
	writeReg(REG_HOP_PERIOD,0xFF);
	writeReg(REG_FIFO_ADDR_PTR, readReg(REG_FIFO_RX_BASE_AD));

	writeReg(REG_LNA, LNA_MAX_GAIN);

	opmodeLora();
	opmode(OPMODE_STANDBY);

	writeReg(RegPaRamp, (readReg(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
	configPower(23);
}



bool LoraTrx::receive(char *payload) {
	// clear rxDone
	writeReg(REG_IRQ_FLAGS, 0x40);

	int irqflags = readReg(REG_IRQ_FLAGS);

	//  payload crc: 0x20
	if((irqflags & 0x20) == 0x20) {
		cout << "CRC error" << endl;
		writeReg(REG_IRQ_FLAGS, 0x20);
		return false;
	} else {

		byte currentAddr = readReg(REG_FIFO_RX_CURRENT_ADDR);
		byte receivedCount = readReg(REG_RX_NB_BYTES);
		receivedbytes = receivedCount;

		writeReg(REG_FIFO_ADDR_PTR, currentAddr);

		for(int i = 0; i < receivedCount; i++) {
			payload[i] = (char)readReg(REG_FIFO);
		}
	}
	return true;
}



bool LoraTrx::receivepacket(string &msg, byte &len, byte &packet_rssi, byte &rssi, long int &snr) {

	//long int SNR;
	int rssicorr;

	if(digitalRead(dio0) == 1) {
		if(receive(message)) {
			byte value = readReg(REG_PKT_SNR_VALUE);
			if( value & 0x80 ) { // The SNR sign bit is 1
				// Invert and divide by 4
				value = ( ( ~value + 1 ) & 0xFF ) >> 2;
				snr = -value;
			} else {
				// Divide by 4
				snr = ( value & 0xFF ) >> 2;
			}

			if (sx1272) {
				rssicorr = 139;
			} else {
				rssicorr = 157;
			}

			msg = message;
			len = (unsigned int) receivedbytes;
			packet_rssi = readReg(0x1A) - rssicorr;
			rssi = readReg(0x1B) - rssicorr;

			return true;

		} // received a message
	} // dio0=1
	return false;
}



void LoraTrx::configPower(int8_t pw) {
	if (sx1272 == false) {
		// no boost used for now
		if(pw >= 17) {
			pw = 15;
		} else if(pw < 2) {
			pw = 2;
		}
		// check board type for BOOST pin
		writeReg(RegPaConfig, (uint8_t)(0x80|(pw&0xf)));
		writeReg(RegPaDac, readReg(RegPaDac)|0x4);

	} else {
		// set PA config (2-17 dBm using PA_BOOST)
		if(pw > 17) {
			pw = 17;
		} else if(pw < 2) {
			pw = 2;
		}
		writeReg(RegPaConfig, (uint8_t)(0x80|(pw-2)));
	}
}



void LoraTrx::writeBuf(byte addr, byte *value, byte len) {
	unsigned char spibuf[256];
	spibuf[0] = addr | 0x80;
	for (int i = 0; i < len; i++) {
		spibuf[i + 1] = value[i];
	}
	selectreceiver();
	wiringPiSPIDataRW(CHANNEL, spibuf, len + 1);
	unselectreceiver();
}



void LoraTrx::txlora(byte *frame, byte datalen) {
	// set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
	writeReg(RegDioMapping1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
	// clear all radio IRQ flags
	writeReg(REG_IRQ_FLAGS, 0xFF);
	// mask all IRQs but TxDone
	writeReg(REG_IRQ_FLAGS_MASK, ~IRQ_LORA_TXDONE_MASK);

	// initialize the payload size and address pointers
	writeReg(REG_FIFO_TX_BASE_AD, 0x00);
	writeReg(REG_FIFO_ADDR_PTR, 0x00);
	writeReg(REG_PAYLOAD_LENGTH, datalen);

	// download buffer to the radio FIFO
	writeBuf(REG_FIFO, frame, datalen);
	// now we actually start the transmission
	opmode(OPMODE_TX);

	cout << "TX: " << frame << endl;
}



void LoraTrx::receiveMode(void) {
	opmode(OPMODE_RX);
}



void LoraTrx::transmitMode(void) {
	opmode(OPMODE_TX);
}



void LoraTrx::server_init(void) {
	int pipe_flags;
	uint32_t max_buffer_pipe_sz = 1048576; //TODO: Change after read max size file
	struct server_params *server_args;

	//TODO: Read /proc/sys/fs/pipe-max-size to max_buffer_pipe_sz
	
	if (max_buffer_pipe_sz > MAX_PIPE_BUFFER_SZ) {
		max_buffer_pipe_sz = MAX_PIPE_BUFFER_SZ;
	}

	pipe(buffer_pipe);

	pipe_flags = fcntl(buffer_pipe[PIPE_SERVER_WRITE], F_GETFL);
	pipe_flags |= O_NONBLOCK;
	if (fcntl(buffer_pipe[PIPE_SERVER_WRITE], F_SETFL, pipe_flags) == -1) {
		cerr << "Failed to set LoRa server pipe to non-blocking!" << endl;
		exit(EXIT_FAILURE);
	}
	if (fcntl(buffer_pipe[PIPE_SERVER_WRITE], F_SETPIPE_SZ, max_buffer_pipe_sz) == -1) {
		cerr << "Failed to set LoRa server pipe size!" << endl;
		exit(EXIT_FAILURE);
	}
	
	halt_server = false;
	server_args = (struct server_params*) malloc(sizeof(struct server_params));
	if (server_args == NULL) {
		cerr << "Failed to malloc() LoRa server argument structure!" << endl;
		exit(EXIT_FAILURE);
	}
	server_args->buffer_pipe = buffer_pipe;
	server_args->halt_server = &halt_server;
	server_args->trx_ptr = this;
	this->receiveMode();
	
	if (pthread_create(
			&server_thread,
			NULL,
			this->server,
			server_args
		) != 0)
	{
		cerr << "Failed to create the LoRa server thread!" << endl;
		exit(EXIT_FAILURE);
	}
}



void LoraTrx::close_server(void) {
	halt_server = true;
	//TODO: Check return val
	pthread_join(server_thread, NULL);
	close(buffer_pipe[PIPE_SERVER_WRITE]);
	close(buffer_pipe[PIPE_SERVER_READ]);
}



void *LoraTrx::server(void *arg) {
	string msg;
	byte len, prssi, rssi;
	long int snr;
	struct server_params *trx = (struct server_params*) arg;
	
	while (!(*trx->halt_server)) {
		if (trx->trx_ptr->receivepacket(msg, len, prssi, rssi, snr)) {
			//TODO: Combine these into a struct to do just 1 syscall
			//except the msg so we can get the len and read just that long of a msg
			write(trx->buffer_pipe[PIPE_SERVER_WRITE], &len, sizeof(len));
			write(trx->buffer_pipe[PIPE_SERVER_WRITE], &prssi, sizeof(prssi));
			write(trx->buffer_pipe[PIPE_SERVER_WRITE], &rssi, sizeof(rssi));
			write(trx->buffer_pipe[PIPE_SERVER_WRITE], &snr, sizeof(snr));
			write(trx->buffer_pipe[PIPE_SERVER_WRITE], msg.c_str(), len);
		}
		delay(1);
	}

	pthread_exit(NULL);
}






string LoraTrx::readMessage(void) {
	char c_msg[256];
	byte len, prssi, rssi;
	long int snr;
	
	//TODO: Return values -- error = flush buffer?
	read(buffer_pipe[PIPE_SERVER_READ], &len, sizeof(len));
	read(buffer_pipe[PIPE_SERVER_READ], &prssi, sizeof(prssi));
	read(buffer_pipe[PIPE_SERVER_READ], &rssi, sizeof(rssi));
	read(buffer_pipe[PIPE_SERVER_READ], &snr, sizeof(snr));
	read(buffer_pipe[PIPE_SERVER_READ], &c_msg, len);
	c_msg[len] = 0; //Ensure the string read is null terminated

	return c_msg;
}



int main (int argc, char *argv[]) {
	LoraTrx trx;
	char d[] = "recruitrecruiterrefereerehaverelativereporterrepresentativerestaurantreverendrguniotrichriderritzyroarsfulipwparkrrollerroofroomroommaterosessagesailorsalesmansaloonsargeantsarkscaffoldsceneschoolseargeantsecondsecretarysellerseniorsequencesergeantservantserverservingsevenseventeenseveralsexualitysheik/villainshepherdsheriffshipshopshowsidekicksingersingingsirensistersixsixteenskatesslaveslickersmallsmugglersosocialsoldiersolidersonsongsongstresssossoyspeakerspokenspysquawsquirestaffstagestallstationstatuesteedstepfatherstepmotherstewardessstorestorekeeperstorystorytellerstrangerstreetstripperstudentstudiostutterersuitsuitorssuperintendentsupermarketsupervisorsurgeonsweethearttailortakertastertaverntaxiteachertechnicianteentelegramtellertenthalthothetheatretheirtherthiefthirty-fivethisthreethroughthrowertickettimetknittotossedtouchtouristtouriststowntownsmantradetradertraintrainertravelertribetriptroopertroubledtrucktrusteetrustytubtwelvetwenty-fivetwintyuncleupstairsurchinsv.vaETERevaletvampirevanvendorvicarviceroyvictimvillagevisitorvocalsvonwaitingwaitresswalkerwarwardenwaswasherwomanwatchingwatchmanweaverwelwerewesswherewhichwhitewhowhosewifewinnerwithwittiestwomanworkerwriterxxxyyellowyoungyoungeryoungestyouthyszealot";
	char tx[1300];

	if (argc < 2) {
		cout << "Usage: " << argv[0] << " sender|rec" << endl;
		exit(EXIT_FAILURE);
	}

	if (!strcmp("sender", argv[1])) {

		for (byte i = 1; i < 256; i++) {
			strncpy(tx, d, i);
			trx.txlora((byte*)tx, i);
			delay(250);
		}

	} else {
		
		trx.server_init();

		for (;;) {
			cout << "LoraTrx::readMessage() returned: " << trx.readMessage() << endl;
		}

		trx.close_server();
	}

	return (0);
}