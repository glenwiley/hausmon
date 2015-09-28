# owbmon
Outdoor Wood Burner monitoring system

This is a collection of hardware designs, MCU firmware and server software
that are used to monitor the state of an outdoor wood burner.  I built
this to work with a Central Boiler Classic Furnace (CL5036) but the ideas
make sense for any system that uses heated water.

The logical components of the system are:
* furnace monitor node
** TC74A0: temperature of water leaving the furnace
** TC74A2: temperature of water returning to furnace
** CT: monitor current for furnace to detect pump and solenoid operation
* house egres monitor node
** TC74A0: temperature of water entering crawl space
** TC74A2: temperature of water leaving crawl space
* Domestic Hot Water monitor node
** TC74A0: temperature of water entering heat exchanger
** TC74A2: temperature of water leaving heat exchanger
* Air Handler monitor node
** TC74A0: temperature of water entering heat exchanger
** TC74A2: temperature of water leaving heat exchanger

## Monitor Node

The nodes in this system are built on the Sparkfun Thing, an ESP8266
based MCU board.  I wrote the firware using the arduino IDE.

All of the monitor nodes are built to the same design, however not all of them
have all the sensors connected.  For example, there is a Current Transformer
(CT) on the furnace attached node but not on the node attached to the Domestic
Hot Water (DHW) heat exchanger since the DHW heat exchanger has no electronic
components.

The boards are placed in plastic enclosures to protect them from the
environment, however the enclosure is not sealed.  The CT is connected via
a 3.5mm TRS connector and audio jack, the I2C net is connected via pin headers
and the programming pins are available via headers.  
This allows sensors to be changed easily as needed.

Parts list:
* Sparkfun Thing, EXP8266 main board, $15, https://www.sparkfun.com/products/13231
* TC74, temperature sensor, $2, http://www.digikey.com/product-detail/en/TC74A0-3.3VAT/TC74A0-3.3VAT-ND/442720
* Current Transformer, $10, https://www.sparkfun.com/products/11005

<pre>

     +---- WiFi to network server
     |
     |
   +-------------+
   |  SFTHING    |------- USB power via mains
   |  ESP8266    |
   +-------------+
     |    |    |
     |    +----www----- CT via ADC0
     |         20ohm
     |
     | I2C network
     |
     |
     |  +----------+
     +--|  TC74A0  |
     |  +----------+
     |
     |  +----------+
     +--|  TC74A2  |
     |  +----------+
     |
     |  +----------+
     +--|  TC74A5  |
        +----------+


</pre>

### Power

Power for each node is supplied by a mains connected adaptor that delivers 5VDC
to the MCU via the USB mini connector on the board.

### Temperature Sensors

The TC74 from Microchip has an I2C interface, the fixed address for each
device is indicated by the part suffix (A0, A2, A5).  The TC74 is available
in both 5V and 3.3A specifications, the 3.3V is used in this project
since the MCU is running at 3.3V.  The TC74 provides 
a very simple interface for reading teh temperature in Celsius.  Each sensor
is calibrated before deployment, the calibration adjustment is stored in a
file used by the network server to adjust the readings prior to permanent
storage.

### Current Transformer

The noninvasive current transformer clips over one of the AC lines and
provides 10mV per amp to the ADC on the MCU.  A 20 Ohm burden resistor
is used to provide a load for the coil.

## Server

The network server (in svr/) listens for connections from the monitoring
nodes and records data reported by the nodes.
