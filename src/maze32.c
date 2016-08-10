/*
	Este programa foi testado em ubuntu 32bits. Aparentemente a funcao
pthread_detach apresenta erros em versoes 64bits para threads alocadas.
A thread principal espera as demais threads (detached) pelo uso de 
semaforos, isto é, utilizando uma espera ocupada, a thr principal testa 
o semaforo enquanto seu valor permanece maior que 0, sinalizando que
ainda ha threads trabalhando.

*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h> //bzero
#include <pthread.h>
#include <unistd.h> // usleep, getopt
#include <signal.h> // sig_atomic_t
#include <semaphore.h>

// saida em cores
#define red   "\033[0;31m"        /* 0 -> normal ;  31 -> red */
#define cyan  "\033[1;36m"        /* 1 -> bold ;  36 -> cyan */
#define green "\033[4;32m"        /* 4 -> underline ;  32 -> green */
#define blue  "\033[9;34m"        /* 9 -> strike ;  34 -> blue */
 
#define black  "\033[0;30m"
#define brown  "\033[0;33m"
#define magenta  "\033[0;35m"
#define gray  "\033[0;37m"
  
#define none   "\033[0m"        /* to flush the previous property */

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"  

// variaveis globais
char *nomeArq = "maze1.txt";

// armazena os indices da entrada e saida do labirinto
int entrada[2];
int saida[2];
int nrow, ncol;

// encontrou a saida? Para verificacao das threads 
// tipo especial garante atomicidade nas atribuições
sig_atomic_t is_finished = 0;

// semaforo
sem_t sem;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct pos {
   // posicao atual no labirinto
   int linha;
   int coluna;
   // ponteiro para estrutura do labirinto
   char *maze;
};

// prototipos das funcoes
int anda_frente(struct pos *);
int anda_tras(struct pos *);
int anda_cima(struct pos *);
int anda_baixo(struct pos *);


void proc_args(int argc, char **argv) {
   int opt;
   while((opt = getopt(argc,argv,"f:h")) != -1) {
      switch(opt) {
         case 'f':
         nomeArq = optarg;
         break;
         case 'h':
         default:
            fprintf(stderr,
            "Usage:\n\t[-f nome do arquivo]\n\t[-h help]\n");
            exit(EXIT_FAILURE);
      }
   }
}

// aloca a matriz linearmente. 
// Conversao: offset = i*ncol+j
char *aloca_maze() {
   char *maze = (char*)malloc(nrow*ncol);
   if(maze == NULL){
      perror("erro de alocacao\n");
      exit(-1);
   }
   return maze;
}

void preenche_maze(FILE *arq, char *maze){
   // le as linhas da matriz
   int i=0, j=0;
   int offset;

   char tmp;
   while((fscanf(arq,"%c",&tmp)) != EOF) {
      if(tmp != '\n') {
         if(tmp == 'e') {
            entrada[0] = i;
            entrada[1] = j;
         } else if(tmp == 's') {
            saida[0] = i;
            saida[1] = j;
         }
         // posicao na matriz
         offset = i*ncol + j;
         maze[offset] = tmp;
         j+=1; i+=j/ncol; j=j%ncol;
      }
   }
}

void imprime_maze(char *maze) {

   pthread_mutex_lock(&mutex);
   system("clear");
   char cor [15];
   // armazena a posicao
   int offset;

   for(int i=0;i<nrow;i++){
      for(int j=0;j<ncol;j++) {
         offset = i*ncol + j;

         // ponto atual em vermelho
         if(maze[offset] == 'o') 
            fprintf(stdout, KRED "%c",maze[offset]);   
          
         // paredes azuis
         else if(maze[offset] == '#') 
            fprintf(stdout, KBLU "%c",maze[offset]);   

         // caminho em verde
         else if(maze[offset] == 'x') 
            fprintf(stdout, KGRN "%c",maze[offset]);   

         // caminho percorrido em magenta
         else /*if(maze[offset] == '-')*/ 
            fprintf(stdout, KMAG "%c",maze[offset]);   
      }
      printf("\n");
   }
   pthread_mutex_unlock(&mutex);
}

struct pos *novo_pos(int i, int j, char *maze) {
   struct pos *npos = malloc(sizeof(struct pos));
   npos->linha    = i;
   npos->coluna   = j;
   npos->maze     = maze;
   return npos;
}

// aloca e seta thread como detached
pthread_t *nova_thread() {
   pthread_t *nova = malloc(sizeof(pthread_t));
   pthread_detach(*nova);
	// incrementa o semaforo
	sem_post(&sem);
   return nova;
}

/* UUT */
int verifica_posicao(struct pos *posicao) {
   // testa se alguma outra thread encontrou a saida 
   if (is_finished) return -1;

   // testes em exclusao mutua
   // tranca mutex
   pthread_mutex_lock(&mutex);

   // posicao na matriz
   int offset = posicao->linha*ncol + posicao->coluna;

   // condicoes de contorno
   if(posicao->linha >= nrow || posicao->coluna >= ncol ||
      posicao->linha < 0 || posicao->coluna < 0) 
      goto FIM_FUNCAO; 
   // else parede    
   else if(posicao->maze[offset] == '#') 
      goto FIM_FUNCAO;
   // else saida
   else if(posicao->maze[offset] == 's') {
		printf("Saida encontrada e: %d %d\n",posicao->linha,posicao->coluna);
      is_finished = 1;
      
      // retorna sucesso! Encontrou a saida
      pthread_mutex_unlock(&mutex);
      return 1;

   // se caminho nao marcado, marca 
   } else if(posicao->maze[offset] == 'x' ||
         posicao->maze[offset] == 'e') {
      posicao->maze[offset] = 'o';

      // destranca o mutex e imprime 
      pthread_mutex_unlock(&mutex);

      imprime_maze(posicao->maze);
      
      // 0.1s
      useconds_t atraso = 500000;
      usleep(atraso);

      // atualiza status da posição para ir para prox
      pthread_mutex_lock(&mutex);
      posicao->maze[offset] = '-';
      pthread_mutex_unlock(&mutex);

      return 0;
   } 
    
FIM_FUNCAO:     
   // destranca mutex
   pthread_mutex_unlock(&mutex);
   return -1;
}

int anda_baixo(struct pos *pos) {
   int *i      = &pos->linha;
   int *j      = &pos->coluna;
   char *maze  = pos->maze;

   pthread_t *thread;
   struct pos *npos;
   
   int ret;
   while((ret = verifica_posicao(pos)) == 0) {
      // frente
      npos     = novo_pos(*i, *j+1, maze);
      thread   = nova_thread();
      pthread_create(thread,NULL,(void*)anda_frente,(void*)npos);
      // tras
      npos     = novo_pos(*i, *j-1, maze);
      thread   = nova_thread();
      pthread_create(thread,NULL,(void*)anda_tras,(void*)npos);

      //nao anda pra cima pois veio de la 
      //anda pra baixo
      (*i)--;
   }
   free(pos);
	// libera semaforo qdo thread termina
	sem_wait(&sem);
   return ret;
}

int anda_cima(struct pos *pos) {
   int *i      = &pos->linha;
   int *j      = &pos->coluna;
   char *maze  = pos->maze;

   pthread_t *thread;
   struct pos *npos;
   
   int ret;
   while((ret = verifica_posicao(pos)) == 0) {
      // frente
      npos     = novo_pos(*i, *j+1, maze);
      thread   = nova_thread();
      pthread_create(thread,NULL,(void*)anda_frente,(void*)npos);
      // tras
      npos     = novo_pos(*i, *j-1, maze);
      thread   = nova_thread();
      pthread_create(thread,NULL,(void*)anda_tras,(void*)npos);

      //nao anda pra baixo pois veio de la 
      //anda pra cima
      (*i)++;
   }

   free(pos);
	// libera semaforo qdo thread termina
	sem_wait(&sem);
   return ret;
}

int anda_tras(struct pos *pos) {
   int *i      = &pos->linha;
   int *j      = &pos->coluna;
   char *maze  = pos->maze;

   pthread_t *thread;
   struct pos *npos;
   
   int ret;
   while((ret = verifica_posicao(pos)) == 0) {
      // cima
      npos     = novo_pos(*i+1, *j, maze);
      thread   = nova_thread();
      pthread_create(thread,NULL,(void *)anda_cima,
               (void *)npos);
      // baixo
      npos     = novo_pos(*i-1, *j, maze);
      thread   = nova_thread();
      pthread_create(thread,NULL,(void *)anda_baixo,
               (void *)npos);

      //nao anda pra frente pois veio de la 
      //anda pra tras 
      (*j)--;
   }

   free(pos);
	// libera semaforo qdo thread termina
	sem_wait(&sem);
   return ret;
}

int anda_frente(struct pos *pos) {
   int *i      = &pos->linha;
   int *j      = &pos->coluna;
   char *maze  = pos->maze;

   pthread_t *thread;
   struct pos *npos;
   
   int ret = verifica_posicao(pos);
   while(ret == 0) {
      // cima
      npos     = novo_pos(*i+1, *j, maze);
      thread   = nova_thread();
      pthread_create(thread,NULL,(void *)anda_cima,
               (void *)npos);
      // baixo
      npos     = novo_pos(*i-1, *j, maze);
      thread   = nova_thread();
      pthread_create(thread,NULL,(void *)anda_baixo,
               (void *)npos);

      //nao anda pra tras pois veio de la 
      //anda pra frente
      (*j)++;
		ret = verifica_posicao(pos);
   }
   free(pos);
	// libera semaforo qdo thread termina
	sem_wait(&sem);
   return ret;
}

char *le_arq_aloca_preenche_maze(){

   FILE *arq = fopen(nomeArq,"r");
   
   if(arq == NULL) {
      fprintf(stderr,"arquivo nao encontrado\n");
      exit(EXIT_FAILURE);
   }

   // le o tam da matriz 
   fscanf(arq,"%d %d\n",&nrow,&ncol);

   // aloca a estrutura de dados labirinto
   char *maze = aloca_maze();

   // le o arquivo e preenche a matrix 
   preenche_maze(arq, maze);

   //imprime_maze(maze);

   fclose(arq);

   return maze;
}

// desaloca memoria ocupada pelo maze
void desaloca_maze(char *maze) {
   free(maze);
}

int main(int argc, char **argv){

   // processa argumentos
   proc_args(argc,argv);

   // le e chama funcao para preencher a matriz
   char *maze = le_arq_aloca_preenche_maze();

   //imprime_maze(maze);
	// inicializa o semaforo: 0 indica compartilhado entre threads do proc
	sem_init(&sem, 0, 0);

   printf("O entrada eh em: %d %d\n",entrada[0],entrada[1]);
   printf("O saida eh em: %d %d\n",saida[0],saida[1]);

   pthread_t *thread;
   
   struct pos *npos;

   // cima da entrada
   npos     = novo_pos(entrada[0]+1,entrada[1],maze);
   thread   = nova_thread();
   pthread_create(thread,NULL,(void*)anda_cima,npos);

   // baixo
   npos     = novo_pos(entrada[0]-1,entrada[1],maze);
   thread   = nova_thread();
   pthread_create(thread,NULL,(void*)anda_baixo,npos);

   // frente a partir da entrada
   npos     = novo_pos(entrada[0],entrada[1],maze);
   thread   = nova_thread();
   pthread_create(thread,NULL,(void*)anda_frente,npos);

   // tras
   npos     = novo_pos(entrada[0],entrada[1]-1,maze);
   thread   = nova_thread();
   pthread_create(thread,NULL,(void*)anda_tras,npos);

	// enquanto sval > 0 ha threads	
	int sval;
	do {
		sem_getvalue(&sem,&sval);
      // sleep para diminuir overhead da espera ocupada
      sleep(1);
	} while(sval > 0);

   desaloca_maze(maze);

   return 0;
}
