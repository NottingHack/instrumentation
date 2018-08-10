#!/bin/sh
export GOPATH="$(pwd)/vendor"
go build plan.go
