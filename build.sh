#!/bin/bash

meson build
cd build
sudo ninja
sudo ./demo
