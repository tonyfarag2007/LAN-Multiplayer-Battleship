#include <SFML/Network.hpp>
#include <iostream>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Graphics.hpp>
#include "Network.h"
sf::RenderWindow window(sf::VideoMode({1920, 1080}),
    "Battleship", sf::Style::Titlebar | sf::Style::Close);
sf::Font font("AsapSharp.ttf");
sf::Color customWhite(255, 255, 255, 180);
class Ship {
public:
    int length;
    int row, col;
    int cellsHit;
    bool isVertical, isDragged = false, isPlaced = false;
    bool sunkReported = false;
    sf::Vector2f dragOffset;
};
class Player {
public:
    Ship ships[5] = {
        {5, 0, 0, 0, true},
        {4, 0, 0, 0, true},
        {3, 0, 0, 0, true},
        {2, 0, 0, 0, true},
        {1, 0, 0, 0, true}
    };
    sf::RectangleShape board[10][10], trackingBoard[10][10], shipShapes[5];
    std::vector<std::vector<std::pair<int, int>>> shipLocations;
    std::string myName, opponentName;
    int hitCount = 0;
    bool myTurn = false;
    bool isReady = false;
    bool playAgain = false;
    bool opponentPlayAgain = false;
};
class UI {
public:
    sf::RectangleShape instructionBorder{{450.f, 450.f}};
    sf::Text instructionText{font, "INSTRUCTIONS"};
    std::vector<std::string> instructions;
    sf::RectangleShape shipLoader{{200.f, 560.f}};
    sf::Text readyText{font, "Ready!"};
    sf::Text playerIsReadyText{font};
    sf::RectangleShape readyButton{{180.f, 90.f}};
    sf::Text placeShipsText{font, "Place ships"};
    sf::Text playerLabel{font, "Player"};
    sf::Text battleText{font, "Battle!"};
    sf::Text attackFeedback{font};
    sf::Text turnFeedback{font, ""};
    sf::Text shipSunk{font};
    sf::Text rematchText{font, "Rematch?"};
    sf::Text rematchAttemptText{font};
    sf::Text disconnectedText{font, "Opponent disconnected!"};
    sf::Text gameOver{font};
    sf::RectangleShape verGridLine{{1.f, 600.f}};
    sf::RectangleShape horGridLine{{600.f, 1.f}};
    sf::RectangleShape playAgainButton{{200.f, 100.f}};
    UI() {
        instructionBorder.setPosition({50.f, 250.f});
        instructionBorder.setFillColor(sf::Color::Red);
        instructionText.setPosition({170.f, 280.f});
        instructions = {
            "- Drag ships onto your board",
            "- Right-click a ship to rotate it",
            "- Ships cannot overlap",
            "- Ships cannot be hanging",
            "- Click ready when all ships",
            " are placed"
        };
        shipLoader.setPosition({1600.f, 160.f});
        readyText.setPosition({1650.f, 775.f});
        playerIsReadyText.setPosition({150.f, 200.f});
        readyButton.setPosition({1610.f, 750.f});
        readyButton.setFillColor(customWhite);
        placeShipsText.setPosition({820.f, 800.f});
        playerLabel.setPosition({850.f, 30.f});
        battleText.setPosition({910.f, 30.f});
        attackFeedback.setPosition({900.f, 400.f});
        turnFeedback.setPosition({900.f, 350.f});
        shipSunk.setPosition({900.f, 300.f});
        playAgainButton.setPosition({800.f, 840.f});
        rematchText.setPosition({830.f, 870.f});
        disconnectedText.setPosition({730.f, 100.f});
        gameOver.setPosition({730.f, 100.f});
        verGridLine.setFillColor(sf::Color::Black);
        horGridLine.setFillColor(sf::Color::Black);
        playAgainButton.setFillColor(sf::Color::Red);
    }

};

Player me; UI gameUI;

enum Screen {
    CONNECTING,
    WAITING_FOR_CLIENT,
    PLACING_SHIPS,
    BATTLESHIP,
    GAME_OVER,
    DISCONNECTED
};
enum MessageType {
    ATTACK,
    HIT_RESULT,
    SUNK_SHIP,
    GAME_END
};
std::vector<std::vector<std::pair<int, int>>> storeShipLocations(Ship ships[], int size) {
    std::vector<std::vector<std::pair<int, int>>> shipLocations(size);
    for (int i = 0; i < size; i++) {
        shipLocations[i].push_back({ships[i].row, ships[i].col});
        }
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < ships[i].length - 1; j++) {
            if (ships[i].isVertical) {
                shipLocations[i].push_back({ships[i].row + (j+1), ships[i].col});
            }
            else if (!ships[i].isVertical) {
                shipLocations[i].push_back({ships[i].row, ships[i].col + (j+1)});
            }
        }
    }
    return shipLocations;
    }
void drawBoard(float boardOffset, sf::RectangleShape board[][10], int rows, UI *ui) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 10; j++) {
            board[i][j].setPosition({boardOffset + j * 60.f, 150.f + i * 60.f });
            window.draw(board[i][j]);
        }
    }
    for (int i = 0; i <= 10; i++) {
        ui->verGridLine.setPosition({boardOffset + i * 60.f, 150.f});
        ui->horGridLine.setPosition({boardOffset, 150.f + i * 60.f});
        window.draw(ui->horGridLine);
        window.draw(ui->verGridLine);
    }
}
void initializeShipLocations(Player& player) {
    for (int i = 0; i < 5; i++) {
        player.shipShapes[i].setSize({60.f, player.ships[i].length * 60.f});
    }
    player.shipShapes[0].setPosition({1620.f, 180.f});
    player.shipShapes[1].setPosition({1720.f, 180.f});
    player.shipShapes[2].setPosition({1720.f, 440.f});
    player.shipShapes[3].setPosition({1620.f, 500.f});
    player.shipShapes[4].setPosition({1620.f, 640.f});
}
bool isOverlapping(std::vector<std::vector<std::pair<int, int>>> playerShipLocations,
    Ship ships[], int size) {
    for (int g = 0; g < 5; g++) {
        int shipIndex = g;
        for (int h = 0; h < 5; h++){
            if (h == shipIndex) continue;
            for (int i = 0; i < ships[g].length; i++) {
                for (int j = 0; j < ships[h].length; j++) {
                    if (playerShipLocations[g][i] == playerShipLocations[h][j]) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
bool isHanging(std::vector<std::vector<std::pair<int, int>>> playerShipLocations,
    Ship ships[], int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < ships[i].length; j++) {
            if (playerShipLocations[i][j].first > 9 || playerShipLocations[i][j].first < 0
                || playerShipLocations[i][j].second > 9 || playerShipLocations[i][j].second < 0) {
                return true;
            }
        }
    }
    return false;
}
bool placeShips(Player& player) {
    if (player.isReady) return false;
    auto mousePosition = sf::Vector2f(sf::Mouse::getPosition(window));
    static bool wasLeftDown = false, wasRightDown = false;
    bool leftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
    rightDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
            bool leftPressedNow = leftDown && !wasLeftDown, rightPressedNow = rightDown && !wasRightDown;
            bool releasedNow = !leftDown && wasLeftDown;
            if (rightPressedNow) {
                for (int i = 0; i < 5; i++) {
                    if (player.shipShapes[i].getGlobalBounds().contains(mousePosition)) {
                        player.ships[i].isVertical = !player.ships[i].isVertical;
                        if (!player.ships[i].isVertical) {
                            player.shipShapes[i].setSize({player.ships[i].length * 60.f, 60.f});
                        }
                        else {
                            player.shipShapes[i].setSize({60.f, player.ships[i].length * 60.f});
                        }
                        break;
                    }
                }
            }
            if (leftPressedNow) {
                for (int i = 0; i < 5; i++) {
                    if (player.shipShapes[i].getGlobalBounds().contains(mousePosition)) {
                        player.ships[i].isDragged = true;
                        player.ships[i].dragOffset = mousePosition - player.shipShapes[i].getPosition();
                        player.ships[i].isPlaced = false;
                        break;
                    }
                }
            }
            for (int i = 0; i < 5; i++) {
                if (player.ships[i].isDragged) {
                    player.shipShapes[i].setPosition(mousePosition - player.ships[i].dragOffset);
                }
            }
            if (releasedNow) {
                for (int i = 0; i < 5; i++) {
                    if (player.ships[i].isDragged) {
                        sf::Vector2f pos = player.shipShapes[i].getPosition();
                        bool insideBoard = (pos.x >= 600.f && pos.x <= 1200.f && pos.y >= 150.f && pos.y <= 750.f);
                        if (insideBoard) {
                            player.ships[i].col = static_cast<int>((pos.x - 600.f) / 60.f);
                            player.ships[i].row = static_cast<int>((pos.y - 150.f) / 60.f);
                            player.shipShapes[i].setPosition({600.f + player.ships[i].col * 60.f,
                                150.f + player.ships[i].row * 60.f});
                            player.ships[i].isPlaced = true;
                        }
                    }
                    player.ships[i].isDragged = false;
                }
            }
    wasLeftDown = leftDown;
    wasRightDown = rightDown;
            for (int i = 0; i < 5; i++) {
                if (!player.ships[i].isPlaced) {
                    return false;
                }
            }
    return true;
}
std::pair<int, int> attack(Player& player) {
    int rowHit = -1, colHit = -1;
    auto mousePosition = sf::Vector2f(sf::Mouse::getPosition(window));
    static bool wasLeftDown = false;
    bool leftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool leftPressedNow = leftDown && !wasLeftDown;
    bool hit = false;
                for (int i = 0; i<10; i++) {
                    for (int j = 0; j<10; j++) {
                            if (player.trackingBoard[i][j].getGlobalBounds().contains(mousePosition) &&
                                player.trackingBoard[i][j].getFillColor() != sf::Color::Red &&
                                player.trackingBoard[i][j].getFillColor() != customWhite &&
                                player.trackingBoard[i][j].getFillColor() != sf::Color::Cyan) {
                                player.trackingBoard[i][j].setFillColor(sf::Color::Yellow);
                                if(leftPressedNow) {
                                    sf::Vector2f hitLocation(mousePosition);
                                    colHit = static_cast<int>((hitLocation.x - 1120.f)/60.f);
                                    rowHit = static_cast<int>((hitLocation.y - 150.f)/60.f);
                                }
                            }
                            else if (!player.trackingBoard[i][j].getGlobalBounds().contains(mousePosition)
                                && player.trackingBoard[i][j].getFillColor() == sf::Color::Yellow) {
                                player.trackingBoard[i][j].setFillColor(sf::Color::Blue);
                                }
                    }
                }
    wasLeftDown = leftDown;
    return {rowHit, colHit};
}
bool handleOpponentAttack(Player& player, int rowHit, int colHit) {
    bool hitFound = false;
   for (int i = 0; i < 5; i++) {
       for (int j = 0; j<player.ships[i].length; j++) {
           if (player.shipLocations[i][j].first == rowHit && player.shipLocations[i][j].second == colHit) {
               player.board[rowHit][colHit].setFillColor(sf::Color::Red);
               hitFound = true;
               player.ships[i].cellsHit++;
           }
       }
   }
    if (!hitFound) {
        player.board[rowHit][colHit].setFillColor(customWhite);
    }
    return hitFound;
}
std::optional<std::vector<std::pair<int, int>>> handleSunkShip(Player& player) {
   for (int i = 0; i < 5; i++) {
           if (player.ships[i].cellsHit == player.ships[i].length) {
               if (!player.ships[i].sunkReported) {
                   player.ships[i].sunkReported = true;
                   return player.shipLocations[i];
               }
           }
   }
    return std::nullopt;
}
bool playerWins(Player& player) {
    return player.hitCount == 15;
}
void resetPlayerData(Player& player, UI *ui, char connectionType) {
    initializeShipLocations(player);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j<10; j++) {
            player.board[i][j].setFillColor(sf::Color::Blue);
            player.trackingBoard[i][j].setFillColor(sf::Color::Blue);
        }
    }
    for (int i = 0; i < 5; i++) {
        player.shipShapes[i].setSize({60.f, player.ships[i].length * 60.f});
        player.ships[i].cellsHit = 0;
        player.ships[i].sunkReported = false;
        player.ships[i].isPlaced = false;
        player.ships[i].isDragged = false;
        player.ships[i].isVertical = true;
        player.ships[i].row = 0, player.ships[i].col = 0;
        player.hitCount = 0;
        player.isReady = false;
        player.playAgain = false;
        player.shipLocations.clear();
    }
    connectionType == 's'? player.myTurn = true: player.myTurn = false;
    player.opponentPlayAgain = false;
    ui->readyButton.setFillColor(customWhite);
    ui->placeShipsText.setString("Place Ships");
    ui->attackFeedback.setString("");
    ui->turnFeedback.setString("");
    ui->shipSunk.setString("");
    ui->rematchAttemptText.setString("");
    ui->placeShipsText.setPosition({820.f, 800.f});
    ui->playerLabel.setPosition({850.f, 30.f});
    ui->shipSunk.setPosition({900.f, 300.f});
}

void buildUI(Player& player, UI *gameUI) {
    gameUI->playerLabel.setString(player.myName);
    window.draw(gameUI->playerLabel);
    window.draw(gameUI->placeShipsText);
    window.draw(gameUI->shipLoader);
    window.draw(gameUI->instructionBorder);
    window.draw(gameUI->instructionText);
    window.draw(gameUI->readyButton);
    window.draw(gameUI->readyText);

    for (int i = 0; i < gameUI->instructions.size()-1; i++) {
        sf::Text line(font, gameUI->instructions[i]);
        line.setPosition({70.f, 360.f + i * 60.f});
        window.draw(line);
    }
    sf::Text line(font, gameUI->instructions[5]);
    line.setPosition({170.f, 630.f});
    window.draw(line);
    for (int i = 0; i < 5; i++) {
        window.draw(player.shipShapes[i]);
    }
}

int main() {
    Screen currentScreen = CONNECTING;
    char connectionType = 0;
    std::string hostIpInput;
    bool awaitingIpInput = false;

    sf::RectangleShape serverButton({250.f, 100.f});
    serverButton.setPosition({700.f, 400.f});
    serverButton.setFillColor(sf::Color::Blue);

    sf::Text serverButtonText(font, "Server");
    serverButtonText.setPosition({770.f, 430.f});

    sf::RectangleShape clientButton({250.f, 100.f});
    clientButton.setPosition({1000.f, 400.f});
    clientButton.setFillColor(sf::Color::Blue);

    sf::Text clientButtonText(font, "Client");
    clientButtonText.setPosition({1070.f, 430.f});

    sf::Text connectingPrompt(font, "Host or join a game?");
    connectingPrompt.setPosition({800.f, 300.f});

    sf::Text ipPrompt(font, "Enter host IP, then press Enter:");
    ipPrompt.setPosition({700.f, 300.f});

    sf::Text ipInputText(font);
    ipInputText.setPosition({700.f, 400.f});

    sf::Text connectionErrorText(font);
    connectionErrorText.setFillColor(sf::Color::Red);

    std::optional<sf::IpAddress> myIp = sf::IpAddress::getLocalAddress();

    sf::Text ip(font, "Hosting on: " + myIp->toString());
    ip.setPosition({750.f, 500.f});

    sf::Text waitingText(font, "Waiting for a client to connect...");
    waitingText.setPosition({700.f, 400.f});

    bool clicked = false;
    bool opponentIsReady = false;

    initializeShipLocations(me);
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            me.board[i][j].setSize({60.f, 60.f});
            me.board[i][j].setPosition({600.f + j * 60.f, 150.f + i * 60.f});
            me.board[i][j].setFillColor(sf::Color::Blue);
            me.board[i][j].setOutlineColor(sf::Color::Black);
            me.board[i][j].setOutlineThickness(0.f);
            me.trackingBoard[i][j].setSize({60.f, 60.f});
            me.trackingBoard[i][j].setPosition({1120.f + j * 60.f, 150.f + i * 60.f});
            me.trackingBoard[i][j].setFillColor(sf::Color::Blue);
            me.trackingBoard[i][j].setOutlineColor(sf::Color::Black);
            me.trackingBoard[i][j].setOutlineThickness(0.f);
        }
    }
    sf::TcpSocket socket;
    sf::TcpListener listener;
        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>())
                    window.close();
                if (awaitingIpInput) {
                    if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                        if (textEvent->unicode == 8) {
                            if (!hostIpInput.empty())
                                hostIpInput.pop_back();
                        }
                        else if (textEvent->unicode == 13) {
                            auto ip = sf::IpAddress::resolve(hostIpInput);
                            if (!ip.has_value()) {
                                connectionErrorText.setPosition({710.f, 470.f});
                                connectionErrorText.setString("Invalid IP address, try again.");
                                hostIpInput.clear();
                            } else {
                                awaitingIpInput = false;
                                connectionErrorText.setString("");
                                if (connectToHost(socket, ip.value()) == sf::Socket::Status::Done) {
                                    currentScreen = PLACING_SHIPS;
                                } else {
                                    connectionErrorText.setPosition({610.f, 470.f});
                                    connectionErrorText.setString("Connection failed, check the IP and try again.");
                                    hostIpInput.clear();
                                    awaitingIpInput = true;
                                }
                            }
                        }
                        else if ((textEvent->unicode >= '0' && textEvent->unicode <= '9') || textEvent->unicode == '.') {
                            hostIpInput += static_cast<char>(textEvent->unicode);
                        }
                    }
                }
            }

            auto mousePosition = sf::Vector2f(sf::Mouse::getPosition(window));
            window.clear();
            if (currentScreen == CONNECTING) {
                if (!awaitingIpInput) {
                    window.draw(connectingPrompt);
                    serverButton.setFillColor(serverButton.getGlobalBounds().contains(mousePosition) ?
                        sf::Color::Green : sf::Color::Blue);
                    clientButton.setFillColor(clientButton.getGlobalBounds().contains(mousePosition) ?
                        sf::Color::Green : sf::Color::Blue);
                    window.draw(serverButton);
                    window.draw(serverButtonText);
                    window.draw(clientButton);
                    window.draw(clientButtonText);

                    static bool wasClicked = false;
                    bool isClickedNow = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
                    bool clickedThisFrame = isClickedNow && !wasClicked;

                    if (clickedThisFrame && serverButton.getGlobalBounds().contains(mousePosition)) {
                        connectionType = 's'; me.myName = "Player 1"; me.opponentName = "Player 2"; me.myTurn = true;
                        startHosting(listener);
                        currentScreen = WAITING_FOR_CLIENT;
                    }
                    else if (clickedThisFrame && clientButton.getGlobalBounds().contains(mousePosition)) {
                        connectionType = 'c'; me.myName = "Player 2"; me.opponentName = "Player 1"; me.myTurn = false;
                        awaitingIpInput = true;
                    }
                    wasClicked = isClickedNow;
                }
                else {
                    window.draw(ipPrompt);
                    ipInputText.setString(hostIpInput);
                    window.draw(ipInputText);
                    window.draw(connectionErrorText);
                }
            }
            else if (currentScreen == WAITING_FOR_CLIENT) {
                 window.draw(waitingText);
                 window.draw(ip);
                 sf::Socket::Status status = tryAccept(listener, socket);
                 if (status == sf::Socket::Status::Done) {
                     currentScreen = PLACING_SHIPS;
                 }
             }
            else if (currentScreen == PLACING_SHIPS) {

                drawBoard(600.f, me.board, 10, &gameUI);
                buildUI(me, &gameUI);
                if (placeShips(me)) {
                    me.shipLocations = storeShipLocations(me.ships, 5);
                    if (!isOverlapping(me.shipLocations, me.ships, 5) &&
                        !isHanging(me.shipLocations, me.ships, 5)) {
                        gameUI.readyButton.setFillColor(sf::Color::Blue);
                        if (gameUI.readyButton.getGlobalBounds().contains(mousePosition)) {
                            gameUI.readyButton.setFillColor(sf::Color::Green);
                            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                                if (!clicked && !me.isReady) {
                                    clicked = true;
                                    me.isReady = true;
                                    sf::Packet ready;
                                    ready << me.isReady;
                                    socket.send(ready);
                                }
                            }
                        }
                    }
                    else if (!clicked){
                        gameUI.readyButton.setFillColor(customWhite);
                    }
                }
                else if (!clicked){
                    gameUI.readyButton.setFillColor(customWhite);
                }
                sf::Packet received;
                sf::Socket::Status status = socket.receive(received);
                if (status == sf::Socket::Status::Disconnected) {
                    currentScreen = DISCONNECTED;
                }
                if (status == sf::Socket::Status::Done) {
                        received >> opponentIsReady;
                }
                if (me.isReady && !opponentIsReady) {
                    gameUI.placeShipsText.setString("Ready! Waiting for " + me.opponentName + " ...");
                    gameUI.placeShipsText.setPosition({715.f, 800.f});
                    window.draw(gameUI.placeShipsText);
                }
                if (!me.isReady && opponentIsReady) {
                    gameUI.playerIsReadyText.setString(me.opponentName +" is ready!");
                    window.draw(gameUI.playerIsReadyText);
                }
                    if (me.isReady && opponentIsReady) {
                        currentScreen = BATTLESHIP;
                    }
                for (int i = 0; i < 5; i++) {
                    me.shipShapes[i].setFillColor(me.shipShapes[i].getGlobalBounds()
                        .contains(mousePosition) ?
                        sf::Color(255, 0, 0, 150) :
                        sf::Color(0, 255, 0, 150));
                }
            }
            else if (currentScreen == BATTLESHIP) {
                    gameUI.playerLabel.setPosition({910.f, 800.f});
                    drawBoard(200.f, me.board, 10, &gameUI);
                    drawBoard(1120.f, me.trackingBoard, 10, &gameUI);
                    window.draw(gameUI.battleText);
                    window.draw(gameUI.playerLabel);
                    window.draw(gameUI.attackFeedback);
                    window.draw(gameUI.turnFeedback);
                    window.draw(gameUI.shipSunk);
                    for (int i = 0; i < 5; i++) {
                        me.shipShapes[i].setPosition({(600.f + me.ships[i].col * 60.f) - 400.f,
                            150.f + me.ships[i].row * 60.f});
                        window.draw(me.shipShapes[i]);
                    }
                    sf::Packet incoming;
                    sf::Socket::Status status = socket.receive(incoming);
                    if (status == sf::Socket::Status::Disconnected) {
                        currentScreen = DISCONNECTED;
                    }
                    else if (status == sf::Socket::Status::Done) {
                            int message;
                            incoming >> message;
                            if (message == ATTACK) {
                                sf::Packet receivedAttack;
                                int rowHit, colHit;
                                message = HIT_RESULT;
                                incoming >> rowHit >> colHit;
                                sf::Packet isHit;
                                bool hitResult = handleOpponentAttack(me, rowHit, colHit);
                                gameUI.shipSunk.setString("");
                                gameUI.attackFeedback.setPosition({850.f, 400.f});
                                if (hitResult) {
                                    std::string attackCoordinate = "Opponent Attack: \n Hit! " +
                                        std::to_string(rowHit+1) + ":" + std::to_string(colHit+1);
                                    gameUI.attackFeedback.setString(attackCoordinate);
                                }
                                else if (!hitResult) {
                                    std::string attackCoordinate = "Opponent Attack: \n Miss! " +
                                        std::to_string(rowHit+1) + ":" + std::to_string(colHit+1);
                                    gameUI.attackFeedback.setString(attackCoordinate);
                                    me.myTurn = true;
                                }
                                isHit << message << hitResult << rowHit << colHit;
                                socket.send(isHit);
                                auto sunkShip = handleSunkShip(me);
                                if (sunkShip.has_value()) {
                                    sf::Packet sunkShipPacket;
                                    message = SUNK_SHIP;
                                    gameUI.shipSunk.setString( me.opponentName + " sunk your \n ship!");
                                    gameUI.shipSunk.setPosition({840.f, 250.f});
                                    sunkShipPacket << message << static_cast<std::int32_t>(sunkShip->size());
                                    for (const auto& location : *sunkShip) {
                                        sunkShipPacket << location.first << location.second;
                                        me.board[location.first][location.second].setFillColor(sf::Color::Magenta);
                                    }
                                    socket.send(sunkShipPacket);
                                }
                            }
                            else if (message == HIT_RESULT) {
                                bool isHit;
                                int rowHit, colHit;
                                incoming >> isHit >> rowHit >> colHit;
                                if (isHit) {
                                    std::string attackCoordinate = "Hit! " + std::to_string(rowHit+1)
                                    + ":" + std::to_string(colHit+1);
                                    gameUI.shipSunk.setString("");
                                    gameUI.attackFeedback.setPosition({900.f, 400.f});
                                    gameUI.attackFeedback.setString(attackCoordinate);
                                    me.hitCount++;
                                    me.trackingBoard[rowHit][colHit].setFillColor(sf::Color::Red);
                                    if (playerWins(me)) {
                                        gameUI.gameOver.setString("Game Over! " + me.myName + " won!");
                                        currentScreen = GAME_OVER;
                                        sf::Packet winDetected;
                                        winDetected << GAME_END;
                                        socket.send(winDetected);
                                    }
                                }
                                else if (!isHit) {
                                    gameUI.attackFeedback.setPosition({900.f, 400.f});
                                    std::string attackCoordinate = "Miss! " + std::to_string(rowHit+1)
                                    + ":" + std::to_string(colHit+1);
                                    gameUI.shipSunk.setString("");
                                    gameUI.attackFeedback.setString(attackCoordinate);
                                    me.trackingBoard[rowHit][colHit].setFillColor(customWhite);
                                    me.myTurn = false;
                                }
                            }
                        else if (message == SUNK_SHIP) {
                            int count;
                            incoming >> count;
                            for (int i = 0; i < count; i++) {
                                std::int32_t row, col;
                                incoming >> row >> col;
                                me.trackingBoard[row][col].setFillColor(sf::Color::Cyan);
                                gameUI.shipSunk.setString("Ship sunk!");
                                gameUI.shipSunk.setPosition({900.f, 300.f});
                            }
                        }
                            else if (message == GAME_END) {
                                gameUI.gameOver.setString("Game Over! " + me.opponentName + " won!");
                                currentScreen = GAME_OVER;
                            }
                        }
                    if (me.myTurn) {
                        gameUI.turnFeedback.setPosition({900.f, 350.f});
                        gameUI.turnFeedback.setString("Your turn");
                        std::pair<int, int> hitPair = attack(me);
                        if (hitPair.first != -1 && hitPair.second != -1) {
                            sf::Packet rowAndColHit;
                            rowAndColHit << ATTACK << hitPair.first << hitPair.second;
                            socket.send(rowAndColHit);
                        }
                    }
                else {
                    gameUI.turnFeedback.setString(me.opponentName + "'s Turn");
                    gameUI.turnFeedback.setPosition({870.f, 350.f});
                }
            }
            else if (currentScreen == GAME_OVER) {
                window.draw(gameUI.gameOver);
                window.draw(gameUI.playAgainButton);
                window.draw(gameUI.rematchText);
                window.draw(gameUI.rematchAttemptText);
                    drawBoard(600.f, me.board, 10, &gameUI);
                    for (int k = 0; k < 5; k++) {
                        me.shipShapes[k].setPosition({600.f + me.ships[k].col * 60.f,
                            150.f + me.ships[k].row * 60.f});
                        window.draw(me.shipShapes[k]);
                    }
                    if (gameUI.playAgainButton.getGlobalBounds().contains(mousePosition)) {
                        gameUI.playAgainButton.setFillColor(sf::Color::Magenta);
                        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !me.playAgain) {
                            sf::Packet playAgainPacket;
                            me.playAgain = true;
                            playAgainPacket << me.playAgain;
                            socket.send(playAgainPacket);
                        }
                    }
                    else {
                        gameUI.playAgainButton.setFillColor(sf::Color::Red);
                    }
                    sf::Packet ready;
                    if (socket.receive(ready) == sf::Socket::Status::Done) {
                        ready >> me.opponentPlayAgain;
                    }
                    if (me.playAgain && me.opponentPlayAgain) {
                        clicked = false;
                        opponentIsReady = false;
                        resetPlayerData(me, &gameUI, connectionType);
                        currentScreen = PLACING_SHIPS;
                    }
                else if (!me.playAgain && me.opponentPlayAgain) {
                    gameUI.rematchAttemptText.setString(me.opponentName + " wants to play again, rematch?");
                    gameUI.rematchAttemptText.setPosition({660.f, 780.f});
                }
                else if (me.playAgain && !me.opponentPlayAgain) {
                    gameUI.rematchAttemptText.setString("Rematch offer sent, waiting for " + me.opponentName + " ...");
                    gameUI.rematchAttemptText.setPosition({640.f, 780.f});
                }
            }
            else if (currentScreen == DISCONNECTED) {
                window.draw(gameUI.disconnectedText);
            }
            window.display();
        }
    return 0;
}