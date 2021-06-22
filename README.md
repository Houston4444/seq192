# seq192

MIDI sequencer based on seq24 with less features and more swag.

**Less features**
- No song editor
- No keyboard controls
- No midi controls
- Linux only

**More swag**
- Interface rewritten with GTK3
- OSC controls
- almost 192 patterns per set

## Build

**Dependencies** (as debian packages)
```
libjack-jackd2-dev liblo-dev libgtkmm-3.0-dev libasound2-dev nlohmann-json3-dev
```

**Build**
```
make clean && make -j8
```



**Run**

```
usage: ./src/seq192 [options]

options:
  -h, --help              show available options
  -f, --file <filename>   load midi file on startup
  -c, --config <filename> load config file on startup
  -p, --osc-port <port>   osc input port (udp port number or unix socket path)
  -j, --jack-transport    sync to jack transport
  -n, --no-gui            enable headless mode
  -v, --version           show version and exit
```

**Install**

```bash
sudo make install
```

Append `PREFIX=/usr` to override the default installation path (`/usr/local`)

**Uninstall**

```bash
sudo make uninstall
```

Append `PREFIX=/usr` to override the default uninstallation path (`/usr/local`)

## Documentation

See [MANUAL.md](man/MANUAL.md) or run `man seq192` after installing.
