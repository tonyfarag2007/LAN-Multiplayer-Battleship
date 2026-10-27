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
    int hitCount = 0;
    bool hasPassed = false;
    bool isReady = false;
    bool playAgain = false;
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
    sf::Text playAgainText{font, "Play Again"};
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
        playAgainButton.setPosition({800.f, 800.f});
        playAgainText.setPosition({830.f, 830.f});
        disconnectedText.setPosition({730.f, 100.f});
        gameOver.setPosition({730.f, 100.f});
        verGridLine.setFillColor(sf::Color::Black);
        horGridLine.setFillColor(sf::Color::Black);
        playAgainButton.setFillColor(sf::Color::Red);
    }

};

Player playerOne, playerTwo; UI gameUI;

enum Screen {
    CONNECTING,
    CONNECTING_TO_HOST,
    WAITING_FOR_CLIENT,
    WELCOME,
    PLAYER_ONE,
    PLAYER_TWO,
    BATTLESHIP,
    GAME_OVER,
    DISCONNECTED
};
enum Winner {
    PLAYER_1,
    PLAYER_2
};
enum MessageType {
    READY,
    ATTACK,
    HIT_RESULT,
    TURN_DONE,
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
void resetPlayerData(Player& player, UI *ui) {
    initializeShipLocations(player);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j<10; j++) {
            player.board[i][j].setFillColor(sf::Color::Blue);
            player.trackingBoard[i][j].setFillColor(sf::Color::Blue);
        }
    }
    for (int i = 0; i < 5; i++) {
        player.shipShapes[i].setSize({60.f, player.ships[i].length * 60.f});
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

        ui->readyButton.setFillColor(customWhite);
        ui->placeShipsText.setString("Place Ships");
        ui->attackFeedback.setString("");
        ui->turnFeedback.setString("");
        ui->shipSunk.setString("");
        ui->placeShipsText.setPosition({820.f, 800.f});
        ui->playerLabel.setPosition({850.f, 30.f});
        ui->shipSunk.setPosition({900.f, 300.f});
    }
}

void buildUI(Player& player, UI *gameUI, char connectionType) {
    connectionType == 's'? gameUI->playerLabel.setString("Player 1"):
    gameUI->playerLabel.setString("Player 2");
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
    bool awaitingIpInput = false, isConnected = false;

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

    sf::Text connectingToHostText(font, "Connecting to host...");
    connectingToHostText.setPosition({700.f, 400.f});

    sf::Clock connectTimeoutClock;
    std::optional<sf::IpAddress> myIp = sf::IpAddress::getLocalAddress();

    sf::Text ip(font, "Hosting on: " + myIp->toString());
    ip.setPosition({750.f, 500.f});

    sf::Text waitingText(font, "Waiting for a client to connect...");
    waitingText.setPosition({700.f, 400.f});

    bool clicked = false;
    bool Clicked = false;
    bool isPlayerOneTurn = true, isPlayerTwoTurn = false;
    bool opponentIsReady = false;

    initializeShipLocations(playerOne);
    initializeShipLocations(playerTwo);
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            playerOne.board[i][j].setSize({60.f, 60.f});
            playerOne.board[i][j].setPosition({600.f + j * 60.f, 150.f + i * 60.f});
            playerOne.board[i][j].setFillColor(sf::Color::Blue);
            playerOne.board[i][j].setOutlineColor(sf::Color::Black);
            playerOne.board[i][j].setOutlineThickness(0.f);
            playerOne.trackingBoard[i][j].setSize({60.f, 60.f});
            playerOne.trackingBoard[i][j].setPosition({1120.f + j * 60.f, 150.f + i * 60.f});
            playerOne.trackingBoard[i][j].setFillColor(sf::Color::Blue);
            playerOne.trackingBoard[i][j].setOutlineColor(sf::Color::Black);
            playerOne.trackingBoard[i][j].setOutlineThickness(0.f);

            playerTwo.board[i][j].setSize({60.f, 60.f});
            playerTwo.board[i][j].setPosition({600.f + j * 60.f, 150.f + i * 60.f});
            playerTwo.board[i][j].setFillColor(sf::Color::Blue);
            playerTwo.board[i][j].setOutlineColor(sf::Color::Black);
            playerTwo.board[i][j].setOutlineThickness(0.f);
            playerTwo.trackingBoard[i][j].setSize({60.f, 60.f});
            playerTwo.trackingBoard[i][j].setPosition({1120.f + j * 60.f, 150.f + i * 60.f});
            playerTwo.trackingBoard[i][j].setFillColor(sf::Color::Blue);
            playerTwo.trackingBoard[i][j].setOutlineColor(sf::Color::Black);
            playerTwo.trackingBoard[i][j].setOutlineThickness(0.f);
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
                                    isConnected = true;
                                    currentScreen = PLAYER_TWO;
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
                        connectionType = 's';
                        startHosting(listener);
                        currentScreen = WAITING_FOR_CLIENT;
                    }
                    else if (clickedThisFrame && clientButton.getGlobalBounds().contains(mousePosition)) {
                        connectionType = 'c';
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
            else if (currentScreen == CONNECTING_TO_HOST) {
                window.draw(connectingToHostText);
                sf::Socket::Status status = tryConnect(socket);
                if (status == sf::Socket::Status::Done) {
                    isConnected = true;
                    currentScreen = PLAYER_TWO;
                }
                else if (connectTimeoutClock.getElapsedTime().asSeconds() > 5.f) {
                    socket.disconnect();
                    connectionErrorText.setString("Connection timed out, check the IP and try again.");
                    hostIpInput.clear();
                    awaitingIpInput = true;
                    currentScreen = CONNECTING;
                }
            }
            else if (currentScreen == WAITING_FOR_CLIENT) {
                 window.draw(waitingText);
                 window.draw(ip);
                 sf::Socket::Status status = tryAccept(listener, socket);
                 if (status == sf::Socket::Status::Done) {
                     isConnected = true;
                     currentScreen = PLAYER_ONE;
                 }
             }
            else if (currentScreen == PLAYER_ONE) {

                drawBoard(600.f, playerOne.board, 10, &gameUI);
                buildUI(playerOne, &gameUI, connectionType);

                if (placeShips(playerOne)) {

                    playerOne.shipLocations = storeShipLocations(playerOne.ships, 5);

                    if (!isOverlapping(playerOne.shipLocations, playerOne.ships, 5) &&
                        !isHanging(playerOne.shipLocations, playerOne.ships, 5)) {

                        gameUI.readyButton.setFillColor(sf::Color::Blue);
                        if (gameUI.readyButton.getGlobalBounds().contains(mousePosition)) {
                            gameUI.readyButton.setFillColor(sf::Color::Green);
                            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                                if (!clicked && !playerOne.isReady) {
                                    clicked = true;
                                    playerOne.isReady = true;
                                    sf::Packet ready;
                                    ready << playerOne.isReady;
                                    socket.send(ready);
                                }
                            }
                            else {
                                clicked = false;
                            }
                        }
                        else {
                            gameUI.readyButton.setFillColor(sf::Color::Blue);
                        }
                        window.draw(gameUI.readyButton);
                        window.draw(gameUI.readyText);
                    }
                }
                sf::Packet received;
                sf::Socket::Status status = socket.receive(received);
                if (status == sf::Socket::Status::Disconnected) {
                    currentScreen = DISCONNECTED;
                }
                if (status == sf::Socket::Status::Done) {
                        received >> opponentIsReady;
                }
                if (playerOne.isReady && !opponentIsReady) {
                    gameUI.placeShipsText.setString("Ready! Waiting for player 2 ...");
                    gameUI.placeShipsText.setPosition({700.f, 800.f});
                    window.draw(gameUI.placeShipsText);
                }
                if (!playerOne.isReady && opponentIsReady) {
                    gameUI.playerIsReadyText.setString("Player 2 is ready!");
                    window.draw(gameUI.playerIsReadyText);
                }
                    if (playerOne.isReady && opponentIsReady) {
                        currentScreen = BATTLESHIP;
                    }
                for (int i = 0; i < 5; i++) {
                    playerOne.shipShapes[i].setFillColor(playerOne.shipShapes[i].getGlobalBounds()
                        .contains(mousePosition) ?
                        sf::Color(255, 0, 0, 150) :
                        sf::Color(0, 255, 0, 150));
                }
            }
            else if (currentScreen == PLAYER_TWO) {

                drawBoard(600.f, playerTwo.board, 10, &gameUI);
                buildUI(playerTwo, &gameUI, connectionType);

                if (placeShips(playerTwo)) {
                    playerTwo.shipLocations = storeShipLocations(playerTwo.ships, 5);
                    if (!isOverlapping(playerTwo.shipLocations, playerTwo.ships, 5)
                        && !isHanging(playerTwo.shipLocations, playerTwo.ships, 5)) {
                        gameUI.readyButton.setFillColor(sf::Color::Blue);
                        if (gameUI.readyButton.getGlobalBounds().contains(mousePosition)) {
                            gameUI.readyButton.setFillColor(sf::Color::Green);
                            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                                if (!Clicked && !playerTwo.isReady) {
                                    Clicked = true;
                                    playerTwo.isReady = true;
                                    sf::Packet ready;
                                    ready << playerTwo.isReady;
                                    socket.send(ready);
                                    gameUI.placeShipsText.setString("Ready! Waiting for player 1 ...");
                                    gameUI.placeShipsText.setPosition({700.f, 800.f});
                                    window.draw(gameUI.placeShipsText);
                                }
                            }
                            else {
                                Clicked = false;
                            }
                        }
                        else {
                            gameUI.readyButton.setFillColor(sf::Color::Blue);
                        }
                        window.draw(gameUI.readyButton);
                        window.draw(gameUI.readyText);
                    }
                }
                sf::Packet received;
                sf::Socket::Status status  = socket.receive(received);
                if (status == sf::Socket::Status::Disconnected) {
                    currentScreen = DISCONNECTED;
                }
                if (status == sf::Socket::Status::Done) {
                    received >> opponentIsReady;
                }
                if (playerTwo.isReady && !opponentIsReady) {
                    gameUI.placeShipsText.setString("Ready! Waiting for player 1 ...");
                    gameUI.placeShipsText.setPosition({700.f, 800.f});
                    window.draw(gameUI.placeShipsText);
                }
                if (!playerTwo.isReady && opponentIsReady) {
                    gameUI.playerIsReadyText.setString("Player 1 is ready!");
                    window.draw(gameUI.playerIsReadyText);
                }
                if (playerTwo.isReady && opponentIsReady) {
                    currentScreen = BATTLESHIP;
                }
                for (int i = 0; i < 5; i++) {
                    playerTwo.shipShapes[i].setFillColor(playerTwo.shipShapes[i].getGlobalBounds()
                        .contains(mousePosition) ?
                        sf::Color(255, 0, 0, 150) :
                        sf::Color(0, 255, 0, 150));
                }
            }
            else if (currentScreen == BATTLESHIP) {
                if (connectionType == 's') {
                    gameUI.playerLabel.setPosition({910.f, 800.f});
                    drawBoard(200.f, playerOne.board, 10, &gameUI);
                    drawBoard(1120.f, playerOne.trackingBoard, 10, &gameUI);
                    window.draw(gameUI.battleText);
                    window.draw(gameUI.playerLabel);
                    window.draw(gameUI.attackFeedback);
                    window.draw(gameUI.turnFeedback);
                    window.draw(gameUI.shipSunk);
                    for (int i = 0; i < 5; i++) {
                        playerOne.shipShapes[i].setPosition({(600.f + playerOne.ships[i].col * 60.f) - 400.f,
                            150.f + playerOne.ships[i].row * 60.f});
                        window.draw(playerOne.shipShapes[i]);
                    }
                    sf::Packet incoming;
                    sf::Socket::Status status = socket.receive(incoming);
                    if (status == sf::Socket::Status::Disconnected) {
                        currentScreen = DISCONNECTED;
                    }
                    else if (status == sf::Socket::Status::Done) {
                            int message;
                            incoming >> message;
                            if (message == TURN_DONE) {
                                isPlayerOneTurn = true;
                            }
                            else if (message == ATTACK) {
                                sf::Packet receivedAttack;
                                int rowHit, colHit;
                                message = HIT_RESULT;
                                incoming >> rowHit >> colHit;
                                sf::Packet isHit;
                                bool hitResult = handleOpponentAttack(playerOne, rowHit, colHit);
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
                                }
                                isHit << message << hitResult << rowHit << colHit;
                                socket.send(isHit);
                                auto sunkShip = handleSunkShip(playerOne);
                                if (sunkShip.has_value()) {
                                    sf::Packet sunkShipPacket;
                                    message = SUNK_SHIP;
                                    gameUI.shipSunk.setString("Player 2 sunk your \n ship!");
                                    gameUI.shipSunk.setPosition({840.f, 250.f});
                                    sunkShipPacket << message << static_cast<std::int32_t>(sunkShip->size());
                                    for (const auto& location : *sunkShip) {
                                        sunkShipPacket << static_cast<std::int32_t>(location.first)
                                        << static_cast<std::int32_t>(location.second);
                                        playerOne.board[location.first][location.second].setFillColor(sf::Color::Magenta);
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
                                    playerOne.hitCount++;
                                    playerOne.trackingBoard[rowHit][colHit].setFillColor(sf::Color::Red);
                                    if (playerWins(playerOne)) {
                                        gameUI.gameOver.setString("Game Over! Player 1 won!");
                                        currentScreen = GAME_OVER;
                                        sf::Packet winDetected;
                                        winDetected << GAME_END;
                                        socket.send(winDetected);
                                    }
                                    else {
                                        isPlayerOneTurn = true;
                                    }
                                }
                                else if (!isHit) {
                                    gameUI.turnFeedback.setString("Player 2's Turn");
                                    gameUI.attackFeedback.setPosition({900.f, 400.f});
                                    std::string attackCoordinate = "Miss! " + std::to_string(rowHit+1)
                                    + ":" + std::to_string(colHit+1);
                                    gameUI.turnFeedback.setPosition({870.f, 350.f});
                                    gameUI.shipSunk.setString("");
                                    gameUI.attackFeedback.setString(attackCoordinate);
                                    playerOne.trackingBoard[rowHit][colHit].setFillColor(customWhite);
                                    message = TURN_DONE;
                                    sf::Packet playerTurnDone;
                                    playerTurnDone << message;
                                    socket.send(playerTurnDone);
                                }
                            }
                        else if (message == SUNK_SHIP) {
                            int count;
                            incoming >> count;
                            for (int i = 0; i < count; i++) {
                                std::int32_t row, col;
                                incoming >> row >> col;
                                playerOne.trackingBoard[row][col].setFillColor(sf::Color::Cyan);
                                gameUI.shipSunk.setString("Ship sunk!");
                                gameUI.shipSunk.setPosition({900.f, 300.f});
                            }
                        }
                            else if (message == GAME_END) {
                                gameUI.gameOver.setString("Game Over! Player 2 won!");
                                currentScreen = GAME_OVER;
                            }
                        }
                    if (isPlayerOneTurn) {
                        gameUI.turnFeedback.setPosition({900.f, 350.f});
                        gameUI.turnFeedback.setString("Your turn");
                        std::pair<int, int> hitPair = attack(playerOne);
                        if (hitPair.first != -1 && hitPair.second != -1) {
                            sf::Packet rowAndColHit;
                            rowAndColHit << ATTACK << hitPair.first << hitPair.second;
                            socket.send(rowAndColHit);
                            isPlayerOneTurn = false;
                        }
                    }
                }
                else if (connectionType == 'c') {
                    gameUI.playerLabel.setPosition({910.f, 800.f});
                    drawBoard(200.f, playerTwo.board, 10, &gameUI);
                    drawBoard(1120.f, playerTwo.trackingBoard, 10, &gameUI);
                    window.draw(gameUI.battleText);
                    window.draw(gameUI.playerLabel);
                    window.draw(gameUI.attackFeedback);
                    window.draw(gameUI.turnFeedback);
                    window.draw(gameUI.shipSunk);
            for (int i = 0; i < 5; i++) {
                playerTwo.shipShapes[i].setPosition({(600.f + playerTwo.ships[i].col * 60.f) - 400.f,
                    150.f + playerTwo.ships[i].row * 60.f});
                window.draw(playerTwo.shipShapes[i]);
            }
                    sf::Packet receivedAttack;
                    sf::Socket::Status status = socket.receive(receivedAttack);
                    if (status == sf::Socket::Status::Disconnected) {
                        currentScreen = DISCONNECTED;
                    }
                    else if (status == sf::Socket::Status::Done) {
                        int message;
                        receivedAttack >> message;
                        if (message == TURN_DONE) {
                            isPlayerTwoTurn = true;
                        }
                        else if (message == ATTACK) {
                            int rowHit, colHit;
                            receivedAttack >> rowHit >> colHit;
                            sf::Packet isHit;
                            message = HIT_RESULT;
                            bool hitResult = handleOpponentAttack(playerTwo, rowHit, colHit);
                            gameUI.shipSunk.setString("");
                            gameUI.attackFeedback.setPosition({850.f, 400.f});
                            if (hitResult) {
                                std::string attackCoordinate = "Opponent Attack: \n Hit! "
                                + std::to_string(rowHit+1) + ":" + std::to_string(colHit+1);
                                gameUI.attackFeedback.setString(attackCoordinate);
                            }
                            else if (!hitResult) {
                                std::string attackCoordinate = "Opponent Attack: \n Miss! "
                                + std::to_string(rowHit+1) + ":" + std::to_string(colHit+1);
                                gameUI.attackFeedback.setString(attackCoordinate);
                            }
                            isHit << message << hitResult << rowHit << colHit;
                            socket.send(isHit);
                            auto sunkShip = handleSunkShip(playerTwo);
                            if (sunkShip.has_value()) {
                                sf::Packet sunkShipPacket;
                                message = SUNK_SHIP;
                                gameUI.shipSunk.setString("Player 1 sunk your \n ship!");
                                gameUI.shipSunk.setPosition({840.f, 250.f});
                                sunkShipPacket << message << static_cast<std::int32_t>(sunkShip->size());
                                for (const auto& location : *sunkShip) {
                                    sunkShipPacket << static_cast<std::int32_t>(location.first)
                                    << static_cast<std::int32_t>(location.second);
                                    playerTwo.board[location.first][location.second].setFillColor(sf::Color::Magenta);
                                }
                                socket.send(sunkShipPacket);
                            }
                        }
                        else if (message == HIT_RESULT) {
                            bool isHit;
                            int rowHit, colHit;
                            receivedAttack >> isHit >> rowHit >> colHit;
                            if (isHit) {
                                std::string attackCoordinate = "Hit! " + std::to_string(rowHit+1)
                                + ":" + std::to_string(colHit+1);
                                gameUI.shipSunk.setString("");
                                gameUI.attackFeedback.setPosition({900.f, 400.f});
                                gameUI.attackFeedback.setString(attackCoordinate);
                                playerTwo.hitCount++;
                                playerTwo.trackingBoard[rowHit][colHit].setFillColor(sf::Color::Red);
                                if (playerWins(playerTwo)) {
                                    gameUI.gameOver.setString("Game Over! Player 2 won!");
                                    currentScreen = GAME_OVER;
                                    sf::Packet winDetected;
                                    winDetected << GAME_END;
                                    socket.send(winDetected);
                                }
                                else {
                                    isPlayerTwoTurn = true;
                                }
                            }
                            else if (!isHit) {
                                gameUI.turnFeedback.setString("Player 1's turn");
                                gameUI.attackFeedback.setPosition({900.f, 400.f});
                                std::string attackCoordinate = "Miss! " + std::to_string(rowHit+1)
                                + ":" + std::to_string(colHit+1);
                                gameUI.turnFeedback.setPosition({870.f, 350.f});
                                gameUI.shipSunk.setString("");
                                gameUI.attackFeedback.setString(attackCoordinate);
                                playerTwo.trackingBoard[rowHit][colHit].setFillColor(customWhite);
                                message = TURN_DONE;
                                sf::Packet playerTurnDone;
                                playerTurnDone << message;
                                socket.send(playerTurnDone);
                            }
                        }
                        else if (message == SUNK_SHIP) {
                            int count;
                            receivedAttack >> count;
                            for (int i = 0; i < count; i++) {
                                std::int32_t row, col;
                                receivedAttack >> row >> col;
                                playerTwo.trackingBoard[row][col].setFillColor(sf::Color::Cyan);
                                gameUI.shipSunk.setString("Ship sunk!");
                                gameUI.shipSunk.setPosition({900.f, 300.f});
                            }
                        }
                        else if (message == GAME_END) {
                            gameUI.gameOver.setString("Game Over! Player 1 won!");
                            currentScreen = GAME_OVER;
                        }
                    }
                    if (isPlayerTwoTurn) {
                        gameUI.turnFeedback.setPosition({900.f, 350.f});
                        gameUI.turnFeedback.setString("Your turn");
                        std::pair <int, int> hitPair = attack(playerTwo);
                        if (hitPair.first != -1 && hitPair.second != -1) {
                            sf::Packet rowAndHitCol;
                            rowAndHitCol << ATTACK << hitPair.first << hitPair.second;
                            socket.send(rowAndHitCol);
                            isPlayerTwoTurn = false;
                        }
                    }
                    else {
                        gameUI.turnFeedback.setString("Player 1's turn");
                        gameUI.turnFeedback.setPosition({870.f, 350.f});
                    }
                }
            }
            else if (currentScreen == GAME_OVER) {
                window.draw(gameUI.gameOver);
                window.draw(gameUI.playAgainButton);
                window.draw(gameUI.playAgainText);

                if (connectionType == 's') {
                    drawBoard(600.f, playerOne.board, 10, &gameUI);
                    for (int k = 0; k < 5; k++) {
                        playerOne.shipShapes[k].setPosition({600.f + playerOne.ships[k].col * 60.f,
                            150.f + playerOne.ships[k].row * 60.f});
                        window.draw(playerOne.shipShapes[k]);
                    }
                    if (gameUI.playAgainButton.getGlobalBounds().contains(mousePosition)) {
                        gameUI.playAgainButton.setFillColor(sf::Color::Magenta);
                        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !playerOne.playAgain) {
                            sf::Packet playAgainPacket;
                            playerOne.playAgain = true;
                            playAgainPacket << playerOne.playAgain;
                            socket.send(playAgainPacket);
                        }
                    }
                    else {
                        gameUI.playAgainButton.setFillColor(sf::Color::Red);
                    }
                    sf::Packet ready;
                    if (socket.receive(ready) == sf::Socket::Status::Done) {
                        ready >> playerTwo.playAgain;
                    }
                    if (playerOne.playAgain && playerTwo.playAgain) {
                        isPlayerOneTurn = true;
                        isPlayerTwoTurn = false;
                        clicked = false;
                        Clicked = false;
                        opponentIsReady = false;
                        resetPlayerData(playerOne, &gameUI); resetPlayerData(playerTwo, &gameUI);
                        currentScreen = PLAYER_ONE;
                    }
                }
                else if (connectionType == 'c') {
                    drawBoard(600.f, playerTwo.board, 10, &gameUI);
                    for (int k = 0; k < 5; k++) {
                        playerTwo.shipShapes[k].setPosition({600.f + playerTwo.ships[k].col * 60.f,
                            150.f + playerTwo.ships[k].row * 60.f});
                        window.draw(playerTwo.shipShapes[k]);
                    }
                    if (gameUI.playAgainButton.getGlobalBounds().contains(mousePosition)) {
                        gameUI.playAgainButton.setFillColor(sf::Color::Magenta);
                        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !playerTwo.playAgain) {
                            sf::Packet playAgain;
                            playerTwo.playAgain = true;
                            playAgain << playerTwo.playAgain;
                            socket.send(playAgain);
                        }
                    }
                    else {
                        gameUI.playAgainButton.setFillColor(sf::Color::Red);
                    }
                    sf::Packet ready;
                    if (socket.receive(ready) == sf::Socket::Status::Done) {
                        ready >> playerOne.playAgain;
                    }
                    if (playerTwo.playAgain && playerOne.playAgain) {
                        isPlayerOneTurn = true;
                        isPlayerTwoTurn = false;
                        clicked = false;
                        Clicked = false;
                        opponentIsReady = false;
                        gameUI.shipSunk.setPosition({900.f, 300.f});
                        resetPlayerData(playerOne, &gameUI); resetPlayerData(playerTwo, &gameUI);
                        currentScreen = PLAYER_TWO;
                    }
                }
            }
            else if (currentScreen == DISCONNECTED) {
                window.draw(gameUI.disconnectedText);
            }
            window.display();
        }
    return 0;
}