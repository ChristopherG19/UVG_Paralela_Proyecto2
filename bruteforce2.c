#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

void decrypt(long key, char *ciph, int len){
  DES_cblock key_block;
  DES_key_schedule schedule;
  
  memset(key_block, 0, sizeof(DES_cblock));
  memcpy(key_block, &key, sizeof(long));  // copy the key into the key_block
  
  DES_set_key_unchecked(&key_block, &schedule);
  DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_DECRYPT);
}

void encrypt(long key, char *ciph, int len){
  DES_key_schedule schedule;
  DES_cblock key_block;
  memcpy(key_block, &key, sizeof(key));  // copy the key into the key_block
  DES_set_key_unchecked(&key_block, &schedule);
  DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_ENCRYPT);
}

char search[] = " the ";

int tryKey(long key, char *ciph, int len){
  char temp[len+1];
  memcpy(temp, ciph, len);
  temp[len]=0;
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

int loadTextFromFile(const char *filename, char **text, int *length) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("Error opening file");
    return 0;
  }

  fseek(file, 0, SEEK_END);
  *length = ftell(file);
  fseek(file, 0, SEEK_SET);

  *text = (char *)malloc(*length);
  if (*text == NULL) {
    perror("Memory allocation error");
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
    perror("Error opening file");
    return 0;
  }

  fwrite(text, 1, length, file);
  fclose(file);

  return 1;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};

int main(int argc, char *argv[]){
  int N, id;
  long upper = (1L << 56); // upper bound DES keys 2^56
  long mylower, myupper;
  MPI_Status st;
  MPI_Request req;
  int flag;
  int ciphlen = strlen(cipher);
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  int range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id+1) - 1;
  if (id == N - 1) {
    // Compensar residuo
    myupper = upper;
  }

  char *text; // Text to encrypt and decrypt
  int textLength;

  if (id == 0) {
    // Load text from a file (e.g., "input.txt")
    if (!loadTextFromFile("input.txt", &text, &textLength)) {
      MPI_Finalize();
      return 1;
    }

    // Encrypt the loaded text using a key (e.g., 42)
    long encryptionKey = 42;
    encrypt(encryptionKey, text, textLength);

    // Save the encrypted text to a file (e.g., "encrypted.txt")
    if (!saveTextToFile("encrypted.txt", text, textLength)) {
      free(text);
      MPI_Finalize();
      return 1;
    }
  }

  // Broadcast the encrypted text to all nodes
  MPI_Bcast(&textLength, 1, MPI_INT, 0, comm);
  if (id != 0) {
    text = (char *)malloc(textLength);
  }
  MPI_Bcast(text, textLength, MPI_CHAR, 0, comm);

  long found = 0;

  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

  for (int i = mylower; i < myupper && (found == 0); ++i) {
    if (tryKey(i, text, textLength)) {
      found = i;
      for (int node = 0; node < N; node++) {
        MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
      }
      break;
    }
  }

  if (id == 0) {
    MPI_Wait(&req, &st);
    decrypt(found, text, textLength);

    // Print the decrypted text and the encryption key
    printf("%li %s\n", found, text);

    // Save the decrypted text to a file (e.g., "decrypted.txt")
    if (!saveTextToFile("decrypted.txt", text, textLength)) {
      free(text);
      MPI_Finalize();
      return 1;
    }
  }

  free(text);

  MPI_Finalize();
}
