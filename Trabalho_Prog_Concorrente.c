#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define P 5     // Número de produtores
#define EP 5    // Número de empacotadores
#define E 3     // Número de entregadores

// Mutexes para controlar produção e entrega
pthread_mutex_t mutex_prod = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ent = PTHREAD_MUTEX_INITIALIZER;

// Condições para poder produzir e para iniciar empacotamento
pthread_cond_t cond_produzir = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_empacotar = PTHREAD_COND_INITIALIZER;
// Condições de entrega
pthread_cond_t cond_entregar = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_buscar = PTHREAD_COND_INITIALIZER;

// Variáveis para gerenciar produção
int id_p = 0; // ID do pedido produzido
int id_ep = 0; // ID do pedido empacotado
int pedidos_buscados = 0;
int producao_concluida = 0;
int empacotamento_concluido = 0;
// Variáveis para manipulação de entregas
int id_c = 0; // ID da carta buscada
int id_e = 0; // ID do pedido enviado
int buscando = 0;
int entregando = 0;

void *entregador_carta(void *arg) {
    int id = *((int *)arg);
    free(arg);
    while (1) {
        // Buscar carta caso não estejam entregando e nem queira entregar nenhum pedido.
        pthread_mutex_lock(&mutex_ent);
        while (entregando != 0 || empacotamento_concluido != 0) {
            pthread_cond_wait(&cond_buscar, &mutex_ent);
        }
        buscando++;
        id_c++;
        printf("Entregador %d: Buscando carta %d...\n", id, id_c);
        // Quando finaliza, acorda demais threads do entregador e inicia a produção.
        pedidos_buscados++;
        buscando--;
        pthread_cond_signal(&cond_produzir);
        pthread_cond_signal(&cond_entregar);
        pthread_mutex_unlock(&mutex_ent);
        sleep(1);
    }
    return NULL;
}

void *entregador_entrega(void *arg) {
    int id = *((int *)arg);
    free(arg);
    while (1) {
        // Esperar empacotamento e espera não estar buscando cartas .
        pthread_mutex_lock(&mutex_ent);
        while (empacotamento_concluido == 0 || buscando != 0) {
            pthread_cond_wait(&cond_entregar, &mutex_ent);
        }
        entregando++;
        id_e++;
        printf("Entregador %d: Entregando pedido %d...\n", id, id_e);
        // Quando termina de entregar ele acorda as threads do entregador.
        empacotamento_concluido--;
        entregando--;
        pthread_cond_signal(&cond_buscar);
        pthread_mutex_unlock(&mutex_ent);
        sleep(2);
    }
    return NULL;
}

void *producao(void *arg) {
    int id = *((int *)arg);
    free(arg);
    while (1) {
        // Inicia a produção quando há pedidos para produzir.
        pthread_mutex_lock(&mutex_ent);
        while (pedidos_buscados == 0) {
            printf("Produtor %d: Aguardando pedidos...\n", id);
            pthread_cond_wait(&cond_produzir, &mutex_ent);
        }
        pedidos_buscados--;
        pthread_mutex_unlock(&mutex_ent);
        
        sleep(1);
        // Quando finaliza  produção ele inicia o empacotamento.
        pthread_mutex_lock(&mutex_prod);
        id_p++;
        printf("Produtor %d: Produzindo pedido %d...\n", id, id_p);
        producao_concluida++;
        pthread_cond_signal(&cond_empacotar);
        pthread_mutex_unlock(&mutex_prod);
        
    }
    return NULL;
}

void *empacotamento(void *arg) {
    int id = *((int *)arg);
    free(arg); 
    while (1) {
        // Inicia o empacotamento depois da produção desse produto.
        pthread_mutex_lock(&mutex_prod);
        while (producao_concluida == 0) {
            printf("Empacotador %d: Aguardando produção...\n", id);
            pthread_cond_wait(&cond_empacotar, &mutex_prod);
        }
        producao_concluida--;
        pthread_mutex_unlock(&mutex_prod);

        // Quando finaliza o empacotamento, acorda o entregador para ser entregue.
        pthread_mutex_lock(&mutex_ent);
        id_ep++;
        printf("Empacotador %d: Empacotando pedido %d...\n", id, id_ep);
        empacotamento_concluido++;
        pthread_cond_signal(&cond_entregar);
        pthread_mutex_unlock(&mutex_ent);
        sleep(1);
    }
    return NULL;
}

int main() {
    
    // Cria threads com os tamanhos desejados
    pthread_t thread_produtores[P];
    pthread_t thread_empacotadores[EP];
    pthread_t thread_entregadores_buscando[E];
    pthread_t thread_entregadores_entregando[E];

    // Criar threads de produção.
    for (int i = 0; i < P; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&thread_produtores[i], NULL, producao, id);
    }

    // Criar threads de empacotamento.
    for (int i = 0; i < EP; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&thread_empacotadores[i], NULL, empacotamento, id);
    }

    // Criar threads de entrega(buscando cartas).
    for (int i = 0; i < E; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&thread_entregadores_buscando[i], NULL, entregador_carta, id);
    }

    // Criar threads de entrega(entregando pedidos).
    for (int i = 0; i < E; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&thread_entregadores_entregando[i], NULL, entregador_entrega, id);
    }

    // Aguardar threads.
    for (int i = 0; i < P; i++) {
        pthread_join(thread_produtores[i], NULL);
    }
    for (int i = 0; i < EP; i++) {
        pthread_join(thread_empacotadores[i], NULL);
    }
    for (int i = 0; i < E; i++) {
        pthread_join(thread_entregadores_buscando[i], NULL);
        pthread_join(thread_entregadores_entregando[i], NULL);
    }

    return 0;
}
