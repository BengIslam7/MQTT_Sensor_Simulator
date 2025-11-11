# Zephyr MQTT Sensor Simulator

This project demonstrates a Zephyr application that simulates temperature and humidity sensor readings and publishes them to an MQTT broker.  
It runs on the **native_sim** board, allowing you to test Zephyr applications directly on your host PC (Linux, macOS, or Windows) without hardware.

---

## 🌐 Network Configuration

The Zephyr native_sim target runs as a Linux process and can emulate hardware peripherals such as Ethernet using the Zeth driver.
Zeth (short for Zephyr Ethernet) is a virtual Ethernet driver that connects the simulated Zephyr system (native_sim) to your host’s networking stack through a TAP interface.

### ⚙️ Configure the Host TAP Interface

Edit the configuration file located at tools/net-tools/zeth.conf:

```bash
IPV4_ADDR_1="192.168.1.100/24"   # Host TAP interface IP
IPV4_ROUTE_1="192.168.1.1/24"    # Default route or gateway to reach external network (e.g., MQTT broker)
```

Then, set up the TAP interface using the provided script:

```bash
./tools/net-tools/net-setup.sh start
```

This script creates and configures the zeth TAP device on the host.

### 🧩 Configure the Zephyr Application

In your Zephyr project's prj.conf, specify the IP settings for the Zephyr side:

```bash
CONFIG_NET_CONFIG_MY_IPV4_ADDR="192.168.1.99"   # IP of the Zephyr simulated node
CONFIG_NET_CONFIG_PEER_IPV4_ADDR="192.168.1.100" # IP of the host TAP interface
```

Additionally, make sure your application source code uses the same host IP address when configuring the broker or peer connection.
For example, in your MQTT or socket client code:

```bash
inet_pton(AF_INET, "192.168.1.100", &broker4->sin_addr);
```

This line ensures that your Zephyr application connects to the correct host (the TAP interface created by Zeth).

⚠️ Tip: Always keep the IP addresses in zeth.conf, prj.conf, and your source code consistent to avoid connection issues.

---

## ⚙️ Build

Clean and build the project using the `native_sim` board target:

```bash
west build -b native_sim --pristine
```

* `-b native_sim`: Specifies the target board for simulation.
* `--pristine`: Ensures a clean build by deleting the previous build directory.

---

## 🚀 Run

After building, run the generated executable:

```bash
build/zephyr/zephyr.exe
```

Or, depending on your system:

```bash
./build/zephyr/zephyr.exe
```

If you are using Linux, the output will appear in your terminal, for example:

```
*** Booting Zephyr OS build v3.x.x ***
[SENSOR] Temperature: 23.45°C, Humidity: 52.12%
[MQTT] Publish successful
```

---

## 🧹 Clean the Build

To remove all generated build files:

```bash
west build -t clean
```

or simply delete the build directory:

```bash
rm -rf build
```

---
