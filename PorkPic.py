#!/usr/bin/python
from PIL import Image
width = 1000
height = 1000
outputFileName = "batman.png"
inputFile = "porkchop_" + str(width) + "x" + str(height) + ".binary"

data = open(inputFile, "rb")

binary = bytes([byte for byte in data.read()])

dvMap = Image.frombytes("L",(width,height),binary,"raw","L",0,-1)
#dvMap.show()
dvMap.save(outputFileName, "PNG")
