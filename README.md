# Process Photos Parallel
Processamento paralelo de imagens JPEG usando multithreading em C com a biblioteca GD.
Descrição
Dois programas que aplicam 5 transformações a imagens (contrast, blur, sepia, thumbnail, grayscale):

Parte A: Processa uma pasta de imagens usando threads trabalhadoras
Parte B: Interface interativa com comunicação via pipes entre threads

# Stack

Linguagem: C (POSIX threads)
Biblioteca: GD (manipulação de imagens)
Concorrência: pthreads, pipes, mutex

# Funcionamento do codigo

## Compilação
make
Ou manualmente:
gcc -Wall process-photos-parallel-A.c image-lib.c -o process-photos-parallel-A -lgd -lpthread
gcc -Wall process-photos-parallel-B.c image-lib.c -o process-photos-parallel-B -lgd -lpthread

## Execução
### Parte A
./process-photos-parallel-A <diretoria> <num_threads> <-name|-size>

Exemplo:
bash./process-photos-parallel-A ./images 4 -size

### Parte B
bash./process-photos-parallel-B <num_threads> <-name|-size>
Comandos disponíveis:

DIR <diretoria> - Processa imagens da pasta
STAT - Mostra estatísticas
QUIT - Termina o programa

Exemplo:
bash./process-photos-parallel-B 4 -size
Qual o comando: DIR ./images
Qual o comando: STAT
Qual o comando: QUIT

## Funcionalidades
### Parte A:

Divisão estática de trabalho entre threads
Ordenação por nome ou tamanho
Medição de tempos de execução
Guarda estatísticas em timing_<threads><mode>.txt

### Parte B:

Comunicação via pipes
Distribuição round-robin
Estatísticas em tempo real
Processamento de múltiplas pastas

# Estrutura
.
├── process-photos-parallel-A.c  # Parte A (divisão estática)
├── process-photos-parallel-B.c  # Parte B (pipes + interativo)
├── image-lib.c                  # Transformações de imagens
├── image-lib.h                  # Headers
├── Makefile
└── README.md

# Transformações Aplicadas
Cada imagem gera 5 versões:

contrast_*.jpeg - Contraste aumentado
blur_*.jpeg - Efeito blur
sepia_*.jpeg - Tom sépia
thumb_*.jpeg - Miniatura (1/5 do tamanho)
gray_*.jpeg - Escala de cinza

Saída: pasta Result-image-dir/

# Performance
O número de threads influencia diretamente o tempo de processamento. Ficheiro timing_*.txt contém:

Tempo total
Tempo de cada thread
Tempo não paralelo
