#include "philosopher.h"

// Inicializa o ID do filósofo, e determina os IDs dos garfos esquerdo e direito com base no seu próprio ID e no número total de filósofos.
// Imprime uma mensagem de inicialização
Philosopher::Philosopher(int id) : id(id) {
    leftFork = id;
    rightFork = (id + 1) % NUM_PHILOSOPHERS;
    std::cout << "[Filósofo " << id << "] Inicializado. Garfo esquerdo: " << leftFork << ", Garfo direito: " << rightFork << "\n" << std::flush;
}

// Cria uma thread para o recebimento de mensagens e chama "runLoop" na thread 
// Usa o join para garantir que o programa espere ela terminar para seguir
void Philosopher::start() {
    std::thread listener(&Philosopher::listenLoop, this);
    runLoop();
    listener.join();
}

// Simula o ciclo de vida de um filósofo: pensando, faminto e comendo.
// Gerencia as transições de estado, solicita e libera garfos, e espera pelas condições necessárias (posse de ambos os garfos) para comer.
void Philosopher::runLoop() {
    using namespace std::chrono_literals;

    while (true) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            state = "thinking";
        }

        std::cout << "[Filósofo " << id << "] Pensando...\n" << std::flush;
        std::this_thread::sleep_for(2s); // Tempo de pensamento

        {
            std::unique_lock<std::mutex> lock(mtx);
            state = "hungry";
        }

        requestFork(leftFork);
        requestFork(rightFork);

        {
            std::unique_lock<std::mutex> lock(mtx);
            while (!(hasLeft && hasRight)) {
                std::cout << "[Filósofo " << id << "] Aguardando garfos (L:" << hasLeft << ", R:" << hasRight << ")...\n" << std::flush;
                cv.wait(lock);
            }
            state = "eating";
        }

        std::cout << "[Filósofo " << id << "] Comendo...\n" << std::flush;
        std::this_thread::sleep_for(2s); // Tempo de alimentação

        releaseFork(leftFork);
        releaseFork(rightFork);
    }
}

// O filósofo abre um socket, vincula-o à sua porta específica ("BASE_PORT + id") e fica aguardando conexões e mensagens.
// Ao receber uma mensagem, ele a deserializa e a despacha para a função "handleMessage".
void Philosopher::listenLoop() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BASE_PORT + id);
    addr.sin_addr.s_addr = INADDR_ANY;

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed on philosopher");
    }

    bind(sockfd, (sockaddr*)&addr, sizeof(addr));
    listen(sockfd, 20);
    std::cout << "[Filósofo " << id << "] Aguardando mensagens na porta " << BASE_PORT + id << "...\n" << std::flush;

    while (true) {
        sockaddr_in cli{};
        socklen_t clilen = sizeof(cli);
        int client = accept(sockfd, (sockaddr*)&cli, &clilen);
        if (client < 0) {
            perror("accept failed on philosopher");
            continue;
        }
        char buffer[1024] = {0};
        int valread = read(client, buffer, 1024);
        if (valread < 0) {
            perror("read failed on philosopher");
            close(client);
            continue;
        }
        std::string received_data(buffer, valread);

        handleMessage(received_data);
        close(client);
    }
    close(sockfd);
}

// Envia mensagem do tipo "REQUEST_FORK" para o coordenador, solicitando um garfo ao coordenador
void Philosopher::requestFork(int forkId) {
    Message msg{ MessageType::REQUEST_FORK, id, std::to_string(forkId) };
    sendMessage(COORDINATOR_PORT, msg.serialize());
    std::cout << "[Filósofo " << id << "] Solicitou garfo " << forkId << "\n" << std::flush;
}

// Envia mensagem do tipo "RELEASE_FORK" para o coordenador, liberando um garfo ao coordenador
void Philosopher::releaseFork(int forkId) {
    Message msg{ MessageType::RELEASE_FORK, id, std::to_string(forkId) };
    sendMessage(COORDINATOR_PORT, msg.serialize());
    std::cout << "[Filósofo " << id << "] Liberou garfo " << forkId << "\n" << std::flush;
    if (forkId == leftFork) hasLeft = false;
    else if (forkId == rightFork) hasRight = false;
}

// Cria um socket para se conectar à porta e endereço especificados para envio de mensagens
void Philosopher::sendMessage(int port, const std::string& data) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed on philosopher");
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, LOCALHOST, &addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported on philosopher");
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

// Deserializa a mensagem e a processa de acordo com o seu tipo.
// Se estiver no modo de gravação de snapshot ("recording"), as mensagens que não são marcadores são armazenadas como mensagens em trânsito.
void Philosopher::handleMessage(const std::string& data) {
    Message msg = Message::deserialize(data);

    if (collectingSnapshot) {
        // Se estiver coletando snapshot, armazena mensagens em trânsito
        if (msg.type != MessageType::MARKER) {
            messagesInTransit[msg.senderId].push_back(data);
        }
    }


    if (msg.type == MessageType::FORK_GRANTED) {
        // Adquire um lock
        std::unique_lock<std::mutex> lock(mtx); 
         // Obtém o ID do garfo concedido
        int fork = std::stoi(msg.content);
        if (fork == leftFork) {
            // Filósofo agora possui o garfo esquerdo
            hasLeft = true; 
            std::cout << "[Filósofo " << id << "] Recebeu garfo esquerdo " << fork << "\n" << std::flush;
        }
        else if (fork == rightFork) {
            // Filósofo agora possui o garfo direito
            hasRight = true;
            std::cout << "[Filósofo " << id << "] Recebeu garfo direito " << fork << "\n" << std::flush;
        }
         // Notifica outras threads
        cv.notify_all();
    } else if (msg.type == MessageType::MARKER) {
         // Chama a função para lidar com a mensagem de marcador
        handleMarker(msg.senderId);
    }
}

// Esta função implementa a lógica do algoritmo de snapshot distribuído.
// Quando o *primeiro* marcador é recebido:
//  -> 1: O filósofo salva seu estado local e começa a gravar as mensagens em trânsito em todos os seus canais de entrada.
//  -> 2: O filósofo envia um marcador para todos os seus *canais de saída*.
// Para marcadores subsequentes (enquanto já está gravando) apenas registra que o marcador foi recebido de um novo canal.
// Quando marcadores são recebidos de *todos* os outros filósofos, o filósofo para de gravar mensagens e envia seu snapshot completo (estado local + mensagens em trânsito) para o coordenador.
void Philosopher::handleMarker(int fromId) {
    std::unique_lock<std::mutex> lock(mtx);

    if (!recording) {
        // P1: Salva o estado local
        recording = true;
        snapshot.localState = state;
        snapshot.hasLeftFork = hasLeft;
        snapshot.hasRightFork = hasRight;
        snapshot.leftForkId = leftFork;
        snapshot.rightForkId = rightFork;

        markersReceivedFrom.insert(fromId);

        // P2: Envia o marcador para todos os outros canais de saída
        sendMarkerToOthers();
    } else {
        markersReceivedFrom.insert(fromId);
    }

    if (markersReceivedFrom.size() == NUM_PHILOSOPHERS - 1) {
        sendSnapshotToCoordinator();
        // Reseta o estado para o próximo snapshot
        recording = false;
        markersReceivedFrom.clear();
        messagesInTransit.clear();
    }
}

// Cria uma mensagem do tipo "MARKER" e a envia para cada filósofo, exceto para si mesmo.
void Philosopher::sendMarkerToOthers() {
    Message marker{ MessageType::MARKER, id, "" };
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (i != id) {
            sendMessage(BASE_PORT + i, marker.serialize());
        }
    }
}

// Atribui as mensagens em trânsito coletadas ao objeto `snapshot`, cria uma mensagem do tipo `SNAPSHOT_DATA` contendo o snapshot serializado, e a envia para o coordenador.
void Philosopher::sendSnapshotToCoordinator() {
    snapshot.channelMessages = messagesInTransit;
    Message snap{ MessageType::SNAPSHOT_DATA, id, snapshot.serialize() };
    sendMessage(COORDINATOR_PORT, snap.serialize());
}

// Espera um ID de filósofo como argumento de linha de comando, cria uma instância da classe Philosopher com este ID e a inic
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <philosopher_id>\n";
        return 1;
    }

    int id = std::stoi(argv[1]);
    Philosopher p(id);
    p.start();

    return 0;
}