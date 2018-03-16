RES=raport3a.txt
./program_static -a 50 -b 80 -f 20 -x 30 -y 30 >> ${RES}
./program_static -a 50 -b 80 -f 20 -x 30 -y 30 -s >> ${RES}
./program_shared -a 50 -b 80 -f 20 -x 30 -y 30 >> ${RES}
./program_shared -a 50 -b 80 -f 20 -x 30 -y 30 -s >> ${RES}
./program_dynamic -a 50 -b 80 -f 20 -x 30 -y 30 >> ${RES}
./program_dynamic -a 50 -b 80 -f 20 -x 30 -y 30 -s >> ${RES}
