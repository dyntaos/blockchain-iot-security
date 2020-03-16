#!/bin/bash
FILEPATH=$HOME/blockchain-iot-security/geth
geth init --datadir "${FILEPATH}/testchain" "${FILEPATH}/genesis.json"
