#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <limits>
#include <map>

using namespace std;

const int PORT = 2345;
const int MAX_CONNECTION = 5;

// Definindo map para conats bancárias
map<int,float> accounts;

// Inicializando algumas contas
void initialize_accounts() {
    accounts[1] = 8346.0f;
    accounts[2] = 4738.0f;
    accounts[3] = 70044.0f;
}

// Operações do banco
string process_operation(const string& message) {
    stringstream ss (message);
    int acc_id_origin;
    int acc_id_recipient;
    string operation;
    float value;

    ss >> acc_id_origin >> operation;

    if (accounts.find(acc_id_origin) == accounts.end()) {
        return "FALHA: Conta de origem " + to_string(acc_id_origin) + " nao existe";
    }

    if (operation == "SAQUE") {
        ss >> value;
        if (ss.fail()) {
            return  "FALHA: Formato de saque inválido";
        }

        if (value <= 0) {
            return "FALHA: Valores devem ser acima de 0";
        }

        if (accounts[acc_id_origin] >= value) {
            accounts[acc_id_origin] -= value;
            return "SUCESSO: Saque de R$ " + to_string(value) + "\n" + "Saldo atual R$ " + to_string(accounts[acc_id_origin]);
        }
        else {
            return "FALHA: Saldo insuficiente";
        }
    }

    else if (operation == "DEPOSITO") {
        ss >> value;
        if (ss.fail()) {
            return  "FALHA: Formato de deposito inválido";
        }

        if (value <= 0) {
            return "FALHA: Valores devem ser acima de 0";
        }

        accounts[acc_id_origin] += value;
        return "SUCESSO: Deposito de R$ " + to_string(value) + "\n" + "Saldo autal R$ " + to_string(accounts[acc_id_origin]);
    }

    else if (operation == "TRANSFERENCIA") {
        ss >> acc_id_recipient >> value;

        if (ss.fail()) {
            return  "FALHA : Formato da transferencia inválido";
        }

        if (accounts.find(acc_id_recipient) == accounts.end()) {
            return "FALHA: Conta de destino " + to_string(acc_id_recipient) + " nao existe";
        }

        if (acc_id_origin == acc_id_recipient) {
            return "FALHA: Nao e possivel transferir para sua propria conta, use o deposito";
        }

        if (value <= 0) {
            return "FALHA: Valores devem ser acima de 0";
        }

        if (accounts[acc_id_origin] >= value) {
            accounts[acc_id_origin] -= value;
            accounts[acc_id_recipient] += value;
            return "SUCESSO: Transferencia de R$ " + to_string(value) + " para " + to_string(acc_id_recipient) + "\n" + "Saldo atual R$ " + to_string(accounts[acc_id_origin]);
        }
        else {
            return "FALHA: Saldo insuficiente";
        }
    }

    else if (operation == "CONSULTA") {        
        return "SUCESSO: Seu saldo é " + to_string(accounts[acc_id_origin]);
    }
    
    else {
        return "FALHA: Operacao desconhecida";
    }
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[1024];
    initialize_accounts();

    // Criando socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        cerr << "Erro ao criar socket do servidor" << endl;
        return EXIT_FAILURE;
    }

    // Configurando endereço servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY; // aceita qualquer IP

    // Reutilizar endereço e porta para evitar "Adress already in use"
    int aux = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &aux, sizeof(aux))) {
        cerr << "setsockopt falhou" << endl;
        close(server_sock);
        return EXIT_FAILURE;
    }

    // Ligar o soeck a um endereço e porta
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Erro ao dar bind no socket";
        return EXIT_FAILURE;
    }

    // Socket em modo listen
    if (listen(server_sock, MAX_CONNECTION) < 0) {
        cerr << "Erro no listen do servidor" << endl;
        close(server_sock);
        return EXIT_FAILURE;
    }

    // Loop principal
    while (true) {

        // Aceitar uma conexão de cliente
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            cerr << "Erro ao aceitar conexão" << endl;
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        cout << "Cliente " << client_ip << " : " << ntohs(client_addr.sin_port) << " conectado" << endl;

        // Loop do cliente
        while(true) {
            // Limpar o buffer
            memset(buffer, 0, sizeof(buffer));

            //Receber mensagem do cliente
            int message_read = recv(client_sock, buffer, sizeof(buffer)-1, 0);
            if (message_read < 0) {
                cerr << "Erro ao receber mensagem do cliente" << endl;
                break;
            }
            else  if (message_read == 0) {
                cout << "Cliente Desconectado" << endl;
                break;
            }

            buffer[message_read] = '\0';
            string client_message(buffer);
            cout << "Mensagem " << client_message << " recebida do cliente" << endl;
        
            // Processar operação pedida
            string response = process_operation(client_message);

            // Enviar resposta
            int message_sent = send(client_sock, response.c_str(), response.length(), 0);
            if (message_sent < 0) {
                cerr << "Erro ao enviar resposta ao cliente" << endl;
                break;
            }
        }

        close (client_sock);
        cout << "Conexão com cliente" << client_ip << "desconectado" << endl;
    }

    close(server_sock);
    return EXIT_SUCCESS;
}