#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <limits> 

using namespace std;

const int PORT = 2345;
const string SERVER_IP = "127.0.0.1";

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];
    int operation;
    float value;
    int acc_id;
    string recipient_acc;


    // Criando socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Erro ao criar socket"<< endl;
        return EXIT_FAILURE;
    } 

    // Configuração do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Conversão endereço de string para binário (Suporte a IPv6, Melhor tratamento de erros e armazenamento)
    if (inet_pton(AF_INET, SERVER_IP.c_str(), &server_addr.sin_addr) <= 0) {
        cerr << "Endereço inválido" << endl;
        close(sock);
        return EXIT_FAILURE;
    }

    // Tentando conexão com o servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Erro ao conectar ao servidor" << endl;
        close(sock);    
        return EXIT_FAILURE;
    }

    cout << "Conectado ao servidor do banco" << endl;

    // Loop de interação
    while (true) {
        cout << "\n-------------------------------" << endl;
        cout << "Digite o número da sua conta (ou 0 para sair): ";
        cin >> acc_id;

        if (acc_id == 0) {
            break;
        }

        cout << "Digite 1 para saque, 2 para depósito, 3 para transferência ou 4 para consultar seu saldo: ";
        cin >> operation;

        string message_to_server;
        stringstream ss;

        // Adiciona o ID da conta de origem para tods operações
        ss << acc_id << " ";

        // Verificas as operações
        switch (operation)
        {
            case 1: 
                {
                    cout << "Digite uma valor para sacar: ";
                    cin >> value;
                    ss << "SAQUE" << " " << value;
                    break;
                }
            case 2:
                {
                    cout << "Digite o valor para depositar: ";
                    cin >> value;
                    ss << "DEPOSITO" << " " << value;
                    break;
                }
            case 3:
                {
                    cout << "Digite o número daconta para qual quer transferir: ";
                    cin >> recipient_acc;

                    cout << "Digite o valor a ser tranferido: ";
                    cin >> value;
                    ss << "TRANSFERENCIA" << " " << recipient_acc << " "<< value;
                    break;
                }
            case 4:
            {
                ss << "CONSULTA";
                break;
            }
            // Diz que a operaçõe é errada e limpa o estado de erro do cin
            default:
                {
                    cout << "Operação invalida!" << endl;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    continue;
                }
        }

        message_to_server = ss.str();

        // Enviar mensagem para o servidor
            int message_sent = send(sock, message_to_server.c_str(), message_to_server.length(), 0);
            if (message_sent < 0) {
                cerr << "Erro ao enviar a mensagem" << endl;
                break;
            }
        
        // Limpar o buffer
        memset(buffer, 0, sizeof(buffer));

        // Ler mensage
        int message_read = recv(sock, buffer, sizeof(buffer) -1, 0);
        if (message_read < 0) {
            cerr << "Erro ao ler resposta" << endl;
            break;
        }
        else if (message_read == 0) {
            cerr << "Servidor fechou conexão" << endl;
            break;
        }

        buffer[message_read] = '\0';
        cout << buffer << endl;

    }

    close(sock);
    return EXIT_SUCCESS;
 
}