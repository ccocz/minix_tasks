rm -rf minix_source_orig
rm -f minix.img
cp /home/resul/Desktop/so/backup/minix.img ./
cp -rf /home/resul/Desktop/so/backup/minix_source_orig ./
sudo qemu-system-x86_64 -curses -drive file=minix.img,format=raw,index=0,media=disk -enable-kvm -rtc base=localtime -net user,hostfwd=tcp::10022-:22 -net nic,model=virtio -m 1024M
