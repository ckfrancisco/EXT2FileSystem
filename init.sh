rm cf
mkfs cf 1440
cc -m32 -w -g main.c -o main.out
main.out < file.txt

rm fs
mkfs fs 1440
