# Water Quality Data Reader
This C script read data from YSI EXO2 multiparameter sonde with EXO Modbus Signal Output Adapter (SOA)
Set up rasperyPI to tranfer data with SOA using modbus

install git first
	$ sudo apt-get install git

git clone the libmodbus5
     $ git clone git://github.com/stephane/libmodbus 

install libtool before install libmodbus
$sudo apt-get install libtool

install libmodbus after that
run ./autogen.sh and run ./configure && make install

run ldconfig as root if required

the script also requires libcurl to send the data to influxDB through HTTP
install libcurl-dev if needed

sudo apt-get install libcurl-dev