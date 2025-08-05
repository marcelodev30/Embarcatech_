Exemplos FreeRTOS para BitDogLab

Este diretório contém cinco exemplos práticos de uso do FreeRTOS para a placa BitDogLab, desenvolvidos para facilitar o aprendizado dos principais recursos do sistema operacional de tempo real. Cada exemplo explora um conceito fundamental do FreeRTOS e como aplicá-lo em projetos embarcados.

Leia o livro: https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/tree/main

Requisitos
Placa BitDogLab (Raspberry Pi Pico W)
SDK do Raspberry Pi Pico configurado
FreeRTOS integrado ao ambiente de desenvolvimento (baixe o código no https://freertos.org/ e 
configue a variável de ambiente FREERTOS_KERNEL_PATH)
Compilador compatível (ex.: GCC para ARM)

Exemplos Disponíveis
1. Blink
Descrição: Faz o LED azul da placa piscar usando uma única tarefa do FreeRTOS.
Conceito: Demonstra o uso básico de tarefas em um sistema de tempo real.
Arquivo: blink.c

2. Multitasking
Descrição: Utiliza duas tarefas para alternar o piscar de dois LEDs na placa.
Conceito: Introdução ao multitarefa no FreeRTOS.
Arquivo: multitasking.c

3. Queue
Descrição: Implementa uma fila (queue) para transferir dados entre tarefas:
Uma tarefa lê a temperatura do sensor interno e armazena na fila.
Outra tarefa calcula a média das temperaturas armazenadas e imprime o resultado.
Conceito: Demonstra a comunicação entre tarefas utilizando filas no FreeRTOS.
Arquivo: queue.c

4. Semáforo
Descrição:
Uma Interrupção de Serviço (ISR), gerada pelo pressionamento do botão, sinaliza um evento via semáforo.
Uma tarefa aguarda o evento do semáforo para acionar o LED.
Conceito: Ilustra a sincronização de eventos entre uma ISR e tarefas usando semáforos.
Arquivo: semaforo.c

5. Mutex
Descrição: Duas tarefas compartilham o acesso a um contador utilizando um mutex para evitar condições de disputa.
Conceito: Mostra como proteger recursos compartilhados entre tarefas usando mutexes no FreeRTOS.
Arquivo: mutex.c

Como Compilar e Executar
Clone este repositório e navegue até o diretório do exemplo desejado.
Compile o código usando o SDK configurado para o Raspberry Pi Pico e FreeRTOS.
Faça o upload do binário para a BitDogLab.
Observe o funcionamento conforme descrito em cada exemplo.