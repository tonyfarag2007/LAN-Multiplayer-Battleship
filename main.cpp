#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include "Network.h"
sf::RenderWindow window(sf::VideoMode({1920, 1080}), "Battleship", sf::Style::Titlebar | sf::Style::Close);
class Ship {
public:
    int length;
    int row, col;
    int cellsHit;
    bool isVertical, isDragged = false, isPlaced = false;
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
};
Player me, playerTwo;
enum screen {
    WELCOME,
    PLAYER_ONE,
    PLAYER_TWO,
    BATTLESHIP,
    GAME_OVER
};
enum winner {
    PLAYER_1,
    PLAYER_2
};
sf::Color customWhite(255, 255, 255, 180);
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
bool isOverlapping(std::vector<std::vector<std::pair<int, int>>> playerShipLocations, Ship ships[], int size) {
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
bool isHanging(std::vector<std::vector<std::pair<int, int>>> playerShipLocations, Ship ships[], int size) {
    for (int i = 0; i < 5; i++) {
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
    auto mousePosition = sf::Vector2f(sf::Mouse::getPosition(window));
    static bool wasLeftDown = false, wasRightDown = false;
            bool leftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left), rightDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
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
                            player.shipShapes[i].setPosition({600.f + player.ships[i].col * 60.f, 150.f + player.ships[i].row * 60.f});
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
void attack(Player& player, Player& otherPlayer) {
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
                                    int colHit = static_cast<int>((hitLocation.x - 1120.f)/60.f);
                                    int rowHit = static_cast<int>((hitLocation.y - 150.f)/60.f);
                                    for (int k = 0; k < 5; k++) {
                                        for (int l = 0; l < otherPlayer.ships[k].length; l++) {
                                            if (otherPlayer.shipLocations[k][l].first == rowHit &&
                                                otherPlayer.shipLocations[k][l].second == colHit) {
                                                player.trackingBoard[i][j].setFillColor(sf::Color::Red);
                                                otherPlayer.board[i][j].setFillColor(sf::Color::Red);
                                                player.hitCount++;
                                                hit = true;
                                                otherPlayer.ships[k].cellsHit++;
                                                if (otherPlayer.ships[k].cellsHit == otherPlayer.ships[k].length) {
                                                    for (int m = 0; m < otherPlayer.ships[k].length; m++) {
                                                        player.trackingBoard[otherPlayer.shipLocations[k][m]
                                                            .first][otherPlayer.shipLocations[k][m].second]
                                                        .setFillColor(sf::Color::Cyan);
                                                        otherPlayer.board[otherPlayer.shipLocations[k][m].first]
                                                        [otherPlayer.shipLocations[k][m].second]
                                                        .setFillColor(sf::Color::Cyan);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (!hit) {
                                        player.trackingBoard[i][j].setFillColor(customWhite);
                                        otherPlayer.board[i][j].setFillColor(customWhite);
                                        player.hasPassed = true;
                                    }
                                }
                            }
                        else if (!player.trackingBoard[i][j].getGlobalBounds().contains(mousePosition)
                            && player.trackingBoard[i][j].getFillColor() == sf::Color::Yellow) {
                            player.trackingBoard[i][j].setFillColor(sf::Color::Blue);
                        }
                    }
                }
    wasLeftDown = leftDown;
}
bool playerWins(Player& player) {
    return player.hitCount == 15;
}
int main() {
    sf::RectangleShape rect({1920.f, 540.f});
    sf::RectangleShape startButton({200.f, 100.f});
    rect.setFillColor(sf::Color::Red);
    sf::Font font("C:/Windows/Fonts/arial.ttf");
    sf::Text welcomeText(font);
    sf::Text buttonText(font);
    sf::Text battleText(font);
    sf::Text gameOver(font);
    gameOver.setPosition({800.f, 540});
    battleText.setString("Battle!");
    battleText.setPosition({910.f, 30.f});
    welcomeText.setString("Battleship");
    buttonText.setString("Start");
    welcomeText.setFillColor(sf::Color::Black);
    buttonText.setFillColor(sf::Color::Blue);
    welcomeText.setPosition({850.f, 230.f});
    buttonText.setPosition({875.f, 700.f});
    startButton.setFillColor(sf::Color::White);
    startButton.setPosition({810.f, 670.f});
    sf::Text playerOneText(font);
    playerOneText.setString("Player 1");
    sf::Text player2(font);
    player2.setString("Player 2");
    sf::Text placeShipsText(font);
    placeShipsText.setString("Place Ships");
    sf::RectangleShape nextPlayer({180.f, 90.f});
    nextPlayer.setFillColor(sf::Color::Blue);
    nextPlayer.setPosition({1610.f, 750.f});
    sf::Text playerTwoText(font);
    playerTwoText.setString("Player 2");
    sf::Text battle(font);
    battle.setString("Battle!");
    battle.setPosition({1655.f, 775.f});
    playerTwoText.setPosition({1640.f, 775.f});
    sf::RectangleShape shipLoader({200.f, 560.f});
    shipLoader.setPosition({1600.f, 160.f});
    sf::RectangleShape battleButton({180.f, 90.f});
    battleButton.setPosition({1610.f, 750.f});
    battleButton.setFillColor(sf::Color::Blue);
    sf::RectangleShape nextTurn({180.f, 90.f});
    nextTurn.setPosition({1400.f, 800.f});
    nextTurn.setFillColor(sf::Color::Yellow);
    bool isClicked = false;
    bool clicked = false;
    bool Clicked = false;
    bool isPlayerOneTurn = true, isPlayerTwoTurn = false;
    int winner;
    screen currentScreen = WELCOME;
    for (int i = 0; i < 5; i++) {
        me.shipShapes[i].setSize({60.f, me.ships[i].length * 60.f});
        playerTwo.shipShapes[i].setSize({60.f, playerTwo.ships[i].length * 60.f});
    }
    me.shipShapes[0].setPosition({1620.f, 180.f});
    me.shipShapes[1].setPosition({1720.f, 180.f});
    me.shipShapes[2].setPosition({1720.f, 440.f});
    me.shipShapes[3].setPosition({1620.f, 500.f});
    me.shipShapes[4].setPosition({1620.f, 640.f});
    playerTwo.shipShapes[0].setPosition({1620.f, 180.f});
    playerTwo.shipShapes[1].setPosition({1720.f, 180.f});
    playerTwo.shipShapes[2].setPosition({1720.f, 440.f});
    playerTwo.shipShapes[3].setPosition({1620.f, 500.f});
    playerTwo.shipShapes[4].setPosition({1620.f, 640.f});
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
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        auto mousePosition = sf::Vector2f(sf::Mouse::getPosition(window));
        window.clear();
        if (currentScreen == WELCOME) {
            if (startButton.getGlobalBounds().contains(mousePosition)) {
                startButton.setFillColor(sf::Color::Blue);
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    if (!isClicked) {
                        std::cout <<"Mouse click!" << std::endl;
                        isClicked = true;
                        currentScreen = PLAYER_ONE;
                    }
                }
                else {
                    isClicked = false;
                }
            }
            else {
                startButton.setFillColor(sf::Color::White);
            }
        }
        if (currentScreen == WELCOME) {
            window.draw(rect);
            window.draw(welcomeText);
            window.draw(startButton);
            window.draw(buttonText);
        }
        else if (currentScreen == PLAYER_ONE){
            playerOneText.setPosition({850.f, 30.f});
            placeShipsText.setPosition({820.f, 800.f});
            window.draw(playerOneText);
            window.draw(placeShipsText);
            window.draw(shipLoader);
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    window.draw(me.board[i][j]);
                }
            }
            for (int j = 0; j <= 10; j++) {
                sf::RectangleShape lineOne({1.f, 600.f});
                lineOne.setFillColor(sf::Color::Black);
                lineOne.setPosition({600.f + j * 60.f, 150.f});
                window.draw(lineOne);
            }
            for (int i = 0; i <= 10; i++) {
                sf::RectangleShape lineOne({600.f, 1.f});
                lineOne.setFillColor(sf::Color::Black);
                lineOne.setPosition({600.f, 150.f + i * 60.f});
                window.draw(lineOne);
            }
            if (placeShips(me)) {
                me.shipLocations = storeShipLocations(me.ships, 5);
                if (!isOverlapping(me.shipLocations, me.ships, 5) && !isHanging(me.shipLocations, me.ships, 5)) {
                    window.draw(nextPlayer);
                    window.draw(playerTwoText);
                    if (nextPlayer.getGlobalBounds().contains(mousePosition)) {
                        nextPlayer.setFillColor(sf::Color::Green);
                        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                            if (!clicked) {
                                currentScreen = PLAYER_TWO;
                                clicked = true;
                                std::cout<<"Click!"<<std::endl;
                            }
                        }
                        else {
                            clicked = false;
                        }
                    }
                    else {
                        nextPlayer.setFillColor(sf::Color::Blue);
                    }
                }
            }
            for (int i = 0; i < 5; i++) {
                me.shipShapes[i].setFillColor(me.shipShapes[i].getGlobalBounds().contains(mousePosition) ?
                    sf::Color(255, 0, 0, 150) : sf::Color(0, 255, 0, 150));
            }
            for (int i = 0; i < 5; i++) {
                window.draw(me.shipShapes[i]);
            }
        }
        else if (currentScreen == PLAYER_TWO){
            player2.setPosition({850.f, 30.f});
            window.draw(player2);
            window.draw(placeShipsText);
            window.draw(shipLoader);
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    window.draw(playerTwo.board[i][j]);
                }
            }
            for (int j = 0; j <= 10; j++) {
                sf::RectangleShape lineTwoVertical({1.f, 600.f});
                lineTwoVertical.setFillColor(sf::Color::Black);
                lineTwoVertical.setPosition({600.f + j * 60.f, 150.f});
                window.draw(lineTwoVertical);
            }
            for (int i = 0; i <= 10; i++) {
                sf::RectangleShape lineTwoHorizontal({600.f, 1.f});
                lineTwoHorizontal.setFillColor(sf::Color::Black);
                lineTwoHorizontal.setPosition({600.f, 150.f + i * 60.f});
                window.draw(lineTwoHorizontal);
            }
            for (int i = 0; i < 5; i++) {
                window.draw(playerTwo.shipShapes[i]);
            }
            if (placeShips(playerTwo)) {
                playerTwo.shipLocations = storeShipLocations(playerTwo.ships, 5);
                if (!isOverlapping(playerTwo.shipLocations, playerTwo.ships, 5) && !isHanging(playerTwo.shipLocations, playerTwo.ships, 5)) {
                    window.draw(battleButton);
                    window.draw(battle);
                    if (battleButton.getGlobalBounds().contains(mousePosition)) {
                        battleButton.setFillColor(sf::Color::Green);
                        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                            if (!Clicked) {
                                Clicked = true;
                                std::cout<<"Click!"<<std::endl;
                                currentScreen = BATTLESHIP;
                            }
                        }
                        else {
                            Clicked = false;
                        }
                    }
                    else {
                        battleButton.setFillColor(sf::Color::Blue);
                    }
                }
            }
            for (int i = 0; i < 5; i++) {
                playerTwo.shipShapes[i].setFillColor(playerTwo.shipShapes[i].getGlobalBounds().contains(mousePosition) ?
                    sf::Color(255, 0, 0, 150) : sf::Color(0, 255, 0, 150));
            }
        }
        else if (currentScreen == BATTLESHIP) {
            static bool wasLeftDown = false, wasRightDown = false;
            bool leftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
            rightDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
            bool leftPressedNow = leftDown && !wasLeftDown, rightPressedNow = rightDown && !wasRightDown;
            bool releasedNow = !leftDown && wasLeftDown;
            playerOneText.setPosition({910.f, 800.f});
            if (isPlayerOneTurn) {
                if (playerWins(me)) {
                    currentScreen = GAME_OVER;
                    winner = PLAYER_1;
                }
                for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    me.board[i][j].setPosition({200.f + j * 60.f, 150.f + i * 60.f });
                    playerTwo.board[i][j].setPosition({200.f + j * 60.f, 150.f + i * 60.f });
                    window.draw(me.board[i][j]);
                    window.draw(me.trackingBoard[i][j]);
                }
            }
            window.draw(battleText);
            window.draw(playerOneText);
            for (int j = 0; j <= 10; j++) {
                sf::RectangleShape lineOneVertical({1.f, 600.f});
                lineOneVertical.setFillColor(sf::Color::Black);
                lineOneVertical.setPosition({1120.f + j * 60.f, 150.f});
                window.draw(lineOneVertical);
            }
            for (int i = 0; i <= 10; i++) {
                sf::RectangleShape lineOneHorizontal({600.f, 1.f});
                lineOneHorizontal.setFillColor(sf::Color::Black);
                lineOneHorizontal.setPosition({1120.f, 150.f + i * 60.f});
                window.draw(lineOneHorizontal);
            }
            for (int j = 0; j <= 10; j++) {
                sf::RectangleShape lineOneTrackingVertical({1.f, 600.f});
                lineOneTrackingVertical.setFillColor(sf::Color::Black);
                lineOneTrackingVertical.setPosition({200.f + j * 60.f, 150.f});
                window.draw(lineOneTrackingVertical);
            }
            for (int i = 0; i <= 10; i++) {
                sf::RectangleShape lineOneTrackingHorizontal({600.f, 1.f});
                lineOneTrackingHorizontal.setFillColor(sf::Color::Black);
                lineOneTrackingHorizontal.setPosition({200.f, 150.f + i * 60.f});
                window.draw(lineOneTrackingHorizontal);
            }
            for (int i = 0; i < 5; i++) {
                me.shipShapes[i].setPosition({(600.f + me.ships[i].col * 60.f) - 400.f, 150.f + me.ships[i].row * 60.f});
                playerTwo.shipShapes[i].setPosition({(600.f + playerTwo.ships[i].col * 60.f) - 400.f, 150.f + playerTwo.ships[i].row * 60.f});
                window.draw(me.shipShapes[i]);
            }
                attack(me, playerTwo);
                if (me.hasPassed) {
                    sf::Text player2(font);
                    player2.setString("Player 2");
                    player2.setFillColor(sf::Color::Black);
                    player2.setPosition({1430.f, 825.f});
                    window.draw(nextTurn);
                    window.draw(player2);
                    if (nextTurn.getGlobalBounds().contains(mousePosition)) {
                        nextTurn.setFillColor(sf::Color::Blue);
                        if (leftPressedNow) {
                            isPlayerOneTurn = false;
                            me.hasPassed = false;
                            isPlayerTwoTurn = true;
                        }
                    }
                    else {
                        nextTurn.setFillColor(sf::Color::Yellow);
                    }
                }
            }
             else if (isPlayerTwoTurn) {
                 if (playerWins(playerTwo)) {
                     currentScreen = GAME_OVER;
                     winner = PLAYER_2;
                 }
                player2.setPosition({910.f, 800.f});
                window.draw(battleText);
                window.draw(player2);
                for (int i = 0; i<10; i++) {
                    for (int j = 0; j<10; j++) {
                        window.draw(playerTwo.board[i][j]);
                        window.draw(playerTwo.trackingBoard[i][j]);
                    }
                }
                for (int j = 0; j <= 10; j++) {
                    sf::RectangleShape lineTwoVertical({1.f, 600.f});
                    lineTwoVertical.setFillColor(sf::Color::Black);
                    lineTwoVertical.setPosition({1120.f + j * 60.f, 150.f});
                    window.draw(lineTwoVertical);
                }
                for (int i = 0; i <= 10; i++) {
                    sf::RectangleShape lineTwoHorizontal({600.f, 1.f});
                    lineTwoHorizontal.setFillColor(sf::Color::Black);
                    lineTwoHorizontal.setPosition({1120.f, 150.f + i * 60.f});
                    window.draw(lineTwoHorizontal);
                }
                for (int j = 0; j <= 10; j++) {
                    sf::RectangleShape lineTwoTrackingVertical({1.f, 600.f});
                    lineTwoTrackingVertical.setFillColor(sf::Color::Black);
                    lineTwoTrackingVertical.setPosition({200.f + j * 60.f, 150.f});
                    window.draw(lineTwoTrackingVertical);
                }
                for (int i = 0; i <= 10; i++) {
                    sf::RectangleShape lineTwoTrackingHorizontal({600.f, 1.f});
                    lineTwoTrackingHorizontal.setFillColor(sf::Color::Black);
                    lineTwoTrackingHorizontal.setPosition({200.f, 150.f + i * 60.f});
                    window.draw(lineTwoTrackingHorizontal);
                }
                for (int i = 0; i<5; i++) {
                    window.draw(playerTwo.shipShapes[i]);
                }
                attack(playerTwo, me);
                 if (playerTwo.hasPassed) {
                     sf::Text player1(font);
                     player1.setString("Player 1");
                     player1.setFillColor(sf::Color::Black);
                     player1.setPosition({1430.f, 825.f});
                     window.draw(nextTurn);
                     window.draw(player1);
                     if (nextTurn.getGlobalBounds().contains(mousePosition)) {
                         nextTurn.setFillColor(sf::Color::Blue);
                         if (leftPressedNow) {
                             isPlayerTwoTurn = false;
                             playerTwo.hasPassed = false;
                             isPlayerOneTurn = true;
                         }
                     }
                     else {
                         nextTurn.setFillColor(sf::Color::Yellow);
                     }
                 }
            }
        }
        else if (currentScreen == GAME_OVER) {
            switch (winner) {
                case PLAYER_1:
                    gameOver.setString("Game over, Player 1 Won!");
                    break;
                case PLAYER_2:
                    gameOver.setString("Game over, Player 2 Won!");
                    break;
            }
            window.draw(gameOver);
        }
        window.display();
    }
    return 0;
}