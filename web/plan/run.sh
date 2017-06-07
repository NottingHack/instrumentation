#!/bin/sh
export GOPATH="$(pwd)/vendor"
go run plan.go -c ../../conf/plan.conf
