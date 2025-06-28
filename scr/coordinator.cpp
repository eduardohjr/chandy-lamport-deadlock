#include "coordinator.h"

// Inicializa o Mapa dos garfos para indicar que todos estão disponíveis e a flag de deadlock como falsa e define o id do coordenador como 999
Coordinator::Coordinator(){
    deadlockDetected = false;
    id = 999;
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
        forkAvailable[i] = true;
    std::cout << "[COORDENADOR] Inicializado. NUM_PHILOSOPHERS=" << NUM_PHILOSOPHERS << "\n" << std::flush;
}

// Cria duas threads, uma para o loop principal do coordenador e outra para o recebimento de mensagens
// Usa o join para garantir que o programa espere ambas terminarem para seguir
void Coordinator::start() {
    std::thread t1(&Coordinator::listenLoop, this);
    std::thread t2(&Coordinator::runLoop, this);
    t1.join();
    t2.join();
}

// Loop executado periodicamente, com um tempo definido pela variável SNAPSHOT_INTERVAL.
// A cada interação, ele verefica se um deadlock foi detectado na interação de snapshot anterior
// Limpa os dados do snapshot anterior para uma nova interação
void Coordinator::runLoop() {
    using namespace std::chrono_literals;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(SNAPSHOT_INTERVAL));
        std::unique_lock<std::mutex> lock(mtx);
        if (deadlockDetected) {
            std::cout << "[COORDENADOR] Deadlock detectado no snapshot anterior. Iniciando novo ciclo de detecção para monitoramento contínuo.\n" << std::flush;
            deadlockDetected = false; 
        }

        snapshots.clear();
        std::cout << "[COORDENADOR] Limpando snapshots anteriores antes de iniciar novo ciclo.\n" << std::flush;
        initiateSnapshot();
    }
}

// Coordenador abre um socket, vinucula ele à porta "COORDINATOR_PORT" e aguarda conexões dos filósofos
// Ao receber uma conexão, lê os dados e os deserializa e despacha para a função responsável pelo tratamneto correspondente deles
void Coordinator::listenLoop() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(COORDINATOR_PORT);
    // Aceita conexões de qualquer endereço
    addr.sin_addr.s_addr = INADDR_ANY;

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt failed");
    }

    bind(sockfd, (sockaddr*)&addr, sizeof(addr));
    listen(sockfd, 10);
    std::cout << "[COORDENADOR] Aguardando conexões na porta " << COORDINATOR_PORT << "...\n" << std::flush;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_sock = sizeof(client_addr);
        int client = accept(sockfd, (sockaddr*)&client_addr, &client_sock);
        if (client < 0) {
            perror("accept failed");
            continue;
        }
        char buffer[1024] = {0};
        int value_read = read(client, buffer, 1024);
        if (value_read < 0) {
            perror("read failed");
            close(client);
            continue;
        }
        std::string received_data(buffer, value_read);

        // Deserializa a mensagem recebida
        Message msg = Message::deserialize(received_data);
        std::cout << "[COORDENADOR] Recebeu mensagem: Tipo=" << static_cast<int>(msg.type)
                  << ", Remetente=" << msg.senderId << ", Conteúdo=" << msg.content << "\n" << std::flush;

        // Envia a mensagem para a função handler correspondente
        if (msg.type == MessageType::REQUEST_FORK) {
            // Função que cuida do pedido de garfo
            handleRequest(msg.senderId, std::stoi(msg.content));
        } else if (msg.type == MessageType::RELEASE_FORK) {
            // Função que cuida da liberação de garfo
            handleRelease(msg.senderId, std::stoi(msg.content));
        } else if (msg.type == MessageType::SNAPSHOT_DATA) {
            // Funlçao que cuida da análise do snapshot
            handleSnapshot(msg.senderId, msg.content);
        }
        close(client);
    }
    close(sockfd);
}

// Adiquire um lock para proteger o Mapa "forkAvalible"
// Se o garfo solicitado estiver disponível, ele é marcado como indisponível e envia uma mensagem de sucesso ao filósofo solicitante
// Caso não esteja, é apenas imprimido uma mensagem no terminal
void Coordinator::handleRequest(int fromId, int forkId) {
    
    std::lock_guard<std::mutex> lock(mtx);
    if (forkAvailable[forkId]) {
        forkAvailable[forkId] = false;
        Message msg{ MessageType::FORK_GRANTED, forkId, std::to_string(forkId) };
        sendMessage(BASE_PORT + fromId, msg.serialize());
        std::cout << "[COORDENADOR] Garfo " << forkId << " concedido ao filósofo " << fromId << "\n" << std::flush;
    } else {
        std::cout << "[COORDENADOR] Filósofo " << fromId << " solicitou garfo " << forkId << ", mas não está disponível.\n" << std::flush;
    }
}

// Adiquire um lock para proteger o Mapa "forkAvalible"
// Marca o garfo solto como disponível
void Coordinator::handleRelease(int fromId, int forkId) {
    std::lock_guard<std::mutex> lock(mtx);
    forkAvailable[forkId] = true;
    std::cout << "[COORDENADOR] Garfo " << forkId << " liberado pelo filósofo " << fromId << "\n" << std::flush;
}

// Cria um socket para se conectar à porta e endereço especificados para envio de mensagens
void Coordinator::sendMessage(int port, const std::string& data) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, LOCALHOST, &addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        return;
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }
    send(sock, data.c_str(), data.size(), 0);
    close(sock);
}

// Envia uma mensagem do tipo Marker para os filósofos, indicando o acontecimento do snapshot
// Dessa form os filósofos deverão mandar seues estados para o coordenador
void Coordinator::initiateSnapshot() {
    std::cout << "\n[COORDENADOR] Iniciando snapshot...\n" << std::flush;
    Message marker{ MessageType::MARKER, id, "" };

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        std::cout << "[COORDENADOR] Enviando marcador para filósofo " << i << " na porta " << BASE_PORT + i << "\n" << std::flush;
        sendMessage(BASE_PORT + i, marker.serialize());
    }
}

// Adiquire um lock para proteger o Mapa "napshots".
// Armazena os dados recebidos do snapshot, caso o filósofo não tenha mandado ainda na interação atual
// Se todos os filósofos tiverem enviado o seu snapshot, chama a função para analisá-los e imprimir eles
void Coordinator::handleSnapshot(int fromId, const std::string& content) {
    std::lock_guard<std::mutex> lock(mtx);
    if (snapshots.find(fromId) == snapshots.end()) {
        snapshots[fromId] = content;
        std::cout << "[COORDENADOR] Recebeu snapshot do filósofo " << fromId << ". (Total: " << snapshots.size() << "/" << NUM_PHILOSOPHERS << ")\n" << std::flush;
    } else {
        std::cout << "[COORDENADOR] Recebeu snapshot DUPLICADO do filósofo " << fromId << ". Ignorando.\n" << std::flush;
    }

    if (snapshots.size() == NUM_PHILOSOPHERS) {
        std::cout << "[COORDENADOR] Recebeu todos os snapshots. Imprimindo e detectando deadlock.\n" << std::flush;
        printSnapshot();
    }
}

// Função recursiva para detectar um ciclo em grafo (Algorítimo DFS)
bool hasCycle(int startNode, std::map<int, std::vector<int>>& adj, std::set<int>& visited, std::set<int>& recursionStack) {
    // Marca nó como visitado
    visited.insert(startNode);
    // Adiciona nó na pilha de recursão
    recursionStack.insert(startNode);

    // Itera sobre vizinhos do nó atual
    if (adj.count(startNode)) {
        for (int neighbor : adj[startNode]) {
            // Se o vizinho não foi visitado
            if (!visited.count(neighbor)) {
                //Chama DFS recursivamente para o vizinho
                if (hasCycle(neighbor, adj, visited, recursionStack)) {
                    // Ciclo determinado
                    return true;
                }
            } 
            // Se o vizinho está na pilha de recursão há ciclo
            else if (recursionStack.count(neighbor)) {
                std::cout << "  [DEBUG DFS] Ciclo detectado: " << startNode << " -> ... -> " << neighbor << "\n" << std::flush;
                return true; // Ciclo detectado
            }
        }
    }
    // Remove o nó da pilha ao sair da chamada e retorna como nenhum ciclo detectado
    recursionStack.erase(startNode);
    return false;
}

// Deserializa os dados do snapshot
// Constói um grafo de espera analisando os etados dos filósofos e a posse dos garfos
// Utiliza a função "hasCycle" para verificar o acontecimento de deadlock
void Coordinator::printSnapshot() {
    std::cout << "\n=== SNAPSHOT COMPLETO ===\n" << std::flush;
    // Mapa para armazenar snapshot
    std::map<int, Snapshot> parsedSnapshots;
    // Mapa para indicar filósofos que possuem garfos
    std::map<int, int> forkOwner; 
    // Mapa para indicar filósofos que etão esperando por garfos
    std::map<int, std::vector<int>> waitForGraph;

    // Deserializa e imprimes os dados dos snapshots da cada filósofos
    for (const auto& [id, snap_str] : snapshots) {
        Snapshot s = Snapshot::deserialize(snap_str);
        parsedSnapshots[id] = s;

        std::cout << "Filósofo " << id
                  << ": Estado=" << s.localState
                  << ", Possui Esquerda=" << (s.hasLeftFork ? "Sim" : "Não")
                  << ", Possui Direita=" << (s.hasRightFork ? "Sim" : "Não")
                  << ", Garfo Esquerdo ID=" << s.leftForkId
                  << ", Garfo Direito ID=" << s.rightForkId << "\n" << std::flush;
         // Imprime as mensagens em trânsito para cada filósofo
        for (const auto& [from, msgs_serialized] : s.channelMessages) {
            for (const std::string& msg_str : msgs_serialized) {
                Message m = Message::deserialize(msg_str);
                std::cout << "  Mensagem em trânsito (de " << m.senderId << " para " << id << "): Tipo=";
                if (m.type == MessageType::REQUEST_FORK) std::cout << "REQUEST_FORK";
                else if (m.type == MessageType::RELEASE_FORK) std::cout << "RELEASE_FORK";
                else if (m.type == MessageType::FORK_GRANTED) std::cout << "FORK_GRANTED";
                else if (m.type == MessageType::MARKER) std::cout << "MARKER";
                else if (m.type == MessageType::SNAPSHOT_DATA) std::cout << "SNAPSHOT_DATA";
                else std::cout << "UNKNOWN";
                std::cout << ", Conteúdo=" << m.content << "\n" << std::flush;
            }
        }
    }
    // Atualiazando o Mapa "forkOwner" com base na posse de garfos
    for (const auto& [id, s] : parsedSnapshots) {
        if (s.hasLeftFork) {
            forkOwner[s.leftForkId] = id;
        }
        if (s.hasRightFork) {
            forkOwner[s.rightForkId] = id;
        }
    }
    // Constrói o grafo de espera para a detecção de deadlock
    for (const auto& [id, s] : parsedSnapshots) {
        if (s.localState == "hungry") {
            if (!s.hasLeftFork) {
                bool forkLeftGrantedInTransit = false;
                if (forkOwner.count(s.leftForkId)) { 
                    int potentialOwnerId = forkOwner[s.leftForkId];
                    if (parsedSnapshots.count(potentialOwnerId) && s.channelMessages.count(potentialOwnerId)) {
                        for (const std::string& msg_str : s.channelMessages.at(potentialOwnerId)) {
                            Message m = Message::deserialize(msg_str);
                            if (m.type == MessageType::FORK_GRANTED && std::stoi(m.content) == s.leftForkId) {
                                forkLeftGrantedInTransit = true;
                                break;
                            }
                        }
                    }
                }

                if (!forkLeftGrantedInTransit) {
                    if (forkOwner.count(s.leftForkId)) {
                        int ownerId = forkOwner[s.leftForkId];
                        if (parsedSnapshots.count(ownerId) && parsedSnapshots[ownerId].localState == "hungry") {
                            waitForGraph[id].push_back(ownerId);
                            std::cout << "  [Grafo] Filósofo " << id << " espera pelo garfo " << s.leftForkId << " (possuído por " << ownerId << ")\n" << std::flush;
                        }
                    }
                }
            }

            if (!s.hasRightFork) {
                bool forkRightGrantedInTransit = false;
                if (forkOwner.count(s.rightForkId)) {
                    int potentialOwnerId = forkOwner[s.rightForkId];
                    if (parsedSnapshots.count(potentialOwnerId) && s.channelMessages.count(potentialOwnerId)) {
                        for (const std::string& msg_str : s.channelMessages.at(potentialOwnerId)) {
                            Message m = Message::deserialize(msg_str);
                            if (m.type == MessageType::FORK_GRANTED && std::stoi(m.content) == s.rightForkId) {
                                forkRightGrantedInTransit = true;
                                break;
                            }
                        }
                    }
                }

                if (!forkRightGrantedInTransit) {
                    if (forkOwner.count(s.rightForkId)) {
                        int ownerId = forkOwner[s.rightForkId];
                        if (parsedSnapshots.count(ownerId) && parsedSnapshots[ownerId].localState == "hungry") {
                            waitForGraph[id].push_back(ownerId);
                            std::cout << "  [Grafo] Filósofo " << id << " espera pelo garfo " << s.rightForkId << " (possuído por " << ownerId << ")\n" << std::flush;
                        }
                    }
                }
            }
        }
    }

    // Verifica se há deadlock
    bool deadlockDetectedThisSnapshot = false;
    std::set<int> visitedNodes;
    std::set<int> recursionStack;

    for (const auto& [philosopherId, _] : parsedSnapshots) {
        if (parsedSnapshots[philosopherId].localState == "hungry" && !visitedNodes.count(philosopherId)) {
            std::cout << "  [DEBUG DFS] Iniciando DFS do filósofo " << philosopherId << "\n" << std::flush;
            if (hasCycle(philosopherId, waitForGraph, visitedNodes, recursionStack)) {
                deadlockDetectedThisSnapshot = true;
                break;
            }
        }
    }

    // Atualiza a flag de deadlock do coordenador e imprime o resultado
    if (deadlockDetectedThisSnapshot) {
        std::cout << "\n!!! DEADLOCK DETECTADO !!!\n" << std::flush;
        this->deadlockDetected = true;
    } else {
        std::cout << "\nNenhum deadlock detectado neste snapshot.\n" << std::flush;
        this->deadlockDetected = false;
    }

    std::cout << "===========================\n" << std::flush;
}


// Cria uma instância da classe Coordinator e a inicia.
int main(int argc, char* argv[]) {
    Coordinator coordinator;
    coordinator.start();
    return 0;
}