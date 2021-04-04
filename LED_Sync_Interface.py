"""
	Andrew O'Shei
	March 11, 2021
	Python OSC Loopback
	
	OSC Interface for synchronizing LED back lighting with control software
	Created as a proof of concept for the Shy Designs iPC
	The creator reserves all rights for code original to this project
"""

import serial
import io
import re
import argparse
import math
from pythonosc.dispatcher import Dispatcher
from pythonosc import osc_server

serCOM = serial.Serial('COM4', 115200)

rateState = 0

hexArray = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F']

"""Handle all other OSC Commands"""
def default_handler(address, *args):
    #addr = address #.replace('/hog/status/', '')
    #print(f"{addr}: {args}")
    return


"""Parse incoming LED Data"""
def led_handler(address, *args):
    global rateState
    addr = address.replace('/hog/status/led/', '')
    print(f"LED => {addr}: {args}")

    val = int(args[0])
    if '/' in addr:
      parseAddr = addr.split('/')
      pos = int(parseAddr[1]) - 1

      if parseAddr[0] == "choose":
        if pos < 10:
          push_led(pos, val)
      elif parseAddr[0] == "go":
        pos += 10
        push_led(pos, val)
      elif parseAddr[0] == "flash":
        pos += 20
        push_led(pos, val)
      
  
    elif addr == "ratedisabled":
      pos = 30
      if rateState == 1:
        rateState = 0
      else:
        rateState = 1
      push_led(pos, rateState)

    elif addr == "restore":
      pos = 31
      push_led(pos, val)

    elif addr == "macro":
      pos = 32
      push_led(pos, val)

    elif addr == "intensity":
      pos = 33
      push_led(pos, val)

    elif addr == "position":
      pos = 34
      push_led(pos, val)

    elif addr == "colour":
      pos = 35
      push_led(pos, val)

    elif addr == "beam":
      pos = 36
      push_led(pos, val)

    elif addr == "effects":
      pos = 37
      push_led(pos, val)

    elif addr == "time":
      pos = 38
      push_led(pos, val)

    elif addr == "group":
      pos = 39
      push_led(pos, val)

    elif addr == "fixture":
      pos = 40
      push_led(pos, val)

    elif addr == "highlight":
      pos = 41
      push_led(pos, val)

    elif addr == "blind":
      pos = 42
      push_led(pos, val)

    elif addr == "clear":
      pos = 43
      push_led(pos, val)
    
    elif re.match('^h([1-9]|1[0-2])$', addr):
      pos = int(addr[1:]) + 43
      push_led(pos, val)


"""Push LED Data to the Microcontroller"""
def push_led(pos, val):
  global serCOM
  if pos > 9:
    message = f"01{pos}0{val}\n"
  else:
    message = f"010{pos}0{val}\n"
  serCOM.write(message.encode("ascii"))
  print("LED: " + message)


"""Get fader value and parse"""
def fader_handler(address, *args):
  addr = address.replace('/hog/hardware/fader/', '')
  fader = int(addr)
  if fader <= 10:
    push_fader(fader, int(args[0]))


"""Send the fader value to microcontroller"""
def push_fader(fader, val):
  global serCOM
  if fader < 10:
    message = f"020{fader}{get_hex(val)}\n"
  else:
    message = f"020:{get_hex(val)}\n"
  serCOM.write(message.encode("ascii"))
  print(f"FADER {fader}, {val}")


"""Return int as a hex value"""
def get_hex(val):
  global hexArray
  return hexArray[val>>4] + hexArray[val % 16]


"""Handle and dismiss time messages"""
def time_handler(address, *args):
    return

"""Resets all leds and presets rate led"""
def sync_leds():
  for x in range(56):
    push_led(x, 0)  
  push_led(33, 1)

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--ip",
      default="192.168.0.105", help="The ip to listen on")
  parser.add_argument("--port",
      type=int, default=7002, help="The port to listen on")
  args = parser.parse_args()

  dispatcher = Dispatcher()

  # Catch led status messages
  dispatcher.map("/hog/status/led*", led_handler)

  # Catch fader value changes
  dispatcher.map("/hog/hardware/fader*", fader_handler)
  
  # Mask time status messages
  dispatcher.map("/hog/status/time*", time_handler)  
  dispatcher.set_default_handler(default_handler)

  sync_leds()  

  server = osc_server.ThreadingOSCUDPServer(
      (args.ip, args.port), dispatcher)
  print("Serving on {}".format(server.server_address))
  server.serve_forever()
