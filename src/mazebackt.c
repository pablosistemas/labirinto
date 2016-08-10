/*
 * COMMENTS
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h> //bzero
#include <pthread.h>
#include <unistd.h> // usleep, getopt
#include <signal.h> // sig_atomic_t
#include <semaphore.h>

// saida em cores
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
}

/* UUT */
int verifica_posicao(int x, int y, char *maze) {

   // posicao na matriz
   int offset = x*ncol + y;

   // condicoes de contorno
   if(x >= nrow || y >= ncol || x < 0 || y < 0) 
      return -1;

   // else parede    
   else if(maze[offset] == '#') 
      return -1;

   // else saida
   else if(maze[offset] == 's') {
		printf("Saida encontrada em: %d %d\n",x,y);
      return 1;
	}

   // se caminho nao marcado, marca 
   else if(maze[offset] == 'x' || maze[offset] == 'e') {
      maze[offset] = 'o';

      imprime_maze(maze);
      
      // 0.1s
      useconds_t atraso = 500000;
      usleep(atraso);

      maze[offset] = '-';

      return 0;
   }
    
   //fprintf(stderr,"I'm done!\n");
   return -1;
}

int backtracking(int x, int y, char *maze) {
   int saida;
   if ((saida = verifica_posicao(x,y,maze)) == 1)
      return 1;
   // else if (saida == -1) return -1;
   else if(saida == 0) {

      // anda para frente
      if((saida = backtracking(x+1,y,maze)) == 1) return 1;
      //else if(saida == -1) return -1;  

      // anda para tras
      if((saida = backtracking(x-1,y,maze)) == 1) return 1;
      //else if(saida == -1) return -1;  

      // anda para esquerda
      if((saida = backtracking(x,y-1,maze)) == 1) return 1;   
      //else if(saida == -1) return -1;  

      // anda para direita
      if((saida = backtracking(x,y+1,maze)) == 1) return 1;
      //else if(saida == -1) return -1;  
   }
   return -1;
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

   printf("O entrada eh em: %d %d\n",entrada[0],entrada[1]);
   printf("O saida eh em: %d %d\n",saida[0],saida[1]);

   // função recursiva para caminha no labirinto
   int saida = backtracking(entrada[0], entrada[1], maze);

   /*if(saida)
      printf("Ha saida\n");
   else
      printf("Nao ha saida\n");*/

   desaloca_maze(maze);

   return 0;
}
