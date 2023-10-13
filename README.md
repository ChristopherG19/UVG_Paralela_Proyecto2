Universidad del Valle de Guatemala <br>
Facultad de Ingeniería <br>
Departamento de Ciencias de la Computación <br>
Programación Distribuida y Paralela CC3069 <br>
Ing. Sebastián Galindo 

# UVG_Paralela_Proyecto2

## Integrantes 
- Christopher García 20541
- Andrea Lam 20102
- Ma. Isabel Solano 20504

## Antecedentes
MPI es el estándar para el modelo paralelo de memoria distribuida. Debido a su interoperabilidad y portabilidad, MPI nos permite interconectar máquinas para crear conjuntos
computacionales homogéneos y heterogéneos. El paradigma de intercambio de mensajes nos permite interconectar máquinas en área local sin depender de la topología de la red de interconexión. Esto hace que MPI sea muy escalable

## Objetivos del proyecto
- Implementa y diseña programas para la paralelización de procesos con memoria distribuida usando Open MPI.
- Optimizar el uso de recursos distribuidos y mejorar el speedup de un programa paralelo.
- Descubrir la llave privada usada para cifrar un texto, usando el método de fuerza bruta (bruteforce).
- Comprender y Analizar el comportamiento del speedup de forma estadística

## Dependencias
Antes de utilizar el programa es recomendable instalar las siguientes librerías y ejecutar los siguientes comandos. 
- `sudo apt update`
- `sudo apt install mpich`
- `sudo apt install lam4-dev`
- `sudo apt install libhdf5-dev`
- `sudo apt install libhdf5-mpich-dev`
- `sudo apt install libhdf5-openmpi-dev`
- `sudo apt install openmpi-bin`

## Modo de ejecución
- [`bruteforce.c`](https://github.com/ChristopherG19/UVG_Paralela_Proyecto2/blob/main/bruteforce.c) <br>
Compilación: `mpicc -o bruteforce bruteforce.c -lcrypto -lssl -w` <br>
Ejecución: `mpirun -np 4 ./bruteforce -k <llave>`
- [`secuencial_des.c`](https://github.com/ChristopherG19/UVG_Paralela_Proyecto2/blob/main/secuencial_des.c) <br>
Compilación: `gcc -o secuencial_des secuencial_des.c -lcrypto -lssl -w` <br>
Ejecución: `./secuencial_des -k <llave>`
