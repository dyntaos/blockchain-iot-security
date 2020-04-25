#!/bin/bash
FILEPATH=$HOME/blockchain-iot-security/geth
geth \
	--unlock `cat "${FILEPATH}/unlock/addr2"` \
	--password "${FILEPATH}/unlock/pw2" \
	--syncmode 'full' \
	--gasprice 0 \
	--targetgaslimit 9999999999999 \
	--cache=512 \
	--etherbase `cat "${FILEPATH}/unlock/addr2"` \
	--mine \
	console \
	--ipcpath "$HOME/.ethereum/geth.ipc" \
	--datadir "${FILEPATH}/testchain" \
	--networkid 21811 \
	--port 30303 \
	--miner.threads 3 \
	--allow-insecure-unlock \
	--nodiscover \
	--netrestrict 172.16.1.0/16
