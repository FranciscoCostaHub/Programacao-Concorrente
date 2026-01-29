# MUDANÇAS DA PARTE A PARA PARTE B

## 📋 RESUMO DAS ALTERAÇÕES

### 1. ARGUMENTOS DA LINHA DE COMANDOS
**ANTES (Parte A):**
```bash
./process-photos-parallel-A <diretoria> <num_threads> <-name|-size>
```

**AGORA (Parte B):**
```bash
./process-photos-parallel-B <num_threads> <-name|-size>
```
⚠️ A diretoria agora é especificada DURANTE a execução (comando DIR)


### 2. ESTRUTURAS DE DADOS NOVAS

#### image_task_t
Estrutura enviada pelos pipes para as threads:
```c
typedef struct {
    char input_dir[MAX_PATH];   // Diretoria das imagens
    char output_dir[MAX_PATH];  // Diretoria Result-image-dir
    char filename[256];         // Nome da imagem
    int terminate;              // Flag para terminar (1 = sair)
} image_task_t;
```

#### statistics_t
Estatísticas partilhadas entre threads (protegidas por mutex):
```c
typedef struct {
    int total_images;           // Número total processado
    double total_time;          // Tempo total acumulado
    pthread_mutex_t mutex;      // Proteção de acesso
} statistics_t;
```

#### thread_data_t (MODIFICADA)
```c
typedef struct {
    int pipe_fd;                // FD para ler do pipe
    statistics_t *stats;        // Ponteiro para stats partilhadas
    int thread_id;
} thread_data_t;
```


### 3. COMUNICAÇÃO VIA PIPES

#### Criação dos Pipes (um por thread)
```c
int pipes[num_threads][2];
for (int i = 0; i < num_threads; i++) {
    pipe(pipes[i]);
    // pipes[i][0] = leitura
    // pipes[i][1] = escrita
}
```

#### Main → Thread (enviar trabalho)
```c
image_task_t task;
strcpy(task.filename, "imagem.jpeg");
task.terminate = 0;
write(pipes[i][1], &task, sizeof(image_task_t));
```

#### Thread (receber trabalho)
```c
image_task_t task;
read(pipe_fd, &task, sizeof(image_task_t));  // BLOQUEIA até haver dados
```


### 4. CICLO PRINCIPAL DO MAIN

**ANTES (Parte A):**
- Ler argumentos
- Listar imagens
- Dividir trabalho
- Criar threads
- Aguardar threads
- Terminar

**AGORA (Parte B):**
- Ler argumentos
- Criar pipes
- Criar threads (ficam à espera)
- **LOOP INFINITO:**
  - Ler comando (DIR/STAT/QUIT)
  - DIR: listar imagens → enviar para threads via pipes
  - STAT: imprimir estatísticas
  - QUIT: enviar sinal terminação → aguardar threads → sair


### 5. COMPORTAMENTO DAS THREADS

**ANTES (Parte A):**
```c
void *thread_worker(void *arg) {
    // Processar imagens já atribuídas
    for (int i = start_idx; i < end_idx; i++) {
        process_image(images[i]);
    }
    return NULL;  // Termina
}
```

**AGORA (Parte B):**
```c
void *thread_worker(void *arg) {
    while (1) {
        read(pipe_fd, &task, ...);  // ESPERA por trabalho
        
        if (task.terminate) break;  // Sinal de terminação
        
        process_image(...);         // Processa
        atualizar_estatisticas();   // Atualiza stats
    }
    return NULL;
}
```


### 6. ESTATÍSTICAS EM TEMPO REAL

Após processar cada imagem:
```c
pthread_mutex_lock(&stats->mutex);

stats->total_images++;
stats->total_time += time_seconds;
double avg = stats->total_time / stats->total_images;

printf("thread %d processou %s em %.2fs\n", ...);
printf("Numero total de imagens processadas - %d\n", ...);
printf("Tempo médio de processamento - %.2fs\n", avg);

pthread_mutex_unlock(&stats->mutex);
```


### 7. DISTRIBUIÇÃO ROUND-ROBIN

```c
int next_thread = 0;

// Para cada imagem
for (int i = 0; i < num_images; i++) {
    write(pipes[next_thread][1], &task, ...);
    next_thread = (next_thread + 1) % num_threads;
}
```
Distribui: Thread 0 → 1 → 2 → 3 → 0 → 1 → ...


### 8. COMANDOS DISPONÍVEIS

#### DIR <diretoria>
```bash
Qual o comando: DIR ./dataset_A
A 10 imagens na pasta ./dataset_A serão processadas pelas 4 threads
```

#### STAT
```bash
Qual o comando: STAT
Numero total de imagens processadas - 5
Tempo médio de processamento - 12.34s
```

#### QUIT
```bash
Qual o comando: QUIT
[aguarda processar todas as imagens pendentes]
Numero total de imagens processadas - 10
Tempo médio de processamento - 11.23s
[programa termina]
```


## 🔧 COMO COMPILAR E EXECUTAR

### Compilar
```bash
make
# OU
gcc -Wall process-photos-parallel-B.c image-lib.c -o process-photos-parallel-B -lgd -lpthread
```

### Executar
```bash
./process-photos-parallel-B 4 -size
```

### Exemplo de Uso
```bash
$ ./process-photos-parallel-B 4 -size
Foram criadas 4 threads
Qual o comando: STAT
0 imagens - 0.0s tempo médio
Qual o comando: DIR ./dataset_A
A 10 imagens na pasta ./dataset_A serão processadas pelas 4 threads
thread 0 processou img-IST-0.jpeg em 8.59s
Numero total de imagens processadas - 1
Tempo médio de processamento - 8.59s
thread 1 processou img-IST-1.jpeg em 10.87s
Numero total de imagens processadas - 2
Tempo médio de processamento - 9.73s
...
Qual o comando: DIR ./dataset_B
A 5 imagens na pasta ./dataset_B serão processadas pelas 4 threads
...
Qual o comando: STAT
Numero total de imagens processadas - 15
Tempo médio de processamento - 10.12s
Qual o comando: QUIT
Numero total de imagens processadas - 15
Tempo médio de processamento - 10.12s
```


## ⚠️ PONTOS IMPORTANTES

1. **Pipes bloqueiam**: `read()` nas threads bloqueia até o main escrever dados
2. **Mutex protege stats**: Sempre lock/unlock ao aceder `total_images` e `total_time`
3. **Terminação elegante**: QUIT envia `terminate=1` para TODAS as threads
4. **Sem verificação de ficheiros existentes**: Parte B não usa `file_exists()` nas threads
5. **Round-robin**: Imagens distribuídas sequencialmente pelas threads


## 📊 DIFERENÇAS DE DESEMPENHO

**Parte A:**
- Eficiente se processar apenas uma vez
- Threads dividem trabalho antecipadamente

**Parte B:**
- Flexível para múltiplas pastas
- Overhead de comunicação via pipes
- Threads reutilizáveis para vários comandos DIR
