/*
  Universidad del Valle de Guatemala
  Computación paralela y distribuida
  Proyecto#2

  - Compilación: mpicc -o bruteforce2 bruteforce2.c -lcrypto -lssl
  - Ejecución: mpirun -np 4 ./bruteforce2 -k <llave>
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

// Función para descifrar utilizando una clave DES
void decrypt(DES_key_schedule key, unsigned char *ciph) {
    DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &key, DES_DECRYPT);
}

// Función para cifrar utilizando una clave DES
void encrypt(DES_key_schedule key, unsigned char *ciph) {
    DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &key, DES_ENCRYPT);
}

// Cadena de búsqueda en el texto desencriptado
char search[] = "es una prueba de";

// Función para intentar una clave y buscar la cadena en el texto desencriptado
int tryKey(DES_key_schedule key, unsigned char *ciph, int len) {
    unsigned char temp[len];
    memcpy(temp, ciph, len);
    decrypt(key, temp);
  
    // Busca la cadena "es una prueba de" en el texto desencriptado
    if (strstr((char *)temp, search) != NULL) {
        printf("Clave encontrada\n");
        return 1;
    }
  
    return 0;
}

int main(int argc, char *argv[]){
    int N, id;
    DES_key_schedule des_key;
    MPI_Status st;
    MPI_Request req;
    int flag;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    // Configura la clave DES a partir de una cadena ("my_password")
    DES_cblock key;
    DES_string_to_key("my_password", &key);
    DES_set_key_checked(&key, &des_key);

    // Divide el espacio de claves entre los nodos MPI
    int range_per_node = 0xFFFFFFFFFFFFFFFF / N;
    unsigned long long mylower = range_per_node * id;
    unsigned long long myupper = range_per_node * (id + 1) - 1;

    // Ajusta el límite superior para el último nodo
    if (id == N - 1) {
        myupper = 0xFFFFFFFFFFFFFFFF;
    }

    unsigned long long found = 0;
    MPI_Irecv(&found, 1, MPI_UNSIGNED_LONG_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    // Texto cifrado a descifrar
    unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66}; 

    // Inicia el contador de tiempo
    double start_time, end_time;
    start_time = MPI_Wtime();

    // Realiza la búsqueda de clave en el espacio asignado
    for (unsigned long long i = mylower; i <= myupper && (found == 0); ++i) {
        if (tryKey(des_key, cipher, sizeof(cipher))) {
            found = i;
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_UNSIGNED_LONG_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    // Termina el contador de tiempo
    end_time = MPI_Wtime();

    // Procesamiento en el nodo 0
    if (id == 0) {
        MPI_Wait(&req, &st);
        decrypt(des_key, cipher);
        printf("Clave encontrada: %llu\n", found);
        printf("Texto descifrado: ");
        for (int i = 0; i < sizeof(cipher); i++) {
            printf("%02x", cipher[i]);
        }
        printf("\n");
        printf("Tiempo de ejecución: %f segundos\n", end_time - start_time);
    }

    MPI_Finalize();
}
