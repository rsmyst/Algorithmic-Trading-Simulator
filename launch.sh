#!/bin/bash
# This script launches the TUI simulation in a new terminal window

echo "Starting simulation in new window..."
xterm -bg black -fg white -e ./build/bin/tradingSim
