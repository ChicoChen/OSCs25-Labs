#!/bin/bash

cd .. && make clean && make;
cd scripts && python send_kernel.py