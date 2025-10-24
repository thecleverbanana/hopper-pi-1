# hopper-pi-1

## python file:
1. enable the virtual environment bno-venv by `source ~/bno-venv/bin/activate`
2. cd to hopper-pi-1 folder by `cd hopper-pi-1`
3. git pull to get updates
4. `python read_bno085_i2c.py` 
5. git push

## c++ script:
make a script excutable: `chmod +x <file>.sh`
run the script: `./<>.sh`

## remote connection
`rsync -av --delete ~/Desktop/hopper-pi-1/ hopper-pi-1@192.168.8.145:~/hopper-pi-1/`
rsync -av --delete ./hopper-pi-1/ hopper-pi-1@192.168.8.145:~/hopper-pi-1/
`rsync -av --delete ~/Desktop/hopper-pi-1/ hopper@192.168.1.1:~/hopper-pi-1/`
rsync -av --delete ./hopper-pi-1/ hopper@192.168.1.1:~/hopper-pi-1/

