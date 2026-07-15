#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* Função auxiliar para dormir microssegundos usando nanosleep */
static void usleep_safe(unsigned int usec) {
  struct timespec req = {.tv_sec = usec / 1000000,
    .tv_nsec = (usec % 1000000) * 1000
  };
  // continua se interrompido por sinal
  while (nanosleep(&req, &req) == -1 && errno == EINTR);
}

#define N 8
#define K 3
#define RODADAS 4

typedef struct {
  int servidor_id;
  int rodada;
  int tamanho_mb;
  int tentativas;
  int sucesso;
} EntradaManifesto;

EntradaManifesto manifesto[200];
int n_entradas = 0;
pthread_mutex_t manifesto_mtx = PTHREAD_MUTEX_INITIALIZER;
sem_t rede;
pthread_barrier_t barreira;

int max_tentativas = 0;
int max_agente = -1;
int max_rodada = -1;

void* agente(void* arg) {
  int id = *(int*)arg;
  unsigned int seed = (unsigned int)(time(NULL) ^ (id + 1) ^ getpid());

  for (int rodada = 1; rodada <= RODADAS; rodada++) {
    int tentativas = 0;
    int falhou = 1;

    while (falhou) {
      tentativas++;
      sem_wait(&rede);

      usleep_safe(100000 + (rand_r(&seed) % 400000));
      int tamanho = 100 + (rand_r(&seed) % 4901);
      falhou = (rand_r(&seed) % 5 == 0);

      sem_post(&rede);

      pthread_mutex_lock(&manifesto_mtx);
      if (n_entradas < 200) {
        manifesto[n_entradas].servidor_id = id;
        manifesto[n_entradas].rodada = rodada;
        manifesto[n_entradas].tamanho_mb = tamanho;
        manifesto[n_entradas].tentativas = tentativas;
        manifesto[n_entradas].sucesso = falhou ? 0 : 1;
        n_entradas++;

        if (!falhou && tentativas > max_tentativas) {
          max_tentativas = tentativas;
          max_agente = id;
          max_rodada = rodada;
        }
      } else {
          // Tratamento de erro: manifesto cheio
          fprintf(stderr, "ERRO: Manifesto cheio (n_entradas >= 200). Encerrando.\n");
          exit(EXIT_FAILURE);
      }
      pthread_mutex_unlock(&manifesto_mtx);

      if (falhou) {
        printf("Agente %d [rodada %d]: FALHA (tentativa %d), retentando...\n",
                id, rodada, tentativas);
        usleep_safe(50000);
      } else {
          printf("Agente %d [rodada %d]: OK (%d MB, %d tentativas)\n",
                  id, rodada, tamanho, tentativas);
          break;
      }
    }

    pthread_barrier_wait(&barreira);

    if (id == 0) {
      int total_mb = 0;
      int total_falhas = 0;
        for (int i = 0; i < n_entradas; i++) {
          if (manifesto[i].rodada == rodada) {
            if (manifesto[i].sucesso)
              total_mb += manifesto[i].tamanho_mb;
            else
              total_falhas++;
          }
        }
      printf("\n=== Resumo da Rodada %d ===\n", rodada);
      printf("Total MB transferidos: %d\n", total_mb);
      printf("Total falhas: %d\n", total_falhas);
      printf("==========================\n\n");
    }

    pthread_barrier_wait(&barreira);
  }
  return NULL;
}

int main(void) {
  pthread_t threads[N];
  int ids[N];

  sem_init(&rede, 0, K);
  pthread_barrier_init(&barreira, NULL, N);
  pthread_mutex_init(&manifesto_mtx, NULL);

  for (int i = 0; i < N; i++) {
    ids[i] = i;
    pthread_create(&threads[i], NULL, agente, &ids[i]);
  }

  for (int i = 0; i < N; i++)
    pthread_join(threads[i], NULL);

  int total_entradas = n_entradas;
  int total_sucesso = 0, total_falhas = 0;
  int volume_total = 0;
  for (int i = 0; i < n_entradas; i++) {
    if (manifesto[i].sucesso) {
      total_sucesso++;
      volume_total += manifesto[i].tamanho_mb;
    } else {
        total_falhas++;
    }
  }
  double taxa_falha = (total_entradas > 0) ?
                      ((double)total_falhas / total_entradas * 100.0) : 0.0;

  printf("\n=== Relatório de Backup ===\n");
  printf("Rodadas concluídas: %d\n", RODADAS);
  printf("Total de entradas no manifesto: %d\n", total_entradas);
  printf("Backups bem-sucedidos: %d\n", total_sucesso);
  printf("Falhas parciais registradas: %d\n", total_falhas);
  printf("Volume total transferido: %d MB\n", volume_total);
  printf("Agente com mais retentativas: Agente %d (%d tentativas na rodada %d)\n",
          max_agente, max_tentativas, max_rodada);
  printf("Taxa de falha geral: %.1f%%\n", taxa_falha);

  sem_destroy(&rede);
  pthread_barrier_destroy(&barreira);
  pthread_mutex_destroy(&manifesto_mtx);
  return 0;
}
