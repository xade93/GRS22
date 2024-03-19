This is a C++20 implementation of GRS22's [Structure-Aware Private Set Intersection, With Applications to Fuzzy Matching](https://eprint.iacr.org/2022/1011). 

## Assumptions
We assume 
1. `sizeof(long) == 8` (which should hold on most modern machines);
2. Host CPU supports AESNI operations. Over Linux this can be checked via `grep -o aes /proc/cpuinfo`.  Again this should hold on most reasonably modern machines.

## Project Structure
- `/src` contains sources. Since we use `std::bitset` as main mode of storage, most of the code is in .tpp and no separation of interface and implementation is possible.
    - `bfss/*` defines `(p, 1)-bFSS`, and implements *Spatial Hash* and *Truth Table* bFSS as described by paper.
    - `matrix_tools.tpp` contains basic linear algebra tools for working over $(\mathbb{F}_2)^q$.
    - `oblivious_transfer_short.tpp` contains wrapper for libOTe's oblivious transfer, limited to <=128bit only. This file is currently not used.
    - `oblivious_transfer.tpp` contains wrapper fro libOTe's oblivious transfer, except that it supports arbitrary length OT via hybrid encryption.
    - `okvs.tpp` contains oblivious key-value storage via random boolean matrix method, described in [PSI from PaXoS: Fast, Malicious Private Set Intersection](https://eprint.iacr.org/2020/193)
    - `protocol.tpp` contains implementation for the PSI protocol, using above as components.
- `/test` folder contains unit tests written with Catch2, which also serves the purpose of usage examples. 

## Installation
To reproduce in Docker container:
```bash
docker pull archlinux
docker run -t -d --name garimella archlinux
docker exec -it garimella bash
pacman -Syyuu
pacman -S python vim make wget curl git gcc autoconf automake pkgconf cmake openssh
```
Then, install libOTe (note below instructions are copied from libOTe page, and may subject to change over time):
```bash
pacman -S libtool
git clone https://github.com/osu-crypto/libOTe.git
cd libOTe
python build.py --all --boost --sodium
python build.py --install
```
Finally, clone this repository:
```bash
git clone https://github.com/xade93/GRS22
cd GRS22
mkdir build/
cmake -DCMAKE_BUILD_TYPE=Release
make -j
```
The exectuables generated are `./tests` and `./grs22`. The latter one is a frontend that is work in progress.
