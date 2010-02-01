#    Purpose: I2C helper functions for use with AVRopendous USBtoSerialI2C-based devices
#    Created: 2009-04-15 by Opendous Inc.
#    Last Edit: 2009-04-15 by Opendous Inc.
#    Released under the MIT License
import binascii       # for converting received ASCII data to a workable form
import baseconvert    # for printing in binary format

# i2c_read: read data from an I2C device
#    device      - an object supporting read() and write() operations which sends bytes to a physical device over I2C
#    address     - proper, non-RW-adjusted, non-shifted, 7-bit I2C device address from its' datasheet
#    expectedlength        - length of data to read, i.e., number of bytes to read
#    data           - raw data tuple to send over I2C - should include any sub-addresses
# Note that a 1-element tuple is created with a comma:  (0x15,)
def i2c_readprint(device, address, expectedRECVlength, data):
    # Com. format is : Read/^Write - Device Address - Expected # of Bytes to Receive - Data
    # Device Address is proper address from device datasheet, not R/W adjusted address
    # for reads, data byte(s) is irrelevent
    length = len(data)
    s = chr(1) + chr(address) + chr(length) + chr(expectedRECVlength)
    for i in data:
        s = s + chr(i)
    device.write(s)

    # first returned value is the number of data bytes that should be received
    index = 0
    count = int(binascii.hexlify((device.read(1)).strip("\ x")))
    returnedData = []
    # copy all bytes into an array
    while (index < count):
        returnedData.append(device.read(1))
        index = index + 1

    print "\n76543210 - Hex- Int - Read from DeviceAddress:", hex(address), "SubAddress:", hex(data[0]), \
        "Length:", "%02d Data:" % int(expectedRECVlength), (returnedData)
    for i in range(len(returnedData)):
        print baseconvert.show_base(int(binascii.hexlify((returnedData[i]).strip("\ x")), 16), 2, 8), \
            "-", binascii.hexlify((returnedData[i]).strip("\ x")), "-", \
            '%03d' % int(binascii.hexlify((returnedData[i]).strip("\ x")), 16)

# i2c_write: read data from an I2C device
#    device      - an object supporting read() and write() operations which sends bytes to a physical device over I2C
#    address     - proper, non-RW-adjusted, non-shifted, 7-bit I2C device address from its' datasheet
#    writelength        - length of data to write, i.e., number of bytes to read
#    data           - raw data tuple to send over I2C - should include any sub-addresses
# Note that a 1-element tuple is created with a comma:  (0x15,)
def i2c_writeprint(device, address, writelength, data):
    # Com. format is : Read/^Write - Device Address - Sub-Address/Register - Length - Data
    # Device Address is proper address from device datasheet, not R/W adjusted address
    length = len(data)
    s = chr(0) + chr(address) + chr(length) + chr(writelength)
    for i in data:
        s = s + chr(i)
    device.write(s)

    # first returned value is the number of data bytes that should be received
    # already know only one byte, count, is returned, but need to clear it from buffer
    count = int(binascii.hexlify((device.read(1)).strip("\ x")))
    s = ""
    for i in data:
        s = s + hex(i) + ","
    print "Wrote to DeviceAddress:", hex(address), \
        "Length:", "%02d Data:" % int(length), (s)

def i2c_writeled(device, address, data):
    # Com. format is : Read/^Write - Device Address - Sub-Address/Register - Length - Data
    # Device Address is proper address from device datasheet, not R/W adjusted address
    length = len(data)
    s = chr(address)
    for i in data:
        s = s + chr(i)
    device.write(s)

    s = ""
    for i in data:
        s = s + hex(i) + ","
    print "Wrote to DeviceAddress:", hex(address), \
        "Length:", "%02d Data:" % int(length), (s)

if __name__ == '__main__':
    print "import this file into your programs"
