# Sistema de Backup Distribuído — Exercício A (LPII 2026.1)

**Aluno:** William Torres Pereira da Silva  
**Matrícula:** 20210026160  
**Disciplina:** LPII – Programação Concorrente: Fundamentos  
**Professor:** Carlos Eduardo Batista  

---

## Descrição

Este programa implementa um sistema de backup distribuído com 8 agentes (threads) que realizam 4 rodadas de backup. O sistema simula:

- **Largura de banda limitada** – no máximo 3 backups simultâneos (semáforo de contagem).
- **Rodadas sincronizadas** – a próxima rodada só começa quando todos os agentes concluíram a atual (barreira).
- **Manifesto compartilhado** – cada tentativa (sucesso ou falha) é registrada em um vetor protegido por mutex.
- **Falha e retry** – cada backup tem 20% de chance de falhar; em caso de falha, o agente repete a tentativa na mesma rodada até ter sucesso.

Ao final, o programa exibe um relatório consolidado com estatísticas (volume total, taxa de falha, agente com mais retentativas, etc.).

---

## Compilação e Execução

### Requisitos

- Compilador GCC com suporte a pthreads.
- Sistema Linux/Unix ou WSL.

### Passos

```bash
# Compilar
gcc -Wall -Wextra -pthread -o backup backup.c

# Executar
./backup
