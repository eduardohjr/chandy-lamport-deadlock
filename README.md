# chandy-lamport-philosophers

Este projeto implementa uma solução distribuída para o clássico problema dos Filósofos Jantando, incorporando um algoritmo de detecção de deadlock baseado no Algoritmo de Snapshot Distribuído de Chandy-Lamport. O sistema é composto por dois tipos de entidades: Filósofos e um Coordenador.

1. Problema dos Filósofos Jantando

O problema dos Filósofos Jantando descreve N filósofos sentados ao redor de uma mesa, alternando entre pensar e comer. Para comer, cada filósofo precisa de dois garfos: um à sua esquerda e um à sua direita. Existem N garfos na mesa, um entre cada par de filósofos. O desafio é projetar um sistema que permita aos filósofos comer sem entrar em um deadlock (situação onde todos os filósofos esperam indefinidamente por um recurso que nunca será liberado) ou inanition (alguns filósofos nunca conseguem comer).

Nesta implementação, o gerenciamento dos garfos é centralizado no Coordenador. Os filósofos não interagem diretamente para solicitar garfos uns aos outros; eles sempre se comunicam com o Coordenador.

2. Componentes do Sistema

O sistema é dividido em duas aplicações principais:

Filósofo (philosopher.cpp, philosopher.h):

Cada filósofo é uma entidade separada, executada em seu próprio processo ou thread.

Possui um ID único, e sabe quais são seus garfos esquerdo e direito.

Alterna entre os estados: "pensando", "faminto" e "comendo".

Comunicação: Comunica-se com o Coordenador para solicitar (REQUEST_FORK) e liberar (RELEASE_FORK) garfos. Recebe mensagens de garfo concedido (FORK_GRANTED) do Coordenador.

Participante do Snapshot: Quando recebe um MARKER (marcador) pela primeira vez, ele salva seu estado local e começa a registrar as mensagens que chegam em seus canais de entrada (mensagens em trânsito). Em seguida, propaga o marcador para outros filósofos. Quando recebe marcadores de todos os outros filósofos (exceto de si mesmo), ele finaliza a gravação e envia seu snapshot completo para o Coordenador.

Coordenador (coordinator.cpp, coordinator.h):

É a entidade central do sistema.

Gerenciamento de Garfos: Mantém um registro global do estado de todos os garfos (forkAvailable). Quando um filósofo solicita um garfo, o Coordenador verifica sua disponibilidade. Se disponível, ele é concedido e o estado do garfo é atualizado.

Comunicação: Escuta mensagens dos filósofos e responde a elas.

Iniciação de Snapshot: Periodicamente (definido por SNAPSHOT_INTERVAL), o Coordenador inicia um novo ciclo de detecção de deadlock enviando MARKERs para todos os filósofos.

Coleta e Análise de Snapshot: Recebe os dados de SNAPSHOT_DATA de todos os filósofos. Uma vez que todos os snapshots foram coletados, ele os compila para formar um instantâneo global consistente do sistema.

Detecção de Deadlock: Analisa o snapshot global. Para isso, ele constrói um grafo de espera, onde uma aresta de Filósofo A para Filósofo B significa que A está esperando por um recurso (garfo) que B possui. Se um ciclo é detectado neste grafo (usando DFS), um deadlock é identificado. As mensagens FORK_GRANTED em trânsito são cruciais para evitar falsos positivos de deadlock, pois um garfo pode já ter sido concedido e estar a caminho, mesmo que o filósofo receptor ainda não o tenha marcado como possuído.

3. Comunicação (message.h, message.cpp)

A comunicação entre os componentes é baseada em sockets TCP/IP. As mensagens são serializadas e deserializadas para serem transmitidas pela rede.

Message struct: Define a estrutura de uma mensagem, incluindo:

MessageType: Um enum class que define os tipos de mensagens possíveis:

REQUEST_FORK: Filósofo solicita um garfo ao Coordenador.

RELEASE_FORK: Filósofo libera um garfo para o Coordenador.

FORK_GRANTED: Coordenador concede um garfo a um filósofo.

MARKER: Usado no algoritmo de snapshot para iniciar a gravação de estado.

SNAPSHOT_DATA: Filósofo envia seu snapshot local para o Coordenador.

senderId: O ID do componente que enviou a mensagem.

content: Conteúdo textual da mensagem (ex: ID do garfo, dados serializados do snapshot).

serialize() e deserialize(): Métodos para converter a estrutura Message para/de uma string, permitindo sua transmissão por sockets.

4. Algoritmo de Snapshot Distribuído (Chandy-Lamport)

O Chandy-Lamport é um algoritmo para registrar um estado global consistente em um sistema distribuído assíncrono. No contexto deste projeto, ele é usado para detectar deadlocks.

Como funciona:

Iniciação (Coordinator::initiateSnapshot()): O Coordenador inicia o algoritmo enviando uma mensagem MARKER para todos os filósofos.

Filósofo Recebe o Primeiro Marcador (Philosopher::handleMarker()):

P1: Salva o estado local: O filósofo imediatamente salva seu estado atual (pensando, faminto, comendo, e posse de garfos).

P2: Inicia gravação e propaga marcador: Ele começa a gravar todas as mensagens subsequentes que chegam em seus canais de entrada (de outros filósofos) até receber um MARKER de cada um deles. Ele também propaga um MARKER para todos os seus canais de saída (ou seja, para todos os outros filósofos).

Filósofo Recebe Marcadores Subsequentes (Philosopher::handleMarker()): Se um filósofo já está no modo de gravação e recebe um MARKER, ele simplesmente registra que o marcador veio daquele canal. Ele não reinicia a gravação nem propaga o marcador novamente por esse canal (pois já o fez quando recebeu o primeiro marcador).

Coleta Completa (Philosopher::handleMarker()): Quando um filósofo recebe um MARKER de todos os outros filósofos (ou seja, de todos os seus canais de entrada), ele para de gravar mensagens e envia seu SNAPSHOT_DATA (estado local salvo + mensagens em trânsito coletadas) para o Coordenador.

Coordenador Coleta Todos os Snapshots (Coordinator::handleSnapshot()): O Coordenador aguarda e coleta os snapshots de todos os filósofos.

Análise de Deadlock (Coordinator::printSnapshot()): Uma vez que o Coordenador tem todos os snapshots:

Ele constrói um grafo de espera. Um filósofo A espera por B se A está hungry e precisa de um garfo que B possui, E não há uma mensagem FORK_GRANTED em trânsito de B para A para esse garfo. A consideração de mensagens em trânsito é crucial para a consistência do snapshot, pois evita que o Coordenador detecte um deadlock onde, na realidade, o recurso já está a caminho.

Ele então usa uma busca em profundidade (DFS) no grafo de espera para detectar ciclos. A presença de um ciclo indica um deadlock.

5. Estrutura Snapshot (snapshot.h, snapshot.cpp)

A estrutura Snapshot armazena as informações coletadas por cada filósofo:

localState: O estado do filósofo ("thinking", "hungry", "eating").

hasLeftFork, hasRightFork: Booleans indicando se o filósofo possui os garfos.

leftForkId, rightForkId: IDs dos garfos associados ao filósofo.

channelMessages: Um mapa que armazena mensagens que estavam em trânsito. A chave é o ID do remetente da mensagem, e o valor é uma lista de strings serializadas das mensagens.

Os métodos serialize() e deserialize() permitem converter um objeto Snapshot para/de uma string para transmissão.

6. Configurações (config.h)

O arquivo config.h define constantes globais importantes para o sistema:

NUM_PHILOSOPHERS: O número de filósofos.

BASE_PORT: A porta inicial para os filósofos (cada filósofo i usará BASE_PORT + i).

COORDINATOR_PORT: A porta em que o Coordenador escuta.

LOCALHOST: O endereço IP para comunicação local.

SNAPSHOT_INTERVAL: O tempo em segundos entre as iniciações de snapshots pelo Coordenador.

7. Detecção de Deadlock (hasCycle e Coordinator::printSnapshot)

A função hasCycle é uma implementação padrão de DFS para detectar ciclos em um grafo direcionado. É chamada pelo Coordenador para analisar o grafo de espera.

A função Coordinator::printSnapshot é o coração da detecção de deadlock:

Processa Snapshots: Deserializa todos os snapshots recebidos dos filósofos.

Identifica Donos de Garfos: Cria um mapa forkOwner para saber qual filósofo possui cada garfo no momento do snapshot.

Constrói Grafo de Espera: Para cada filósofo hungry:

Se ele não possui um garfo, verifica quem o possui.

Crucialmente, verifica se há uma mensagem FORK_GRANTED em trânsito para aquele garfo. Se houver, o filósofo não está esperando por ele, pois o garfo já foi concedido e está "a caminho" (mesmo que ainda não tenha chegado ao destino).

Se não há mensagem FORK_GRANTED em trânsito e o garfo é possuído por outro filósofo também faminto, uma aresta é adicionada ao grafo de espera (do filósofo que espera para o filósofo que possui o garfo).

Detecta Ciclos: Aplica a função hasCycle (DFS) em todos os filósofos que estão hungry e ainda não foram visitados na DFS, buscando por um ciclo. Se um ciclo é encontrado, um deadlock é declarado.
