#include <stdio.h>
#include <stdlib.h>
#include <strings.h> //bzero
#include <pthread.h>
#include <unistd.h> // usleep, getopt
#include <signal.h> // sig_atomic_t

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

// ponteiro para estrutura labirinto
char **maze;

// armazena os indices da entrada e saida do labirinto
int entrada[2];
int saida[2];
int nrow, ncol;

// encontrou a saida? Para verificacao das threads 
// tipo especial garante atomicidade nas atribuições
sig_atomic_t is_finished = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct pos {
   int linha;
   int coluna;
};

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

void aloca_maze() {
   maze = (char**)malloc(nrow*sizeof(char*));
   if(maze == NULL){
      perror("erro de alocacao\n");
      exit(-1);
   }
   for(int i=0; i < nrow; i++){
      maze[i] = (char*)malloc(ncol);
      if(maze[i] == NULL) {
         perror("erro de alocacao\n");
         exit(-1);
      } else {
         // zera a matriz
         bzero((void *)maze[i],ncol);
      }
   }
   return;
}

void preenche_maze(FILE *arq){
   // le as linhas da matriz
   int i=0, j=0;

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
         maze[i][j] = tmp;
         j+=1; i+=j/ncol; j=j%ncol;
      }
   }
}

void imprime_maze() {

   pthread_mutex_lock(&mutex);
   system("clear");
   char cor [15];
   for(int i=0;i<nrow;i++){
      for(int j=0;j<ncol;j++) {
         // ponto atual em vermelho
         if(maze[i][j] == 'o') 
            fprintf(stdout, KRED "%c",maze[i][j]);   
          
         // paredes azuis
         else if(maze[i][j] == '#') 
            fprintf(stdout, KBLU "%c",maze[i][j]);   

         // caminho em verde
         else if(maze[i][j] == 'x') 
            fprintf(stdout, KGRN "%c",maze[i][j]);   

         // caminho percorrido em magenta
         else /*if(maze[i][j] == '-')*/ 
            fprintf(stdout, KMAG "%c",maze[i][j]);   
      }
      printf("\n");
   }
   pthread_mutex_unlock(&mutex);
}

void le_arquivo(){

   FILE *arq = fopen(nomeArq,"r");
   
   if(arq == NULL) {
      fprintf(stderr,"arquivo nao encontrado\n");
      exit(EXIT_FAILURE);
   }

   // le o tam da matriz 
   fscanf(arq,"%d %d\n",&nrow,&ncol);

   aloca_maze();

   // le o arquivo e preenche a matrix 
   preenche_maze(arq);

   //imprime_maze();

   fclose(arq);
}

/* UUT */
void percorre_maze(struct pos *posicao) {
   // testa se alguma outra thread encontrou a saida 
   if (is_finished) exit(EXIT_FAILURE);

   // testes em exclusao mutua
   // tranca mutex
   pthread_mutex_lock(&mutex);

   // condicoes de contorno
   if(posicao->linha >= nrow || posicao->coluna >= ncol ||
      posicao->linha < 0 || posicao->coluna < 0) 
      goto FIM_FUNCAO; 
   // else parede    
   else if(maze[posicao->linha][posicao->coluna] == '#') 
      goto FIM_FUNCAO;
   // else saida
   else if(maze[posicao->linha][posicao->coluna] == 's') {
      printf("saida encontrada em: %d %d\n",posicao->linha,
      posicao->coluna);
      is_finished = 1;
      goto FIM_FUNCAO;
   // se caminho nao marcado, marca 
   } else if(maze[posicao->linha][posicao->coluna] == 'x' ||
         maze[posicao->linha][posicao->coluna] == 'e') {
      maze[posicao->linha][posicao->coluna] = 'o';

      // destranca o mutex e imprime 
      pthread_mutex_unlock(&mutex);

      imprime_maze();
      
      // 0.1s
      useconds_t atraso = 500000;
      usleep(atraso);

      // atualiza status da posição para ir para prox
      pthread_mutex_lock(&mutex);
      maze[posicao->linha][posicao->coluna] = '-';
      pthread_mutex_unlock(&mutex);

      // uma thread para cada direcao
      pthread_t threads[4];
      
      struct pos nova_pos0;
      // anda pra frente
      nova_pos0.linha = posicao->linha;
      nova_pos0.coluna = posicao->coluna+1;
      pthread_create(threads,NULL,(void *)percorre_maze,
            (void *)&nova_pos0);
     
      struct pos nova_pos1;
      // anda pra cima
      nova_pos1.linha = posicao->linha-1;
      nova_pos1.coluna = posicao->coluna;
      pthread_create(threads+1,NULL,(void *)percorre_maze,
            (void *)&nova_pos1);
     
      struct pos nova_pos2;
      // anda pra baixo
      nova_pos2.linha = posicao->linha+1;
      nova_pos2.coluna = posicao->coluna;
      pthread_create(threads+2,NULL,(void *)percorre_maze,
            (void *)&nova_pos2);
     
      struct pos nova_pos3;
      // anda pra tras
      nova_pos3.linha = posicao->linha;
      nova_pos3.coluna = posicao->coluna-1;
      pthread_create(threads+3,NULL,(void *)percorre_maze,
            (void *)&nova_pos3);

      // espera threads para conferir valor de retorno
      int retval0,retval1,retval2,retval3;

      pthread_join(threads[0],(void*)&retval0);
      pthread_join(threads[1],(void*)&retval1);
      pthread_join(threads[2],(void*)&retval2);
      pthread_join(threads[3],(void*)&retval3);

      int ret = (int)retval0+(int)retval1+(int)retval2+(int)retval3;
      // retorna se algum encontrou a saida
      if(ret)
         pthread_exit((void*)1);
      else
         pthread_exit((void*)0);
   } 
    
FIM_FUNCAO:     
   // destranca mutex
   pthread_mutex_unlock(&mutex);
   pthread_exit((void*)0);
}

void desaloca_maze() {
   for(int i=0;i<nrow;i++){
      free(maze[i]);
   }
   free(maze);
}

int main(int argc, char **argv){

   // processa argumentos
   proc_args(argc,argv);

   // le e chama funcao para preencher a matriz
   le_arquivo();

   struct pos entrada_pos;
   entrada_pos.linha = entrada[0];
   entrada_pos.coluna = entrada[1];

   printf("O entrada eh em: %d %d\n",entrada[0],entrada[1]);
   printf("O saida eh em: %d %d\n",saida[0],saida[1]);

   percorre_maze(&entrada_pos);

   desaloca_maze();

   return 0;
}
