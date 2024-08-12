# QEMU/KVM install guide

## install QEMU
```bash
sudo apt update
sudo apt upgrade
sudo apt install qemu-system
```

## check KVM support
To check whether current system supports KVM
```bash
kvm-ok
```

## VM Manager install
The virt-manager provides both of command line and GUI interface to manager VM.
```bash
sudo apt install virt-manager
```