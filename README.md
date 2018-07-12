# Water Quality Data Reader
This C script read data from YSI EXO2 multiparameter sonde with EXO Modbus Signal Output Adapter (SOA)
Set up rasperyPI to tranfer data with SOA using modbus

# Pre-request
The sensor and SOA communicate with the raspberry PI using Modbus protocal, so you need libmodbus 3.1.4 to compile the script.

install git first if you don't have it yet
    `sudo apt-get install git`

git clone the libmodbus5
     `git clone git://github.com/stephane/libmodbus` 

install libtool before install libmodbus
     `sudo apt-get install libtool`

install libmodbus after that
run `./autogen.sh` and run `./configure && make install`

run `ldconfig` as root if required

Refer to https://github.com/stephane/libmodbus 

the script also requires libcurl to send the data to influxDB through HTTP
install libcurl-dev if needed.

`sudo apt-get install libcurl-dev`

# Compile 
compile the C script using gcc and pkg-config.
`` gcc waterquality.c -o waterquality `pkg-config --libs --cflags libmodbus` ``

# Run
You can directly run the script or use nohup or tmux to run it so it will not be terminated if you log off from SSH.
`./waterquality`  or
`nohup ./waterquality`
You may need root to run
