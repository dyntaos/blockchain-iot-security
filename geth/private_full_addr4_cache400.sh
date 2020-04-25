#!/bin/bash
FILEPATH=$HOME/blockchain-iot-security/geth
geth \
	--unlock `cat "${FILEPATH}/unlock/addr4"` \
	--password "${FILEPATH}/unlock/pw4" \
	--syncmode 'full' \
	--gasprice 0 \
	--targetgaslimit 9999999999999 \
	--cache=400 \
	--etherbase `cat "${FILEPATH}/unlock/addr4"` \
	--mine \
	console \
	--ipcpath "$HOME/.ethereum/geth.ipc" \
	--datadir "${FILEPATH}/testchain" \
	--networkid 21811 \
	--port 30303 \
	--allow-insecure-unlock \
	--netrestrict 172.16.1.1/16
