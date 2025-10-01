#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script is located in: $SCRIPT_DIR"

echo | openssl s_client -showcerts -connect batteryportal-e9czhgamgferavf7.ukwest-01.azurewebsites.net:443 2>/dev/null | awk '/-----BEGIN CERTIFICATE-----/{n++} n==2' | openssl x509 -outform PEM > website_cert.pem
xxd -i website_cert.pem > $SCRIPT_DIR/include/cert.h
