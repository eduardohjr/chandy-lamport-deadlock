#include "snapshot.h"
// Converte todos os campos do snapshot (estado local, posse de garfos, IDs de garfos e mensagens em trânsito) em um formato de string que pode ser facilmente transmitido e posteriormente deserializado.
std::string Snapshot::serialize() const {
    std::ostringstream oss;
    oss << "state=" << localState;
    oss << ";hasLeft=" << (hasLeftFork ? "true" : "false");
    oss << ";hasRight=" << (hasRightFork ? "true" : "false");
    oss << ";leftForkId=" << leftForkId;
    oss << ";rightForkId=" << rightForkId;

    for (const auto& [from, msgs] : channelMessages) {
        for (const std::string& msg : msgs) {
            oss << ";channel_from_" << from << "_msg_" << msg;
        }
    }
    return oss.str();
}

// Inverte o processo de serialização, analisando a string de entrada para reconstruir o objeto Snapshot, preenchendo seus campos com os valores extraídos. 
// Lida com a formatação específica das mensagens em trânsito.
Snapshot Snapshot::deserialize(const std::string& data) {
    Snapshot snap;
    std::string token;
    std::istringstream iss(data);

    while (std::getline(iss, token, ';')) {
        size_t eqPos = token.find('=');
        if (eqPos == std::string::npos) { // Pode ser um campo de mensagem sem '=' diretamente
            size_t fromPos = token.find("channel_from_");
            if (fromPos != std::string::npos) {
                std::string rest = token.substr(fromPos + strlen("channel_from_"));
                size_t msgPos = rest.find("_msg_");
                if (msgPos != std::string::npos) {
                    int senderId = std::stoi(rest.substr(0, msgPos));
                    std::string msgContent = rest.substr(msgPos + strlen("_msg_"));
                    snap.channelMessages[senderId].push_back(msgContent);
                }
            }
            continue;
        }

        std::string key = token.substr(0, eqPos);
        std::string value = token.substr(eqPos + 1);

        if (key == "state") {
            snap.localState = value;
        } else if (key == "hasLeft") {
            snap.hasLeftFork = (value == "true");
        } else if (key == "hasRight") {
            snap.hasRightFork = (value == "true");
        } else if (key == "leftForkId") {
            snap.leftForkId = std::stoi(value);
        } else if (key == "rightForkId") {
            snap.rightForkId = std::stoi(value);
        }
    }
    return snap;
}