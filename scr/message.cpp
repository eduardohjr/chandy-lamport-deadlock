#include "message.h"

// Converte o tipo da mensagem, o Id do remetente e o conteúdo em uma única string, separados por "|".
// Isso permite transmitir a mensagem pela rede
std::string Message::serialize() const {
    return std::to_string(static_cast<int>(type)) + "|" + std::to_string(senderId) + "|" + content;
}

// Reconstrói um objeto Message com os valores extraídos no formato : "tipo|senderID|conteúdo"
Message Message::deserialize(const std::string& data) {
    std::stringstream ss(data);
    std::string part;
    Message msg;

    std::getline(ss, part, '|');
    msg.type = static_cast<MessageType>(std::stoi(part));

    std::getline(ss, part, '|');
    msg.senderId = std::stoi(part);

    if (std::getline(ss, msg.content)) {
    } else {
        msg.content = "";
    }

    return msg;
}