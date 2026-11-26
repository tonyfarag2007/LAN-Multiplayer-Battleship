#include <SFML/Network.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Graphics.hpp>
#include <cmath>
#include "Network.h"

#define INITIAL_BOARD_OFFSET_X 600.f
#define BOARD_LENGTH 600.f
#define CELL_LENGTH 60.f
#define INITIAL_BOARD_OFFSET_Y 150.f
#define BATTLESHIP_BOARD_OFFSET 160.f
#define BATTLESHIP_TRACKING_BOARD_OFFSET 1160.f

sf::RenderWindow window(sf::VideoMode({1920, 1080}),
    "Battleship", sf::Style::Titlebar | sf::Style::Close);
sf::Font font("AsapSharp.ttf");
sf::Color customWhite(255, 255, 255, 180);
sf::Color shipGreen(0, 255, 0, 150), shipRed(255, 0, 0, 150);
sf::Color sunkShipColor(80, 75, 0, 255);
class Ship {
public:
    int length, row, col, cellsHit;
    bool isVertical, isDragged = false, isPlaced = false, sunkReported = false;
    sf::Vector2f dragOffset;
};
class HitMark {
public:
    sf::RectangleShape xShape[2];
    HitMark() {
        xShape[0].setSize({std::sqrt(2.f * CELL_LENGTH * CELL_LENGTH), 5.f});
        xShape[0].setFillColor(sf::Color::Magenta);
        xShape[0].setRotation(sf::degrees(45));
        xShape[1].setSize({std::sqrt(2.f * CELL_LENGTH * CELL_LENGTH), 5.f});
        xShape[1].setFillColor(sf::Color::Magenta);
        xShape[1].setOrigin({std::sqrt(2.f * CELL_LENGTH * CELL_LENGTH), 0.f});
        xShape[1].setRotation(sf::degrees(315));
    }
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
    bool cellAttacked[10][10] = {false}; // tracks all attacked cells (not just hits)

    std::vector<std::vector<std::pair<int, int>>> shipLocations;

    std::vector<HitMark> trackingHitMarkLocations;
    std::vector<HitMark> hitMarkLocations;
    std::vector<HitMark> gameOverHitMarkLocations;

    std::string myName, opponentName;
    int hitCount = 0;
    bool myTurn = false;
    bool isReady = false, opponentIsReady = false;
    bool playAgain = false, opponentPlayAgain = false;
};
class UI {
public:
    sf::Text connectionErrorText{font}; sf::Text ip{font};
    sf::Text playerIsReadyText{font}; sf::Text coordinatesCol{font};
    sf::Text coordinatesRow{font}; sf::Text attackFeedback{font};
    sf::Text shipSunk{font}; sf::Text gameOverText{font};
    sf::Text rematchAttemptText{font}; sf::Text ipInputText{font};


    sf::Text serverButtonText{font, "Server"};
    sf::Text clientButtonText{font, "Client"};
    sf::Text connectingPrompt{font, "Host or join a game?"};
    sf::Text ipPrompt{font, "Enter host IP, then press Enter:"};
    sf::Text waitingTest{font, "Waiting for a client to connect..."};
    sf::Text instructionText{font, "INSTRUCTIONS"};
    sf::Text readyText{font, "Ready!"};
    sf::Text placeShipsText{font, "Place ships"};
    sf::Text playerLabel{font, "Player"};
    sf::Text battleText{font, "Battle!"};
    sf::Text turnFeedback{font, ""};
    sf::Text rematchText{font, "Rematch?"};
    sf::Text disconnectedText{font, "Opponent disconnected!"};

    sf::RectangleShape xMarker[2];
    sf::RectangleShape clientButton{{250.f, 100.f}};
    sf::RectangleShape instructionBorder{{450.f, 450.f}};
    sf::RectangleShape shipLoader{{200.f, 560.f}};
    sf::RectangleShape readyButton{{180.f, 90.f}};
    sf::RectangleShape serverButton{{250.f, 100.f}};
    sf::RectangleShape verGridLine{{1.f, 600.f}};
    sf::RectangleShape horGridLine{{600.f, 1.f}};
    sf::RectangleShape playAgainButton{{200.f, 100.f}};

    std::vector<std::string> instructions;

    UI() {
        ipInputText.setPosition({700.f, 400.f});
        serverButtonText.setPosition({770.f, 430.f});
        clientButton.setPosition({1000.f, 400.f});
        clientButtonText.setPosition({1070.f, 430.f});
        connectingPrompt.setPosition({800.f, 300.f});
        ipPrompt.setPosition({700.f, 300.f});
        ip.setPosition({750.f, 500.f});
        waitingTest.setPosition({700.f, 400.f});
        instructionText.setPosition({170.f, 280.f});
        instructionBorder.setPosition({50.f, 250.f});

        clientButton.setFillColor(sf::Color::Blue);
        connectionErrorText.setFillColor(sf::Color::Red);
        instructionBorder.setFillColor(sf::Color::Red);

        instructions = {
            "- Drag ships onto your board",
            "- Right-click a ship to rotate it",
            "- Ships cannot overlap",
            "- Ships cannot be hanging",
            "- Click ready when all ships",
            " are placed"
        };

        shipLoader.setPosition({1600.f, 160.f}); readyText.setPosition({1650.f, 775.f});
        playerIsReadyText.setPosition({150.f, 200.f}); readyButton.setPosition({1610.f, 750.f});
        readyButton.setFillColor(customWhite); serverButton.setPosition({700.f, 400.f});
        placeShipsText.setPosition({820.f, 800.f}); playerLabel.setPosition({850.f, 30.f});
        battleText.setPosition({910.f, 30.f}); attackFeedback.setPosition({900.f, 400.f});
        turnFeedback.setPosition({900.f, 350.f}); shipSunk.setPosition({900.f, 300.f});
        playAgainButton.setPosition({800.f, 840.f}); rematchText.setPosition({830.f, 870.f});
        disconnectedText.setPosition({730.f, 100.f}); gameOverText.setPosition({730.f, 35.f});
        serverButton.setFillColor(sf::Color::Blue); verGridLine.setFillColor(sf::Color::Black);
        horGridLine.setFillColor(sf::Color::Black); playAgainButton.setFillColor(sf::Color::Red);
    }

};

Player me; UI gameUI;

enum Screen {
    CONNECTING, WAITING_FOR_CLIENT,
    PLACING_SHIPS, GAMEPLAY_LOOP,
    GAME_OVER, DISCONNECTED
};
enum MessageType {
    ATTACK, HIT_RESULT,
    SUNK_SHIP, GAME_END
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
    char coordinateRow = 'A', coordinateCol = '0';
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 10; j++) {
            // print coordinates only in the first iteration
            if (i == 0) {
                gameUI.coordinatesRow.setString(coordinateRow);
                gameUI.coordinatesRow.setPosition({boardOffset - 40.f + i * CELL_LENGTH, 165.f + j * CELL_LENGTH});
                gameUI.coordinatesCol.setString(coordinateCol);
                gameUI.coordinatesCol.setPosition({boardOffset + 20.f + j * CELL_LENGTH, 105.f + i * CELL_LENGTH});
                window.draw(gameUI.coordinatesCol); window.draw(gameUI.coordinatesRow);
                coordinateCol += 1; coordinateRow += 1;
            }
            board[i][j].setPosition({boardOffset + j * CELL_LENGTH, INITIAL_BOARD_OFFSET_Y + i * CELL_LENGTH});
            window.draw(board[i][j]);
        }
    }
    for (int i = 0; i <= 10; i++) {
        ui->verGridLine.setPosition({boardOffset + i * CELL_LENGTH, INITIAL_BOARD_OFFSET_Y});
        ui->horGridLine.setPosition({boardOffset, INITIAL_BOARD_OFFSET_Y + i * CELL_LENGTH});
        window.draw(ui->horGridLine); window.draw(ui->verGridLine);
    }
}
void storeMarker(Player &player, int row, int col, bool isTrackingBoard) {
    HitMark xTrackingBoard, xGameOverBoard;
    if (isTrackingBoard) {
        xTrackingBoard.xShape[0].setPosition(player.trackingBoard[row][col].getPosition());
        xTrackingBoard.xShape[1].setPosition(player.trackingBoard[row][col].getPosition() + sf::Vector2f{CELL_LENGTH, 0.f});
        player.trackingHitMarkLocations.push_back(xTrackingBoard);
    }
    else {
        xTrackingBoard.xShape[0].setPosition(player.board[row][col].getPosition());
        xTrackingBoard.xShape[1].setPosition(player.board[row][col].getPosition() + sf::Vector2f{CELL_LENGTH, 0.f});

        xGameOverBoard.xShape[0].setPosition(player.board[row][col].getPosition() + sf::Vector2f(440.f, 0.f));
        xGameOverBoard.xShape[1].setPosition(player.board[row][col].getPosition() + sf::Vector2f(500.f, 0.f));
        player.hitMarkLocations.push_back(xTrackingBoard); player.gameOverHitMarkLocations.push_back(xGameOverBoard);
    }
}
void drawMarkers(std::vector<HitMark>& markerLocations) {
    if (markerLocations.empty()) return;
        for (const HitMark &X : markerLocations) {
            window.draw(X.xShape[0]);
            window.draw(X.xShape[1]);
        }
    }
void initializePlayerData(Player& player) {
    for (int i = 0; i < 5; i++) {
        player.shipShapes[i].setSize({CELL_LENGTH, player.ships[i].length * CELL_LENGTH});
    }
    player.shipShapes[0].setPosition({1620.f, 180.f}); player.shipShapes[1].setPosition({1720.f, 180.f});
    player.shipShapes[2].setPosition({1720.f, 440.f}); player.shipShapes[3].setPosition({1620.f, 500.f});
    player.shipShapes[4].setPosition({1620.f, 640.f});

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            player.board[i][j].setSize({CELL_LENGTH, CELL_LENGTH});
            player.board[i][j].setPosition({INITIAL_BOARD_OFFSET_X + j * CELL_LENGTH, INITIAL_BOARD_OFFSET_Y + i * CELL_LENGTH});
            player.board[i][j].setFillColor(sf::Color::Blue);
            player.board[i][j].setOutlineColor(sf::Color::Black);
            player.board[i][j].setOutlineThickness(0.f);

            player.trackingBoard[i][j].setSize({CELL_LENGTH, CELL_LENGTH});
            player.trackingBoard[i][j].setPosition({1120.f + j * CELL_LENGTH, INITIAL_BOARD_OFFSET_Y + i * CELL_LENGTH});
            player.trackingBoard[i][j].setFillColor(sf::Color::Blue);
            player.trackingBoard[i][j].setOutlineColor(sf::Color::Black);
            player.trackingBoard[i][j].setOutlineThickness(0.f);
        }
    }
}
bool isOverlapping(std::vector<std::vector<std::pair<int, int>>>& locations,
    Ship ships[], int size) {
    for (int g = 0; g < size; g++) {
        int shipIndex = g;
        for (int h = 0; h < size; h++){
            if (h == shipIndex) continue;
            for (int i = 0; i < ships[g].length; i++) {
                for (int j = 0; j < ships[h].length; j++) {
                    if (locations[g][i] == locations[h][j]) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
bool isHanging(std::vector<std::vector<std::pair<int, int>>>& locations,
    Ship ships[], int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < ships[i].length; j++) {
            if (locations[i][j].first > 9 || locations[i][j].first < 0
                || locations[i][j].second > 9 || locations[i][j].second < 0) {
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
                            player.shipShapes[i].setSize({player.ships[i].length * CELL_LENGTH, CELL_LENGTH});
                        }
                        else {
                            player.shipShapes[i].setSize({CELL_LENGTH, player.ships[i].length * CELL_LENGTH});
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
                        bool insideBoard = (pos.x >= INITIAL_BOARD_OFFSET_X && pos.x <= 2 * BOARD_LENGTH
                            && pos.y >= INITIAL_BOARD_OFFSET_Y && pos.y <= INITIAL_BOARD_OFFSET_Y + BOARD_LENGTH);
                        if (insideBoard) {
                            player.ships[i].col = static_cast<int>((pos.x - INITIAL_BOARD_OFFSET_X) / CELL_LENGTH);
                            player.ships[i].row = static_cast<int>((pos.y - INITIAL_BOARD_OFFSET_Y) / CELL_LENGTH);
                            player.shipShapes[i].setPosition({INITIAL_BOARD_OFFSET_X + player.ships[i].col * CELL_LENGTH,
                                INITIAL_BOARD_OFFSET_Y + player.ships[i].row * CELL_LENGTH});
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
                for (int i = 0; i<10; i++) {
                    for (int j = 0; j<10; j++) {
                            if (player.trackingBoard[i][j].getGlobalBounds().contains(mousePosition)
                                && !player.cellAttacked[i][j]) {
                                player.trackingBoard[i][j].setFillColor(sf::Color::Yellow);
                                if(leftPressedNow) {
                                    rowHit = i;
                                    colHit = j;
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
bool isHitFound(Player& player, int rowHit, int colHit) {
    bool isHit = false;
   for (int i = 0; i < 5; i++) {
       for (int j = 0; j<player.ships[i].length; j++) {
           if (player.shipLocations[i][j].first == rowHit && player.shipLocations[i][j].second == colHit) {
               player.board[rowHit][colHit].setFillColor(sf::Color::White);
               isHit = true;
               player.ships[i].cellsHit++;
           }
       }
   }
    if (!isHit) {
        player.board[rowHit][colHit].setFillColor(customWhite);
    }
    return isHit;
}

// player wins on 15 hits (5 + 4 + 3 + 2 + 1)
bool playerWins(Player& player) {
    return player.hitCount == 15;
}

void handleHitResult(Screen *currentScreen, sf::TcpSocket *socket,
    Player& player, UI *ui, bool hitFound, int row, int col){
    std::string attackCoordinate;
    player.cellAttacked[row][col] = true;
    ui->attackFeedback.setPosition({880.f, 400.f});
    ui->shipSunk.setString("");

    if (hitFound) {
        std::string attackCoordinate = std::string("Hit! ") + static_cast<char>(row + 65)
        + ":" + std::to_string(col);
        player.hitCount++;
        player.trackingBoard[row][col].setFillColor(sf::Color::Red);
        storeMarker(player, row, col, true);

        // win detector
        if (playerWins(player)) {
            ui->gameOverText.setString("Game Over! " + player.myName + " won!");
            *currentScreen = Screen::GAME_OVER;
            sf::Packet winDetected;
            winDetected << MessageType::GAME_END;
            socket->send(winDetected);
        }

    }
    else if (!hitFound) {
        std::string attackCoordinate = std::string("Miss! ") + static_cast<char>(row + 65)
        + ":" + std::to_string(col);
        player.trackingBoard[row][col].setFillColor(customWhite);
        player.myTurn = false;
    }
    ui->attackFeedback.setString(attackCoordinate);
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
void revealSunkShip(sf::TcpSocket &socket, Player &player, sf::Packet sunkShipPacket,
    std::optional<std::vector<std::pair<int, int>>> sunkShip) {
    for (const auto& location : *sunkShip) {
        sunkShipPacket << location.first << location.second;
    }
    socket.send(sunkShipPacket);
}

void resetPlayerData(Player& player, UI *ui, char connectionType) {
    initializePlayerData(player);

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j<10; j++) {
            player.board[i][j].setFillColor(sf::Color::Blue);
            player.trackingBoard[i][j].setFillColor(sf::Color::Blue);
            player.cellAttacked[i][j] = false;
        }
    }
    for (int i = 0; i < 5; i++) {
        player.shipShapes[i].setSize({CELL_LENGTH, player.ships[i].length * CELL_LENGTH});
        player.shipShapes[i].setFillColor(sf::Color(0, 255, 0, 150));
        player.ships[i].cellsHit = 0;
        player.ships[i].sunkReported = false; player.ships[i].isPlaced = false;
        player.ships[i].isDragged = false; player.ships[i].isVertical = true;
        player.ships[i].row = 0, player.ships[i].col = 0;
    }
    connectionType == 's'? player.myTurn = true: player.myTurn = false;
    player.hitCount = 0;
    player.isReady = false; player.playAgain = false;
    player.opponentPlayAgain = false; player.opponentIsReady = false;
    player.shipLocations.clear(); player.trackingHitMarkLocations.clear();
    player.hitMarkLocations.clear(); player.gameOverHitMarkLocations.clear();
    ui->readyButton.setFillColor(customWhite); ui->placeShipsText.setString("Place Ships");
    ui->attackFeedback.setString(""); ui->turnFeedback.setString("");
    ui->shipSunk.setString(""); ui->rematchAttemptText.setString("");
    ui->placeShipsText.setPosition({820.f, 800.f}); ui->playerLabel.setPosition({850.f, 30.f});
    ui->shipSunk.setPosition({900.f, 300.f});
}

void buildShipPlacementUI(Player& player, UI *gameUI) {
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

void buildGameplayUI(Player& player, UI *ui) {
    ui->playerLabel.setPosition({910.f, 800.f});
    drawBoard(BATTLESHIP_BOARD_OFFSET, player.board, 10, ui);
    drawBoard(BATTLESHIP_TRACKING_BOARD_OFFSET, player.trackingBoard, 10, ui);
    window.draw(ui->battleText);
    window.draw(ui->playerLabel);
    window.draw(ui->attackFeedback);
    window.draw(ui->turnFeedback);
    window.draw(ui->shipSunk);
    for (int i = 0; i < 5; i++) {
        player.shipShapes[i].setPosition({(INITIAL_BOARD_OFFSET_X + player.ships[i].col *
            CELL_LENGTH) - 440.f,
            INITIAL_BOARD_OFFSET_Y + player.ships[i].row * CELL_LENGTH});
        window.draw(player.shipShapes[i]);
    }
    drawMarkers(player.trackingHitMarkLocations); drawMarkers(player.hitMarkLocations);
}

int main() {
    Screen currentScreen = Screen::CONNECTING;
    char connectionType = 0;
    std::string hostIpInput;
    bool awaitingIpInput = false, clicked = false;

    std::optional<sf::IpAddress> myIp = sf::IpAddress::getLocalAddress();
    gameUI.ip.setString("Hosting on: " + myIp->toString());

    initializePlayerData(me);
    sf::TcpSocket socket; sf::TcpListener listener;
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
                                gameUI.connectionErrorText.setPosition({710.f, 470.f});
                                gameUI.connectionErrorText.setString("Invalid IP address, try again.");
                                hostIpInput.clear();
                            } else {
                                awaitingIpInput = false;
                                gameUI.connectionErrorText.setString("");
                                if (connectToHost(socket, ip.value()) == sf::Socket::Status::Done) {
                                    currentScreen = Screen::PLACING_SHIPS;
                                } else {
                                    gameUI.connectionErrorText.setPosition({610.f, 470.f});
                                    gameUI.connectionErrorText.setString("Connection failed, check the IP and try again.");
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
            if (currentScreen == Screen::CONNECTING) {
                if (!awaitingIpInput) {
                    window.draw(gameUI.connectingPrompt);
                    gameUI.serverButton.setFillColor(gameUI.serverButton.getGlobalBounds().contains(mousePosition) ?
                        sf::Color::Green : sf::Color::Blue);
                    gameUI.clientButton.setFillColor(gameUI.clientButton.getGlobalBounds().contains(mousePosition) ?
                        sf::Color::Green : sf::Color::Blue);
                    window.draw(gameUI.serverButton); window.draw(gameUI.serverButtonText);
                    window.draw(gameUI.clientButton); window.draw(gameUI.clientButtonText);

                    static bool wasClicked = false;
                    bool isClickedNow = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
                    bool clickedThisFrame = isClickedNow && !wasClicked;

                    if (clickedThisFrame && gameUI.serverButton.getGlobalBounds().contains(mousePosition)) {
                        connectionType = 's'; me.myName = "Player 1"; me.opponentName = "Player 2"; me.myTurn = true;
                        startHosting(listener);
                        currentScreen = Screen::WAITING_FOR_CLIENT;
                    }
                    else if (clickedThisFrame && gameUI.clientButton.getGlobalBounds().contains(mousePosition)) {
                        connectionType = 'c'; me.myName = "Player 2"; me.opponentName = "Player 1"; me.myTurn = false;
                        awaitingIpInput = true;
                    }
                    wasClicked = isClickedNow;
                }
                else {
                    gameUI.ipInputText.setString(hostIpInput);
                    window.draw(gameUI.ipPrompt);
                    window.draw(gameUI.ipInputText);
                    window.draw(gameUI.connectionErrorText);
                }
            }
            else if (currentScreen == Screen::WAITING_FOR_CLIENT) {
                 window.draw(gameUI.waitingTest);
                 window.draw(gameUI.ip);
                 sf::Socket::Status status = tryAccept(listener, socket);
                 if (status == sf::Socket::Status::Done) {
                     currentScreen = Screen::PLACING_SHIPS;
                 }
             }
            else if (currentScreen == Screen::PLACING_SHIPS) {

                drawBoard(INITIAL_BOARD_OFFSET_X, me.board, 10, &gameUI);
                buildShipPlacementUI(me, &gameUI);

                // ship placement and location storing logic
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

                // disconnect screen if player disconnects
                if (status == sf::Socket::Status::Disconnected) {
                    currentScreen = Screen::DISCONNECTED;
                }

                // confirms opponent is ready
                if (status == sf::Socket::Status::Done) {
                        received >> me.opponentIsReady;
                }

                // player ready but opponent not ready case
                if (me.isReady && !me.opponentIsReady) {
                    gameUI.placeShipsText.setString("Ready! Waiting for " + me.opponentName + " ...");
                    gameUI.placeShipsText.setPosition({715.f, 800.f});
                    window.draw(gameUI.placeShipsText);
                }

                // player not ready but opponent ready case
                if (!me.isReady && me.opponentIsReady) {
                    gameUI.playerIsReadyText.setString(me.opponentName +" is ready!");
                    window.draw(gameUI.playerIsReadyText);
                }

                // both players ready case
                if (me.isReady && me.opponentIsReady) {
                        currentScreen = Screen::GAMEPLAY_LOOP;
                    }

                // ship color decided on mouse position, custom colors used
                for (int i = 0; i < 5; i++) {
                    me.shipShapes[i].setFillColor(me.shipShapes[i].getGlobalBounds()
                        .contains(mousePosition) ?
                        sf::Color(shipRed) :
                        sf::Color(shipGreen));
                }
            }

            // main gameplay loop
            else if (currentScreen == Screen::GAMEPLAY_LOOP) {

                buildGameplayUI(me, &gameUI);

                    sf::Packet incoming;
                    sf::Socket::Status status = socket.receive(incoming);

                    if (status == sf::Socket::Status::Disconnected) {
                        currentScreen = Screen::DISCONNECTED;
                    }
                    else if (status == sf::Socket::Status::Done) {
                            int message;
                            incoming >> message;
                            if (message == MessageType::ATTACK) {
                                message = MessageType::HIT_RESULT;
                                sf::Packet receivedAttack;
                                int rowHit, colHit;
                                incoming >> rowHit >> colHit;
                                sf::Packet isHit;
                                bool hitResult = isHitFound(me, rowHit, colHit);
                                gameUI.shipSunk.setString("");
                                gameUI.attackFeedback.setPosition({825.f, 400.f});
                                if (hitResult) {
                                    std::string attackCoordinate = std::string("Opponent Attack: \n Hit! ") +
                                        static_cast<char>(rowHit + 65) + ":" + std::to_string(colHit);
                                    storeMarker(me, rowHit, colHit, false);
                                    gameUI.attackFeedback.setString(attackCoordinate);
                                }
                                else if (!hitResult) {
                                    std::string attackCoordinate = std::string("Opponent Attack: \n Miss! ") +
                                        static_cast<char>(rowHit + 65) + ":" + std::to_string(colHit);
                                    gameUI.attackFeedback.setString(attackCoordinate);
                                    me.myTurn = true;
                                }
                                isHit << message << hitResult << rowHit << colHit;
                                socket.send(isHit);
                                auto sunkShip = handleSunkShip(me);
                                if (sunkShip.has_value()) {
                                    message = MessageType::SUNK_SHIP;
                                    sf::Packet sunkShipPacket;
                                    gameUI.shipSunk.setString( me.opponentName + " sunk your \n ship!");
                                    gameUI.shipSunk.setPosition({820.f, 250.f});
                                    sunkShipPacket << message << static_cast<int>(sunkShip->size());
                                    for (int i = 0; i < 5; i++) {
                                        if (me.ships[i].length == me.ships[i].cellsHit) {
                                            me.shipShapes[i].setFillColor(sunkShipColor);
                                        }
                                    }

                                    /* sends the location of the sunk ship to the opponent in this form:
                                      message [shipSize] [row col] [row col] ... */

                                    revealSunkShip(socket, me, sunkShipPacket, sunkShip);
                                }
                            }
                            else if (message == MessageType::HIT_RESULT) {
                                bool isHit;
                                int rowHit, colHit;
                                incoming >> isHit >> rowHit >> colHit;
                                handleHitResult(&currentScreen, &socket, me, &gameUI, isHit, rowHit, colHit);
                            }
                            else if (message == MessageType::SUNK_SHIP) {
                            int count, row, col;
                            incoming >> count;
                            gameUI.shipSunk.setString("Ship sunk!");
                            gameUI.shipSunk.setPosition({880.f, 300.f});

                            // marking the sunk ship on the player's tracking board with sunkShipColor
                            for (int i = 0; i < count; i++) {
                                incoming >> row >> col;
                                me.trackingBoard[row][col].setFillColor(sunkShipColor);
                            }
                        }
                            // if the game is over
                            else if (message == MessageType::GAME_END) {
                                gameUI.gameOverText.setString("Game Over! " + me.opponentName + " won!");
                                currentScreen = Screen::GAME_OVER;
                            }
                        }
                    if (me.myTurn) {
                        gameUI.turnFeedback.setPosition({880.f, 350.f});
                        gameUI.turnFeedback.setString("Your turn");
                        std::pair<int, int> hitPair = attack(me);
                        if (hitPair.first != -1 && hitPair.second != -1) {
                            sf::Packet rowAndColHit;
                            rowAndColHit << MessageType::ATTACK << hitPair.first << hitPair.second;
                            socket.send(rowAndColHit);
                        }
                    }
                    else {
                    gameUI.turnFeedback.setString(me.opponentName + "'s Turn");
                    gameUI.turnFeedback.setPosition({850.f, 350.f});
                }
            }
            else if (currentScreen == Screen::GAME_OVER) {

                window.draw(gameUI.gameOverText); window.draw(gameUI.playAgainButton);
                window.draw(gameUI.rematchText); window.draw(gameUI.rematchAttemptText);
                drawBoard(INITIAL_BOARD_OFFSET_X, me.board, 10, &gameUI);

                    for (int i = 0; i < 5;i++) {
                        me.shipShapes[i].setPosition({INITIAL_BOARD_OFFSET_X + me.ships[i].col * CELL_LENGTH,
                            INITIAL_BOARD_OFFSET_Y + me.ships[i].row * CELL_LENGTH});
                        window.draw(me.shipShapes[i]);
                    }
                    drawMarkers(me.gameOverHitMarkLocations);
                    if (gameUI.playAgainButton.getGlobalBounds().contains(mousePosition)) {
                        gameUI.playAgainButton.setFillColor(sf::Color::Magenta);
                        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !me.playAgain) {
                            sf::Packet playAgainPacket;
                            me.playAgain = true;
                            playAgainPacket << me.playAgain;
                            socket.send(playAgainPacket);
                        }
                    }
                    else if (!gameUI.playAgainButton.getGlobalBounds().contains(mousePosition)
                        && !me.playAgain) {
                        gameUI.playAgainButton.setFillColor(sf::Color::Red);
                    }
                    sf::Packet ready;
                sf::Socket::Status status = socket.receive(ready);
                    if (status == sf::Socket::Status::Done) {
                        ready >> me.opponentPlayAgain;
                    }
                // disconnected case
                    else if (status == sf::Socket::Status::Disconnected) {
                    currentScreen = Screen::DISCONNECTED;
                }
                // both players ready case
                    if (me.playAgain && me.opponentPlayAgain) {
                        clicked = false;
                        me.opponentIsReady = false;
                        resetPlayerData(me, &gameUI, connectionType);
                        currentScreen = Screen::PLACING_SHIPS;
                    }

                // player not ready but opponent ready case
                    else if (!me.playAgain && me.opponentPlayAgain) {
                    gameUI.rematchAttemptText.setString(me.opponentName + " wants to play again, rematch?");
                    gameUI.rematchAttemptText.setPosition({660.f, 780.f});
                }

                // player ready but opponent not ready case
                    else if (me.playAgain && !me.opponentPlayAgain) {
                    gameUI.rematchAttemptText.setString("Rematch offer sent, waiting for " + me.opponentName + " ...");
                    gameUI.rematchAttemptText.setPosition({640.f, 780.f});
                }
            }
            else if (currentScreen == Screen::DISCONNECTED) {
                window.draw(gameUI.disconnectedText);
            }
            window.display();
        }
    return 0;
}