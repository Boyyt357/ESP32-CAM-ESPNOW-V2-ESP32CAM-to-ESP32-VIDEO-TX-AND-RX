# **High-Speed ESP32-CAM Video Relay with ESP-NOW**

This repository contains the firmware for a two-node system designed to transmit video from an **ESP32-CAM (TX)** to a **Viewing Device (Client)** using the fast, peer-to-peer **ESP-NOW** protocol, with a second **ESP32 (RX)** acting as a dedicated relay and web server.

This approach eliminates the need for a router and significantly reduces latency compared to standard Wi-Fi streaming methods.

## **🎥 Video Tutorial**

For a detailed, step-by-step guide on setting up the hardware, flashing the code, and understanding the core concepts, please refer to the official video tutorial:

### **Tutorial and Testing👇**
[ESP32-CAM to ESP32 High Speed Video TX/RX using ESP-NOW](https://youtu.be/i73PIZorhhw)
<img width="1450" height="967" alt="3bb64f09-70f4-411a-84ec-98a55a553816" src="https://github.com/user-attachments/assets/2a39c84e-c764-425f-bb17-6d2cb3b30ca3" />

## **🚀 Architecture**

The system operates in two stages:

1. **Stage 1: High-Speed Wireless Link (ESP-NOW)**  
   * **TX Unit (ESP32-CAM):** Captures JPEG images, fragments them into small chunks (up to 1400 bytes each) with custom headers, and bursts them directly to the RX Unit using ESP-NOW.  
   * **RX Unit (ESP32):** Receives the fragments, uses a critical section (mutex) and a mapping structure (std::map\<uint32\_t, IncomingFrame\>) to reassemble the frame based on its unique frame\_id and packet count.  
2. **Stage 2: Client Access (Wi-Fi AP & HTTP)**  
   * The **RX Unit** simultaneously operates as a standard Wi-Fi Access Point (ESP32-AP).  
   * Once a frame is fully reassembled, the RX Unit stores the complete JPEG data.  
   * A viewing client (phone/PC) connects to the RX's AP and loads a simple web page which continuously requests the latest reassembled image via HTTP.

## **✨ Features**

* **ESP-NOW for Core Transmission:** Achieves maximum theoretical data rates and minimal latency between the two ESP32 units.  
* **Packet Fragmentation:** Implements custom logic to send large JPEG images by splitting them into smaller, transportable ESP-NOW packets.  
* **Web Server Interface:** The RX unit provides a responsive, single-page web interface to display the live feed, calculate FPS, and handle download/pause functions.  
* **Direct Link:** No external router required, making it ideal for field use, robotics, or long-range applications.  
* **Camera Configuration:** TX unit is pre-configured for QVGA (320x240) at high quality (20) for optimal speed/quality balance over ESP-NOW.

## **🛠️ Setup and Configuration**

This project requires manually configuring the MAC addresses of the two devices to establish the peer-to-peer ESP-NOW link.

### **1\. Identify MAC Addresses**

**Before uploading the code**, you must get the MAC address of the device intended to be the **RX Unit (Receiver)** and the **TX Unit (Sender)**.

You can upload a simple sketch to each ESP32 to get its Station MAC address (STA MAC):

\#include \<WiFi.h\>  
void setup() {  
  Serial.begin(115200);  
  delay(1000);  
  Serial.print("STA MAC Address: ");  
  Serial.println(WiFi.macAddress());  
}  
void loop() {}

### **2\. Configure TX Unit (TX.ino)**

The TX unit needs to know the MAC address of the RX unit to send data.

* **Action:** Update the receiverMac array in TX.ino with the **STA MAC address of your RX Unit**.

// Receiver MAC (Update this with the RX Unit's STA MAC)  
uint8\_t receiverMac\[6\] \= {0x00, 0x4B, 0x12, 0x4A, 0x81, 0xF9}; // \<-- \*\*CHANGE THIS\*\*

### **3\. Configure RX Unit (RX.ino)**

The RX unit needs to know the MAC address of the TX unit to register the peer.

* **Action:** Update the senderMac array in RX.ino with the **STA MAC address of your TX Unit (ESP32-CAM)**.

// sender MAC (Update this with the TX Unit's STA MAC)  
uint8\_t senderMac\[6\] \= {0x8C, 0x4F, 0x00, 0xD0, 0x4F, 0x20}; // \<-- \*\*CHANGE THIS\*\*

### **4\. Upload Code**

1. Upload RX.ino to your standard ESP32 board.  
2. Upload TX.ino to your ESP32-CAM board.

## **📺 Usage**

Once both devices are powered on and running:

1. **Connect Client:** Connect your phone, tablet, or PC to the Wi-Fi Access Point created by the RX Unit:  
   * **SSID:** ESP32-AP  
   * **Password:** esp32pass  
2. **Access Stream:** Open a web browser and navigate to the IP address of the RX Unit's Access Point:  
   \[http://192.168.4.1\](http://192.168.4.1)

3. The web page will load and automatically begin requesting the latest image from the RX's internal buffer, displaying a near real-time video stream transmitted over ESP-NOW.
