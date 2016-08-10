# Problema do labirinto - percurso do labirinto *singlethreading* e *multithreading*
## maze64.c: 
versão lança uma thread para cada passo no caminho. Testado no
Fedora 64bits e Ubuntu 32bits.

## maze32.c: 
versão mais leve pois lança menos threads no total. Possivel 
problema com *pthread_detach()* para ponteiros do tipo *pthread_t\** 
impediu os testes no Fedora 64bits. Testado no Ubuntu 32bits.

## mazebackt.c: 
resolve o problema o labirinto implementando função de 
*backtracking* (recursiva). Não lança nenhuma thread. Testado no Fedora
64bits e Ubuntu 32bits.

## maze.cpp: 
Implementado utilizando biblioteca std::thread do C++. Testado
no Fedora 64bits e Ubuntu 32bits.

## Tutorial:

Há um arquivo Makefile para automatizar a compilação:
Compila todos:
* make

 Compila e gera executavel da a versão "pesada":
* make make64 && /maze64 -h 

Compila e gera executavel da a versão "leve":
 * make make32 && ./maze32 -h

Compila e gera executavel da versão mono-thread:
 * make makebt && ./mazebt -h

Compila e gera executavel do programa em C++	
 * make makecpp &&./mazecpp -h

apaga executaveis gerados:
 * make clean
 - 
