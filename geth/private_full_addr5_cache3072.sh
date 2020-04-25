#!/bin/bash
FILEPATH=$HOME/blockchain-iot-security/geth
geth \
	--unlock `cat "${FILEPATH}/unlock/addr5"` \
	--password "${FILEPATH}/unlock/pw5" \
	--syncmode 'full' \
	--gasprice 0 \
	--cache=3072 \
	--etherbase `cat "${FILEPATH}/unlock/addr5"` \
	--mine \
	console \
	--ipcpath "$HOME/.ethereum/geth.ipc" \
	--datadir "${FILEPATH}/testchain" \
	--networkid 21811 \
	--port 30303 \
	--allow-insecure-unlock \
	--rpc \
	--bootnodes 'enode://c5e8421dfa0c296e0294b2235c56a283652bf269b477c2cbbbf4bbf434a00090ccf9c2d0a5e55387348fc105305e8080ccb40ebaf6066ae06b7b90471ddf0658@172.16.1.100:30304' \
	--netrestrict 172.16.1.1/16,204.83.77.178/0
