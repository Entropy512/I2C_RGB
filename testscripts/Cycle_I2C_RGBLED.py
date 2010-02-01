#!/usr/bin/python
#    Purpose: Communicate over I2C with a device connected to a Micropendous
#        programmed with USBVirtualSerial_I2C firmware.  Visit Micropendous.org for more info.
#    Created: 2009-04-15 by Opendous Inc.
#    Last Edit: 2009-12-029 by Andrew Dodd
#    Released under the MIT License
import serial           # for accessing the serial port on multiple platforms
import i2cfuncs         # useful i2c functions
import time             # needed for sleep function
import sys              # needed for command-line input
import colorsys

def Test_I2C_RGBLED_Communication(comport):
    # open the given serial port for communication
    ser = serial.Serial(port=comport,baudrate=57600)

    print ser               # dump all info regarding serial port being used


    # RGB LED controller at address 0x32
    TRGT_ADDRESS = 0x10

    # i2cfuncs usage:
    # ser is the Serial object which must support read() and write() functions
    # TRGT_ADDRESS is the non-RW-adjusted, non-shifted, 7-bit device address from its' datasheet
    # length, 3rd variable, is the length of data (number of bytes) to expect to be returned
    # data is the tuple of data to send over I2C, which includes any sub-addresses
    # Note a 1 element tuple is created with a comma after the data (elem,)
    j = 0;

    #cycle from off to fullbright white




    while(1):
        if(1):
            while(j < 360):
                j += 1
                if((j % 30) == 0):
                    for bright in range(250,0,-5):
                        (r,g,b) = colorsys.hsv_to_rgb(j/360.0,1,bright/255.0)
                        r = int(r*255)
                        g = int(g*255)
                        b = int(b*255)
                        i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS, (r,g,b))
#                        time.sleep(0.01)
                    
                        k = j + 180
                        k = k % 360
                        (r,g,b) = colorsys.hsv_to_rgb(k/360.0,1,bright/255.0)
                        r = int(r*255)
                        g = int(g*255)
                        b = int(b*255)
                        i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS+1, (r,g,b))
                        time.sleep(0.01)

                    i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS, (255,255,255))
                    i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS+1, (255,255,255))
                    time.sleep(0.2)                    

                    for bright in range(0,250,5):
                        (r,g,b) = colorsys.hsv_to_rgb(j/360.0,1,bright/255.0)
                        r = int(r*255)
                        g = int(g*255)
                        b = int(b*255)
                        i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS, (r,g,b))
#                        time.sleep(0.01)
                    
                        k = j + 180
                        k = k % 360
                        (r,g,b) = colorsys.hsv_to_rgb(k/360.0,1,bright/255.0)
                        r = int(r*255)
                        g = int(g*255)
                        b = int(b*255)
                        i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS+1, (r,g,b))
                        time.sleep(0.01)

                (r,g,b) = colorsys.hsv_to_rgb(j/360.0,1,1)
                r = int(r*255)
                g = int(g*255)
                b = int(b*255)
                i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS, (r,g,b))
#                time.sleep(0.02)
                
                k = j + 180
                k = k % 360
                (r,g,b) = colorsys.hsv_to_rgb(k/360.0,1,1)
                r = int(r*255)
                g = int(g*255)
                b = int(b*255)
                i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS+1, (r,g,b))
                time.sleep(0.02)
 
#               if((j % 10) == 5):
#                    i2cfuncs.i2c_writeprint(ser, TRGT_ADDRESS, 1, (0,0,0))
#                    time.sleep(0.01)
#                    i2cfuncs.i2c_writeprint(ser, TRGT_ADDRESS+1, 1, (0xff,0xff,0xff))
#                    time.sleep(0.1)                   
#                if((j % 10) == 0):
#                    i2cfuncs.i2c_writeprint(ser, TRGT_ADDRESS, 1, (0xff,0xff,0xff))
#                    time.sleep(0.01)
#                    i2cfuncs.i2c_writeprint(ser, TRGT_ADDRESS+1, 1, (0,0,0))
#                    time.sleep(0.1)            

            j = 0
        if(0):
            while(j < 360):
                j += 60
                (r,g,b) = colorsys.hsv_to_rgb(j/360.0,1,1)
                r = int(r*255)
                g = int(g*255)
                b = int(b*255)
                i2cfuncs.i2c_writeled(ser, TRGT_ADDRESS, (r,g,b))
                time.sleep(2)
            j = 0


    ser.close()             # release/close the serial port



# if this file is the program actually being run, print usage info or run SerialSendReceive
if __name__ == '__main__':
    if (len(sys.argv) != 2):
        print "I2C RGBLED Serial Communication Example"
        print "    Usage:"
        print "      python", sys.argv[0], "<port>"
        print "        Where <port> = serial port; COM? on Windows, '/dev/ttyACM0 on Linux'"
        print "          Enumerated serial port can be found on Linux using dmesg"
        print "          look for something like  cdc_acm 2-1:1.0: ttyACM0: USB ACM device"
        print "      python", sys.argv[0], "COM5"
        exit()
    Test_I2C_RGBLED_Communication(sys.argv[1])
