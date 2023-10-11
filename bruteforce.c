//bruteforce.c
//nota: el key usado es bastante pequenio, cuando sea random speedup variara
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <openssl/des.h> // Incluimos las cabeceras de OpenSSL

void decrypt(DES_key_schedule key, unsigned char *ciph) {
    DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &key, DES_DECRYPT);
}

void encrypt(DES_key_schedule key, unsigned char *ciph) {
    DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &key, DES_ENCRYPT);
}

char search[] = " the ";

int tryKey(DES_key_schedule key, unsigned char *ciph) {
    unsigned char temp[8];
    memcpy(temp, ciph, 8);
    decrypt(key, temp);
    return strstr((char *)temp, search) != NULL;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66};

int main(int argc, char *argv[]) {
    int N, id;
    DES_key_schedule des_key;
    MPI_Status st;
    MPI_Request req;
    int flag;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    DES_cblock key;
    DES_string_to_key("my_password", &key);
    DES_set_key_checked(&key, &des_key);

    int range_per_node = 0xFFFFFFFFFFFFFFFF / N;
    unsigned long long mylower = range_per_node * id;
    unsigned long long myupper = range_per_node * (id + 1) - 1;

    if (id == N - 1) {
        myupper = 0xFFFFFFFFFFFFFFFF;
    }

    unsigned long long found = 0;
    MPI_Irecv(&found, 1, MPI_UNSIGNED_LONG_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (unsigned long long i = mylower; i < myupper && (found == 0); ++i) {
        if (tryKey(des_key, cipher)) {
            found = i;
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_UNSIGNED_LONG_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    if (id == 0) {
        MPI_Wait(&req, &st);
        decrypt(des_key, cipher);
        printf("%llu %s\n", found, cipher);
    }

    MPI_Finalize();
}
