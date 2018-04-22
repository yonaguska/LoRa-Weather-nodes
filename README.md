# LoRa-Weather-nodes
This is based on Andreas Spiess' LoRa mailbox notifier, seen at https://youtu.be/WV_VumvI-0A. It consists of a singIe-channel gateway and one or more LoRa nodes.

The change to the LoRa node was the addition of a BME280 sensor and a 4-bit DIP switch. The sensor gives us temperature, humidity and pressure. The code adds pressure in inches of mercury, dewpoint and the battery voltage.

There are two types of LoRa gateways. The first type of LoRa gateway has code added to send messages to a MQTT queue on my computer. It connnects to my WiFi and sends the packets to a fixed IP. The second type of LoRa gateway has code added to send messages to a MQTT queue on Cayenne's web site. It connnects to my WiFi and sends the packets via the Cayenne library utilities.
