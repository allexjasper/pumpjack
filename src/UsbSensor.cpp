#include "UsbSensor.h"

#include <boost/log/trivial.hpp>

#include <libusb-1.0/libusb.h>

#define CMD_ECHO 0
#define CMD_GET_FUNC 1
#define CMD_SET_DELAY 2
#define CMD_GET_STATUS 3
#define CMD_I2C_IO 4
#define CMD_I2C_IO_BEGIN 1
#define CMD_I2C_IO_END 2
#define CMD_START_BOOTLOADER 0x10
#define CMD_SET_BAUDRATE 0x11
#define I2C_M_RD 0x01
#define I2C_MP_USB_VID 0x0403
#define I2C_MP_USB_PID 0xc631



libusb_device_handle* usbhandle;

void initialize_usb_device() {
    libusb_context* ctx;
    libusb_init(&ctx);
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 3);

    libusb_device** list;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) {
        BOOST_LOG_TRIVIAL(info) << "Error getting device list";
        return;
    }

    int found = 0;
    for (ssize_t i = 0; i < cnt; i++) {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(list[i], &desc);
        if (desc.idVendor == I2C_MP_USB_VID && desc.idProduct == I2C_MP_USB_PID) {
            libusb_open(list[i], &usbhandle);
            found = 1;
            break;
        }
    }

    libusb_free_device_list(list, 1);

    if (!found) {
        BOOST_LOG_TRIVIAL(info) << "I2C_MP_USB device not found";
        return;
    }

    libusb_claim_interface(usbhandle, 0);
}

void close_usb_device() {
    libusb_close(usbhandle);
    libusb_exit(NULL);
}

void perform_control_transfer(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint8_t* data, uint16_t length) {
    int result = libusb_control_transfer(usbhandle, request_type, request, value, index, data, length, 0);
    if (result < 0) {
        BOOST_LOG_TRIVIAL(info) << "Control transfer failed: " << libusb_strerror(result);
    }
}


UsbSensor::UsbSensor(float magnetOffset) : _shutdownRequested(false), _magnetOffset(magnetOffset)
{
}

void UsbSensor::readData(Queue& queue)
{
    _thread = std::thread([this, &queue]() {
        readThread(queue);
        });
}

void UsbSensor::shutdown()
{
    _shutdownRequested = true;
    _thread.join();
}

void UsbSensor::readThread(Sensor::Queue& queue)
{
    initialize_usb_device();

    const int AS5600_SCL = 19;
    const int AS5600_SDA = 22;
    const int AS5600_ADDRESS = 0x36;
    const int ANGLE_H = 0x0E;
    const int ANGLE_L = 0x0F;
    const int length = 2; // high and low bytes

    std::vector<uint8_t> buf(1 + length);
    buf[0] = ANGLE_H;


    std::vector<std::chrono::time_point<std::chrono::steady_clock>> timestamps;

    int i = 0;
    while (!_shutdownRequested)
    {
        i++;
        perform_control_transfer((LIBUSB_REQUEST_TYPE_CLASS & ~LIBUSB_ENDPOINT_DIR_MASK) | LIBUSB_ENDPOINT_OUT,
            CMD_I2C_IO + CMD_I2C_IO_BEGIN, 0, AS5600_ADDRESS, &buf[0], 1);

        perform_control_transfer((LIBUSB_REQUEST_TYPE_CLASS & ~LIBUSB_ENDPOINT_DIR_MASK) | LIBUSB_ENDPOINT_IN,
            CMD_I2C_IO + CMD_I2C_IO_END, I2C_M_RD, AS5600_ADDRESS, &buf[1], length);

        float angle = static_cast<float>(((buf[1] << 8) | buf[2])) / 4096.0f * 360.0f - _magnetOffset;
      
     
        if (!queue.push({ angle, std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()), 0 }))
        {
            BOOST_LOG_TRIVIAL(info) << "inbound queue overflow" << std::endl;
        }

    }

    close_usb_device();
}