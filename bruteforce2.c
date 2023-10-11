#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <openssl/des.h>

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

    unsigned long long range = 0xFFFFFFFFFFFFFFFF; // Espacio de búsqueda completo
    unsigned long long range_per_node = range / N;
    unsigned long long mylower = id * range_per_node;
    unsigned long long myupper = (id + 1) * range_per_node - 1;

    // Sincronización antes de iniciar la búsqueda (elimina barreras innecesarias)
    MPI_Barrier(comm);

    unsigned long long found = 0;
    int search_finished = 0; // Variable de bandera para controlar la terminación del bucle de búsqueda

    for (unsigned long long i = mylower; i < myupper && !search_finished; ++i) {
        if (tryKey(des_key, cipher)) {
            found = i;
            search_finished = 1; // Se encontró la clave, establecer la bandera en 1
            // Sincronización antes de compartir el resultado (elimina barreras innecesarias)
            MPI_Barrier(comm);
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_UNSIGNED_LONG_LONG, node, 0, MPI_COMM_WORLD);
            }
        }
    }

    if (id == 0) {
        MPI_Wait(&req, &st);
        decrypt(des_key, cipher);
        printf("%llu %s\n", found, cipher);
    }

    MPI_Finalize();
}
