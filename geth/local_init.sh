#!/bin/bash
FILEPATH=$HOME/blockchain-iot-security/geth
geth init --datadir "${FILEPATH}/localchain" "${FILEPATH}/genesis.local.json"
