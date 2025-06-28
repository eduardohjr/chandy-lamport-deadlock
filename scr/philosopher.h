#ifndef PHILOSOPHER_H
#define PHILOSOPHER_H

#include <string>
#include <mutex>
#include <condition_variable>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

#include "message.h"
#include "snapshot.h"
#include "config.h"
#include "snapshot.h"


// Cada filósofo opera de forma autônoma, interagindo com um coordenador para solicitar e liberar garfos. 
// Ele também participa do algoritmo de snapshot distribuído de Chandy-Lamport, registrando seu estado local e as mensagens em trânsito ao receber um marcador.
class Philosopher {
public:
    // Construtor da classe Philosopher.
    Philosopher(int id);
    // Inicia o filósofo.
    // Cria e gerencia as threads para o loop de execução (comportamento de jantar) e o loop de escuta (receber mensagens).
    void start();

private:
    int id;
    int leftFork;
    int rightFork;
    std::string state = "thinking";

    bool hasLeft = false;
    bool hasRight = false;

    // Flag se o filósofo está gravando mensagens em trânsito
    bool recording = false;
    // IDs dos filósofos (ou coordenador) de quem marcadores já foram recebidos
    std::set<int> markersReceivedFrom;
    // Mensagens em trâncsito
    std::map<int, std::vector<std::string>> messagesInTransit;
    // Objeto Snapshot para armazenar o estado local e mensagens em trânsito
    Snapshot snapshot;

    std::mutex mtx;
    // Variável de condição para sincronizar a espera pelos garfos
    std::condition_variable cv;
     //Flag para indicar que um snapshot está sendo coletado
    bool collectingSnapshot = false;

    // Loop principal do comportamento do filósofo
    void runLoop();
    // Loop de escuta por mensagens de rede
    void listenLoop();
    // Solicitaçao de um garfo ao coordenador
    void requestFork(int forkId);
    // Liberação de um garfo para o coordenador
    void releaseFork(int forkId);
    // Envio de mensagens
    void sendMessage(int port, const std::string& data);
    // Despacha mensagens recebidas com base no tipo
    void handleMessage(const std::string& data);
    // Lida com a recepção de uma mensagem de marcador
    void handleMarker(int fromId);
    // Envia marcadores para os outros filósofos
    void sendMarkerToOthers();
    // Envia o snapshot coletado para o coordenador
    void sendSnapshotToCoordinator();
};

#endif