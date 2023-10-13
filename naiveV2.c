/*
  Universidad del Valle de Guatemala
  Computación paralela y distribuida
  Proyecto#2

  - Compilación: mpicc -o bruteforce bruteforce.c -lcrypto -lssl -fopenmp -w
  - Ejecución: mpirun -np 4 ./bruteforce -k <llave>
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <ctype.h>


void decrypt(long key, char *ciph, int len) {
    // printf("Decrypting with key: %ld\n", key);

    DES_cblock key_block;
    DES_key_schedule schedule;

    // Inicializa el bloque de clave DES con ceros
    memset(key_block, 0, sizeof(DES_cblock));
    // Copia la clave proporcionada en el bloque de clave
    memcpy(key_block, &key, sizeof(long));

    // Crea un horario de clave DES basado en la clave
    DES_set_key_unchecked(&key_block, &schedule);

    // Realiza una desencriptación DES en el texto cifrado
    for (int i = 0; i < len; i += 8) {
        // printf("Decrypting block %d\n", i/8);
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_DECRYPT);
    }
}

void encrypt(long key, char *ciph, int len){
    // printf("Encrypting with key: %ld\n", key);

    DES_cblock key_block;
    DES_key_schedule schedule;

    // Inicializa el bloque de clave DES con ceros
    memset(key_block, 0, sizeof(DES_cblock));
    // Copia la clave proporcionada en el bloque de clave
    memcpy(key_block, &key, sizeof(long));
  
    // Crea un horario de clave DES basado en la clave
    DES_set_key_unchecked(&key_block, &schedule);
    // Realiza una encriptación DES en el texto
    for (int i = 0; i < len; i += 8) {
        // printf("Encrypting block %d\n", i/8);
        DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &schedule, DES_ENCRYPT);
    }
}

char search[] = "es una prueba de";

int tryKey(long key, char *ciph, int len){
  char temp[len+1];
  memcpy(temp, ciph, len);
  temp[len]=0;
  decrypt(key, temp, len);
  
  // Busca la cadena " the " en el texto desencriptado
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

  // Determina la longitud del archivo
  fseek(file, 0, SEEK_END);
  *length = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Asigna memoria para el texto basado en la longitud del archivo
  *text = (char *)malloc(*length);
  if (*text == NULL) {
    perror("Error de asignación de memoria");
    fclose(file);
    return 0;
  }

  // Lee el contenido del archivo en el texto
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

  // Escribe el contenido del texto en el archivo
  fwrite(text, 1, length, file);
  fclose(file);

  return 1;
}

int main(int argc, char *argv[]){
  int N, id;
  long upper = (1L << 56); // Límite superior para claves DES: 2^56
  long mylower, myupper;
  MPI_Status st;
  MPI_Request req;
  int flag;
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  double start_time, end_time; // Variables para medir el tiempo de ejecución
  long encryptionKey = 123456L; // Valor predeterminado para la clave de encriptación

  int option;

  // Procesa las opciones de la línea de comandos
  while ((option = getopt(argc, argv, "k:")) != -1) {
    switch (option) {
      case 'k':
        encryptionKey = atol(optarg);
        break;
      default:
        fprintf(stderr, "Uso: %s [-k key]\n", argv[0]);
        MPI_Finalize();
        exit(1);
    }
  }

  // Inicia el contador de tiempo
  start_time = MPI_Wtime();

  // Calcula el rango de claves que cada proceso MPI debe buscar
  int range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id+1) - 1;
  if (id == N - 1) {
    // Compensar el residuo
    myupper = upper;
  }

  char *text; // Texto para encriptar y desencriptar
  int textLength;

  if (id == 0) {
    // Carga el texto desde un archivo (por ejemplo, "input.txt")
    if (!loadTextFromFile("input.txt", &text, &textLength)) {
      MPI_Finalize();
      return 1;
    }

    encrypt(encryptionKey, text, textLength);

    // Guarda el texto encriptado en un archivo (por ejemplo, "encrypted.txt")
    if (!saveTextToFile("encrypted.txt", text, textLength)) {
      free(text);
      MPI_Finalize();
      return 1;
    }
  }

  // Difunde el texto encriptado a todos los nodos
  MPI_Bcast(&textLength, 1, MPI_INT, 0, comm);
  if (id != 0) {
    text = (char *)malloc(textLength);
  }
  MPI_Bcast(text, textLength, MPI_CHAR, 0, comm);

  long found = 0;

  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

  #pragma omp parallel num_threads(4) // Definir el número de hilos que deseas utilizar
  {
    int tid = omp_get_thread_num();
    long local_found = 0;

    // Cada hilo procesa su rango de claves
    for (long i = mylower + tid; i < myupper && (local_found == 0); i += 4) {
      if (tryKey(i, text, textLength)) {
        local_found = i;
      }
    }

    // Actualizar la variable "found" en una sección crítica
    #pragma omp critical
    {
      if (local_found != 0 && found == 0) {
        found = local_found;
      }
    }
  }

  // Termina el contador de tiempo
  end_time = MPI_Wtime();

  if (id == 0) {
    MPI_Wait(&req, &st);
    decrypt(found, text, textLength);

    // Imprime el texto desencriptado y la clave de encriptación
    printf("Tiempo de ejecución: %f segundos\n", end_time - start_time);

    // Guarda el texto desencriptado en un archivo (por ejemplo, "decrypted.txt")
    if (!saveTextToFile("decrypted.txt", text, textLength)) {
      free(text);
      MPI_Finalize();
      return 1;
    }
  }

  free(text);

  MPI_Finalize();
}