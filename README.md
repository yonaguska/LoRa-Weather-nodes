# LoRa-Weather-nodes
This is based on Andreas Spiess' LoRa mailbox notifier, seen at https://youtu.be/WV_VumvI-0A. It consists of a singIe-channel gateway and one or more LoRa nodes. I've made a couple of short videos and supplied some pictures of these nodes. The pictures are in the repository and the videos are https://youtu.be/bzqtKwmoZaE, https://youtu.be/Qnk1anLy27Q and https://youtu.be/R6djJ7CDpwM.

The change to the LoRa node was the addition of a BME280 sensor and a 4-bit DIP switch. The sensor gives us temperature, humidity and pressure. The code adds pressure in inches of mercury, dewpoint and the battery voltage. I've added a solar charge controller, a Li-ion battery and a 6vdc solar panel to supply power to the node and make it usable remotely.

There are two types of LoRa gateways. The first type of LoRa gateway has code added to send messages to a MQTT queue on my computer. It connnects to my WiFi and sends the packets to my computer's IP. I pick up this data and put it into InfluxDB, and I use Grafana to visualize the data. 

The second type of LoRa gateway has code added to send messages to a MQTT queue on Cayenne's web site. It connnects to my WiFi and sends the packets via the Cayenne library utilities. It requires you creat an account on Cayenne, but it's free. On Cayenne I've defined a device into which I receive and display the data.
