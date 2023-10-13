/*
  Universidad del Valle de Guatemala
  Computación paralela y distribuida
  Proyecto #2

  - Compilación: mpicc -o NaiveV1 NaiveV1.c -lcrypto -lssl
  - Ejecución: mpirun -np 4 ./NaiveV1 -k <llave>
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <ctype.h>

void decrypt(long key, char *ciph, int len) {
    DES_cblock key_block;
    DES_key_schedule schedule;
    memset(key_block, 0, sizeof(DES_cblock));
    memcpy(key_block, &key, sizeof(long));
    DES_set_key_unchecked(&key_block, &schedule);
    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_DECRYPT);
    }
}

void encrypt(long key, char *ciph, int len) {
    DES_cblock key_block;
    DES_key_schedule schedule;
    memset(key_block, 0, sizeof(DES_cblock));
    memcpy(key_block, &key, sizeof(long));
    DES_set_key_unchecked(&key_block, &schedule);
    for (int i = 0; i < len; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_ENCRYPT);
    }
}

char search[] = "es una prueba de";

int tryKey(long key, char *ciph, int len) {
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    if (strstr((char *)temp, search) != NULL) {
        printf("Clave encontrada: %li\n", key);
        return 1;
    }
    return 0;
}

int loadTextFromFile(const char *filename, char **text, int *length) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return 0;
    }
    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    fseek(file, 0, SEEK_SET);
    *text = (char *)malloc(*length);
    if (*text == NULL) {
        perror("Error de asignación de memoria");
        fclose(file);
        return 0;
    }
    fread(*text, 1, *length, file);
    fclose(file);
    return 1;
}

int saveTextToFile(const char *filename, char *text, int length) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return 0;
    }
    fwrite(text, 1, length, file);
    fclose(file);
    return 1;
}

int main(int argc, char *argv[]) {
    MPI_Init(NULL, NULL);
    MPI_Comm comm = MPI_COMM_WORLD;
    int N, id;
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    double start_time, end_time;
    long found = 0;
    int textLength;
    char *text;
    long upper = (1L << 56);

    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;

    long encryptionKey = 123456L;
    int option;

    while ((option = getopt(argc, argv, "k:")) != -1) {
        switch (option) {
            case 'k':
                encryptionKey = atol(optarg);
                break;
            default:
                if (id == 0) {
                    fprintf(stderr, "Uso: %s [-k key]\n", argv[0]);
                }
                MPI_Finalize();
                exit(1);
        }
    }

    start_time = MPI_Wtime();

    int range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id + 1) - 1;
    if (id == N - 1) {
        myupper = upper;
    }

    char *text_to_encrypt;
    int textLength_to_encrypt;

    if (id == 0) {
        if (!loadTextFromFile("input.txt", &text_to_encrypt, &textLength_to_encrypt)) {
            MPI_Finalize();
            return 1;
        }

        encrypt(encryptionKey, text_to_encrypt, textLength_to_encrypt);

        if (!saveTextToFile("EncryptedNaiveV1.txt", text_to_encrypt, textLength_to_encrypt)) {
            free(text_to_encrypt);
            MPI_Finalize();
            return 1;
        }
    }

    MPI_Bcast(&textLength, 1, MPI_INT, 0, comm);
    if (id != 0) {
        text = (char *)malloc(textLength);
    }
    MPI_Bcast(text, textLength, MPI_CHAR, 0, comm);

    int blockSize = (upper - 1) / N + 1;
    mylower = id * blockSize;
    myupper = (id + 1) * blockSize;
    if (myupper > upper) {
        myupper = upper;
    }

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        if (tryKey(i, text, textLength)) {
            found = i;
            MPI_Bcast(&found, 1, MPI_LONG, 0, comm);
            break;
        }
    }

    end_time = MPI_Wtime();

    if (id == 0) {
        decrypt(found, text, textLength);
        printf("Tiempo de ejecución: %f segundos\n", end_time - start_time);
        if (!saveTextToFile("DecryptedNaiveV1.txt", text, textLength)) {
            free(text);
            MPI_Finalize();
            return 1;
        }
    }

    free(text);

    MPI_Finalize();
}
