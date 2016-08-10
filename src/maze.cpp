#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <stdlib.h>
#include <ctime>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <stdio.h>

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
std::string nomeArq("maze1.txt");

// ponteiro para estrutura labirinto
char **maze;

// armazena os indices da entrada e saida do labirinto
int entrada[2];
int saida[2];
int nrow, ncol;

// encontrou a saida? Para verificacao das threads 
// tipo especial garante atomicidade nas atribuições
std::sig_atomic_t is_finished = 0;

std::mutex mtx;

/*struct pos {
   int linha;
   int coluna;
};*/

class Pos {
   //posicao
   int lin, col;
   // valor de retorno
   int ret;

   public:
   Pos(): lin(0), col(0) {}
   Pos(int l, int c): lin(l), col(c) {}
   /* Setters */
   void setLinha(int l) { lin = l; }
   void setColuna(int c) { col = c; }
   void setRet(int r) { ret = r; }
   /* Getters */
   int getLinha() const { return lin; }
   int getColuna() const { return col; }
   int getRet() const { return ret; }
   /* funcao p/ threads - percorre labirinto */
   void percorre_maze();
};

void preenche_maze(std::fstream &arq){
   // le as linhas da matriz
   int i=0, j=0;

   char tmp;
   //while((fscanf(arq,"%c",&tmp)) != EOF) {
   while(arq.get(tmp)) {
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

void imprime_maze() {

   mtx.lock();
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
   mtx.unlock();
}

void desaloca_maze() {
   for(int i=0;i<nrow;i++){
      free(maze[i]);
   }
   free(maze);
}


void le_arquivo(){

   // FILE *arq = fopen(nomeArq,"r");
   std::fstream arq(nomeArq,std::ios_base::in);
   
   // testa erro de abertura do arquivo
   if(( arq.rdstate() & std::ifstream::failbit ) != 0 ) {
      std::cerr << "arquivo nao encontrado" << std::endl;
      exit(EXIT_FAILURE);
   }

   // le o tam da matriz 
   arq >> nrow;
   arq >> ncol;
   std::cout<<"linhas: " << nrow <<" Colunas: "<< ncol <<std::endl;

   aloca_maze();

   // le o arquivo e preenche a matrix 
   preenche_maze(arq);

   imprime_maze();

   arq.close();
}

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

/* UUT */
void Pos::percorre_maze() {
   int linha = getLinha();
   int coluna = getColuna();
   // testa se alguma outra thread encontrou a saida 
   if (is_finished) {
      exit(EXIT_FAILURE);
   }

   // testes em exclusao mutua
   // tranca mutex
   mtx.lock();

   // condicoes de contorno
   if(linha >= nrow || coluna >= ncol || linha < 0 || coluna < 0) {
      goto FIM_FUNCAO; 
   // else parede    
   } else if(maze[linha][coluna] == '#') {
      goto FIM_FUNCAO;
   // else saida
   } else if(maze[linha][coluna] == 's') {
      printf("saida encontrada em: %d %d\n",linha,coluna);
      is_finished = 1;
      goto FIM_FUNCAO;
   // se caminho nao marcado, marca 
   } else if(maze[linha][coluna]=='x' || maze[linha][coluna]=='e') {
      maze[linha][coluna] = 'o';

      // destranca o mutex e imprime 
      mtx.unlock();

      imprime_maze();
      
      // 0.1s
      //useconds_t atraso = 500000000;
      struct timespec atraso{0, 500000000};
      nanosleep(&atraso,NULL); //usleep(atraso);

      // atualiza status da posição para ir para prox
      mtx.lock();
      maze[linha][coluna] = '-';
      mtx.unlock();

      // uma thread para cada direcao
      std::vector<std::thread> threads;
      
      // lança todas as threads
      Pos nova_pos0;
      // anda pra frente
      nova_pos0.setLinha(linha);
      nova_pos0.setColuna(coluna+1);
      // cria e lança a thread
      threads.push_back(std::thread(&Pos::percorre_maze,&nova_pos0));
     
      Pos nova_pos1;
      // anda pra cima
      nova_pos1.setLinha(linha-1);
      nova_pos1.setColuna(coluna);
      // cria e lança a thread
      threads.push_back(std::thread(&Pos::percorre_maze,&nova_pos1));
     
      Pos nova_pos2;
      // anda pra baixo
      nova_pos2.setLinha(linha+1);
      nova_pos2.setColuna(coluna);
      // cria e lança a thread
      threads.push_back(std::thread(&Pos::percorre_maze,&nova_pos2));
     
      Pos nova_pos3;
      // anda pra tras
      nova_pos3.setLinha(linha);
      nova_pos3.setColuna(coluna-1);
      // cria e lança a thread
      threads.push_back(std::thread(&Pos::percorre_maze,&nova_pos3));

      for(auto& th : threads) th.join();

      int all = nova_pos0.getRet() + nova_pos1.getRet() + 
            nova_pos2.getRet() + nova_pos3.getRet();
      // retorna se algum encontrou a saida
      if(all) {
         setRet(1);
         return;
      } else {
         setRet(0);
         return;
      }
   }
    
FIM_FUNCAO:     
   // destranca mutex
   mtx.unlock();
   setRet(0);
   return;
}

int main(int argc, char **argv) {
   
   // processa argumentos
   proc_args(argc,argv);

   le_arquivo();

   printf("O entrada eh em: %d %d\n",entrada[0],entrada[1]);
   printf("O saida eh em: %d %d\n",saida[0],saida[1]);

   /* Thread principal */ 
   Pos my_pos;
   my_pos.setLinha(entrada[0]); 
   my_pos.setColuna(entrada[1]); 

   std::thread t(&Pos::percorre_maze,&my_pos);
   t.join();

   desaloca_maze();

   return 0;
}
