#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iostream> // Para debugging
#include <cstring> // Para strlen

#include "message.h" // Incluir para deserializar mensagens em transito no deserialize do snapshot (apenas para debug se precisar)


// Estrutura usada para coletar o estado global do sistema em um determinado instante de tempo
// Cada filósofo registra seu estado local e as mensagens que estavam em trânsito em seus canais de entrada no momento que recebream a mensagem "MARKERr" do coordenador
struct Snapshot {
    std::string localState;
    std::map<int, std::vector<std::string>> channelMessages;
    // Adicionado para a detecção de deadlock
    bool hasLeftFork = false;
    bool hasRightFork = false;
    int leftForkId; // Adicionar o ID do garfo esquerdo
    int rightForkId; // Adicionar o ID do garfo direito

    // Serializa a estrutura de snapshot para a transmissão pela rede
    std::string serialize() const;
    // Deserializa a string em na estrutura de Snapshot
    static Snapshot deserialize(const std::string& data);
};

#endif