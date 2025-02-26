# Legacy Logic Synthesis Software

This repository contains old logic synthesis software, from University of California, Berkeley:
- SIS
- BALM
- MVSIS

These are all refactored releases, for making sure they compile on modern systems like Ubuntu 18.04.

> AUR ...

## SIS/MVSIS/BALM

The repo contains SIS 1.4, an unofficial release of SIS, the logic synthesis
system from UC Berkeley. The repo also contains MVSIS 3.0 and BALM 2.0, with a
small set of patches for allowing compilation under modern systems.

The primary features added on top of SIS 1.3 are:
- Code refactoring for making sure it compiles under modern Linux systems like Ubuntu 18.04
- Exploring and deleting unused/unuseful parts (like `xsis`)
- Rewriting the `sis` shell using `GNU readline`

### Compatibility

This repo is tested on the following distributions:
  - Ubuntu 22.04 / 18.04 / 18.10 / 16.04
  - Arch Linux

Some other platforms should be okay with little or no modifications.  However,
SIS is old software, and may not build or run correctly on modern systems.

### Compilation and installation

If you're using a supported OS like Debian/Ubuntu, you can use the Debian
package you can find in the "_releases_" tab.  There's also an already compiled
static binary, with installation and uninstallation scripts.

For compiling SIS, you'll need the following software:
- `GNU gcc` (tested with version 7.3)
- `GNU make` (tested with version 4.1)
- `GNU bison` (tested with version 3.0.4)
- `GNU flex` (tested with version 2.6.4)
- `GNU readline` (tested with version 7)
If you are using Ubuntu, you can easily install all these dependecies with
```shell
sudo apt install -y make gcc bison flex build-essential libreadline-dev gcc-9
```

Then run these commands:

```shell
autoreconf -i
CC="gcc-9" CFLAGS="-fno-stack-protector" \
  ./configure --prefix=<target install directory> <other options>
make
sudo make install
```

The gcc flag `-fno-stack-protector` should be used cautiously (for security
reason, remove this flag when you hit a seg fault). This flag "fixes" a stack
smashing error when running `red_removal`. 

You can also build it using the provided `Dockerfile`:

```shell
# Building the image
docker build -t sis-image-ubuntu22.04 .

# Running the image
docker run -it sis-image-ubuntu22.04
```
