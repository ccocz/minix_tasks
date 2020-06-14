set -e
patch -p1 < rh402185.patch
cd /usr/src/minix/drivers/dfa
make && make install
service update /service/dfa
cd /home
set +e
