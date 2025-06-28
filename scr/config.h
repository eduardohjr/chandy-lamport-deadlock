#ifndef CONFIG_H
#define CONFIG_H

// Constantes de configuração global do sistema

// Número total de filósofos no sistema.
const int NUM_PHILOSOPHERS = 3;
// Porta base para os filósofos.
//Cada filósofo 'i' usará a porta BASE_PORT + i.
const int BASE_PORT = 5000;
// Porta do Coordenador.
const int COORDINATOR_PORT = 6000;
// Endereço IP padrão para comunicação local
const char* LOCALHOST = "127.0.0.1";
// Intervalo (em segundos) entre as iniciações de snapshots pelo Coordenador.
const int SNAPSHOT_INTERVAL = 5;

#endif