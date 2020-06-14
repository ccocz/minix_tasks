diff -rupN minix_source_orig/usr/ minix_source_new/usr/ > rh402185.patch
scp rh402185.patch test_minix.sh  minix:/
rm -rf minix_source_orig
cp -rf minix_source_new ./minix_source_orig
