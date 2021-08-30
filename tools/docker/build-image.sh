#!/usr/bin/env bash
rm -rf ./build
mkdir build
cp -R ../../build/* ./build
docker build -t ibrio:latest .
rm -rf ./build
