#! /bin/bash

name="dogratian/ubuntu16_iot7688"
echo "Building $name"

sudo docker build -t $name .
sudo docker images
