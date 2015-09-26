# owbmon
Outdoor Wood Burner monitoring system

This is a collection of hardware designs, MCU firmware and server software
that are used to monitor the state of an outdoor wood burner.  I built
this to work with a Central Boiler Classic Furnace (CL5036) but the ideas
make sense for any system that uses heated water.

## Monitor Node

The nodes in this system are built on the Sparkfun Thing, an ESP8266
based MCU board.  I wrote the firware using the arduino IDE.

All of the monitor nodes are built the same, however not all of them have
all the sensors connected.  For example, there is a Current Transformer (CT)
on the furnace attached node but not on the node attached to the Domestic
Hot Water (DHW) heat exchanger since the DHW heat exchanger has no electronic
components.

<pre>

     +---- WiFi to network server
     |
     |
   +-----------+
   |  SFTHING  |------- USB power via mains
   |  ESP8266  |
   +-----------+
     |    |
     |    +------------ CT via ADC0
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

## Server

The network server (in svr/) listens for connections from the monitoring
nodes and records data reported by the nodes.
