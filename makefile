img=os.img

mbr_src=mbr.S
loader_src=loader.S

mbr=mbr.bin
loader=loader.bin

os:
	nasm -I boot/include/ -o boot/${mbr} boot/${mbr_src}
	nasm -I boot/include/ -o boot/${loader} boot/${loader_src}
	cd kernel && make && cd ..

dump: 
	cd kernel && objdump -S kernel.bin > kernel.dump

image:
	dd if=./boot/mbr.bin of=$(img) bs=512 count=1 conv=notrunc
	dd if=./boot/loader.bin of=$(img) bs=512 seek=2 count=3 conv=notrunc
	dd if=./kernel/kernel.bin of=$(img) bs=512 seek=9 count=200 conv=notrunc


run: 
	/usr/local/bin/bochs -qf bochsrc.floppy

clean:
	@-rm -rf kernel/build/*.o
