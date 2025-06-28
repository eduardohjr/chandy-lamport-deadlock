#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <thread>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <set>
#include <algorithm>


#include "message.h"
#include "config.h"
#include "snapshot.h"

// Definição da classe Coordinator
class Coordinator {
public:
    // Construtor da classe
    Coordinator();
    // Função para inicializar o coordenador (criar as threads)
    void start();

private:
    // Mutex para proteção da região crítica
    std::mutex mtx;
    // Mapa para indicar disponibilidade de um determinado garfo (True = Disponível, False = Ocupado)
    std::map<int, bool> forkAvailable; 
    // Mapa para armazenar o snpashot de cada filósofo
    std::map<int, std::string> snapshots; 
    // Flag para identificar a detecção de um deadlock
    bool deadlockDetected;
    // Id do coordenador
    int id;

    // Loop principal do coordenador
    void runLoop();
    // Loop de escuta do coordenador
    void listenLoop();
    // Função para cuidar dos pedidos de garfos por filósofos
    void handleRequest(int fromId, int forkId);
    // Função para cuidar da liberação de garfos por filósofos
    void handleRelease(int fromId, int forkId);
    // Função para cuidar dos envios de mensagens
    void sendMessage(int port, const std::string& data);
    // Função para iniciar o processo de tirar o snapshot
    void initiateSnapshot();
    // Função que cuidará da análise dos dados recebidos pelo snapshot
    void handleSnapshot(int fromId, const std::string& content);
    // Função para imprimir os resultados coletados e realizar a detecção de deadlock
    void printSnapshot();
    
};

#endif