rm cf
mkfs cf 1440
cc -m32 -w main.c -o main.out
main.out cf
