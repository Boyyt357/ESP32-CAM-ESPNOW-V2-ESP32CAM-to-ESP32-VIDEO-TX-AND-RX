# **📷 ESP32-CAM High-Speed ESP-NOW Video Link**

This project demonstrates a high-performance, low-latency method for streaming live video from an **ESP32-CAM** to a local viewing device (phone, PC) by leveraging a two-part system:

1. **ESP-NOW:** Used for the fast, dedicated, point-to-point wireless link between the ESP32-CAM (Sender) and a standard ESP32 (Receiver).  
2. **Web Server (AP Mode):** The receiving ESP32 creates its own Wi-Fi Access Point (AP) and hosts a simple web server to display the incoming video frames.

This decoupled architecture allows the image transmission to benefit from the speed of **ESP-NOW** while still providing the convenience of Wi-Fi for viewing the stream.

## **🚀 Key Features and Benefits**

The primary advantage of this setup is the separation of duties, which dramatically improves performance, especially at range or in congested environments, compared to a single ESP32-CAM attempting to stream over a Wi-Fi network.

| Feature | ESP-NOW Camera Link (This Project) | Standard Wi-Fi Camera Stream (Single Module) |
| :---- | :---- | :---- |
| **Protocol Efficiency** | **Extremely high.** Minimal overhead allows for faster frame rates. | High overhead (TCP/IP stack, HTTP/MJPEG headers) limits speed. |
| **Wireless Range & Stability** | **Dedicated P2P Link.** The ESP-NOW link is robust and performs better over distance. | Dependent on a central router's signal strength and environmental congestion. |
| **Network Requirement** | **No external router needed.** The receiver creates the access point. | Requires both the camera and viewer to connect to the same external Wi-Fi router. |
| **Camera Focus** | ESP32-CAM focuses only on **capture and sending**, optimizing its performance. | Single module handles camera capture, JPEG encoding, TCP/IP stack management, and HTTP serving. |

## **🛠️ Components Required**

1. **ESP32-CAM Module** (Sender)  
2. **Standard ESP32 Development Board** (Receiver/Web Server)  
3. **USB-to-Serial Programmer** (for ESP32-CAM)

## **⚙️ Setup and Code Upload Guide**

### **1\. Prerequisite Libraries and Setup**

Ensure you are using the **Arduino IDE** or **PlatformIO** with the ESP32 board package installed. This project uses standard built-in libraries (esp\_now.h, WiFi.h, esp\_camera.h, WebServer.h).

### **2\. Critical: Configure MAC Addresses**

The ESP-NOW protocol requires specifying the MAC address of the target device. You must configure these values correctly in both sketches.

A. Find Your MAC Addresses  
Upload a simple sketch to both your ESP32-CAM and Standard ESP32 to print their Wi-Fi Station MAC addresses. The output will look like 00:4B:12:4A:81:F9.  
B. Update the Sender Code (ESP32-CAM-Sender.ino)  
Modify the receiverMac array to match the MAC address of your Standard ESP32 Receiver board.  
// Receiver MAC (MAC of your Standard ESP32)  
uint8\_t receiverMac\[6\] \= {0x00, 0x4B, 0x12, 0x4A, 0x81, 0xF9}; // \<-- UPDATE THIS

C. Update the Receiver Code (ESP32-Receiver-Server.ino)  
Modify the senderMac array to match the MAC address of your ESP32-CAM Sender board.  
// Sender MAC (MAC of your ESP32-CAM)  
uint8\_t senderMac\[6\] \= {0x8C, 0x4F, 0x00, 0xD0, 0x4F, 0x20}; // \<-- UPDATE THIS

### **3\. Uploading the Code**

1. Upload the **Sender Code** (ESP32-CAM-Sender.ino) to your **ESP32-CAM**.  
2. Upload the **Receiver Code** (ESP32-Receiver-Server.ino) to your **Standard ESP32** board.

## **💻 How to Use the System**

### **1\. Power On**

Power both the ESP32-CAM (Sender) and the Standard ESP32 (Receiver).

### **2\. Connect to the Access Point**

The Standard ESP32 (Receiver) will automatically create a Wi-Fi Access Point (AP).

* **SSID:** ESP32-AP  
* **Password:** esp32pass

Connect your phone, tablet, or PC to this Wi-Fi network.

### **3\. View the Stream**

Open a web browser on your connected device and navigate to the default IP address:

\[http://192.168.4.1\](http://192.168.4.1)

You should now see the live video stream being served from the receiver, with the image data being rapidly fed over the dedicated ESP-NOW link.

### **4\. Web Interface Functions**

The basic web interface provides:

* **Live Stream:** Displays the most recently received JPEG frame.  
* **Pause/Play Button:** Stops or resumes the auto-refreshing stream.  
* **Download Button:** Captures and downloads the current frame as a JPEG file.  
* **Statistics:** Displays estimated FPS (Frames Per Second) and latency.

## **🎯 Ideal Use Cases**

This high-speed, dual-module approach is perfect for applications where low latency and reliable data transmission are critical:

1. **Remote FPV (First-Person View) for RC Models/Drones:** The dedicated ESP-NOW link ensures minimal delay when transmitting video from a fast-moving vehicle to a nearby ground station.  
2. **Long-Range Monitoring:** Place the ESP32-CAM far from the viewing location, and use the Standard ESP32 as a "wireless repeater" or gateway to bridge the high-speed ESP-NOW link to a convenient local Wi-Fi AP.  
3. **Industrial/Workshop Monitoring:** Monitor equipment in a noisy RF environment where standard Wi-Fi might struggle, relying on the robust ESP-NOW protocol to punch through interference.
