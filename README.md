This is a C++20 implementation of GRS22's [Structure-Aware Private Set Intersection, With Applications to Fuzzy Matching](https://eprint.iacr.org/2022/1011). 

Since we use `std::bitset<>` as main medium of storage, most of the code is in .tpp and in `include` folder.
`/test` folder contains unit tests written with Catch2, which also serves the purpose of usage examples. 

# Installation
To reproduce in Docker container:
```bash
docker pull archlinux
docker run -t -d --name garimella archlinux
docker exec -it garimella bash
pacman -Syyuu
pacman -S python vim make wget curl git gcc autoconf automake pkgconf cmake openssh
```
Then, install libOTe:
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
The exectuable generated are `./tests` and `./garimella`. The latter one is a frontend that is work in progress.
