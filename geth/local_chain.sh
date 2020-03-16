#!/bin/bash
FILEPATH=$HOME/blockchain-iot-security/geth
geth \
	--unlock `cat "${FILEPATH}/unlock/addr1"` \
	--password "${FILEPATH}/unlock/pw1" \
	--syncmode 'full' \
	--gasprice 0 \
	--cache=2048 \
	--etherbase `cat "${FILEPATH}/unlock/addr1"` \
	--mine \
	console \
	--ipcpath "$HOME/.ethereum/geth.ipc" \
	--datadir "${FILEPATH}/localchain" \
	--networkid 21811 \
	--port 30303 \
	--allow-insecure-unlock \
	--netrestrict 127.0.0.1/0
