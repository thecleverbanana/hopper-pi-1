# hopper-pi-1

## python file:
1. enable the virtual environment bno-venv by `source ~/bno-venv/bin/activate`
2. cd to hopper-pi-1 folder by `cd hopper-pi-1`
3. git pull to get updates
4. `python read_bno085_i2c.py` 
5. git push

## c++ python:
g++ -Iinclude src/bno085.cpp src/loadcell.cpp main.cpp -lpigpio -lrt -o hopper