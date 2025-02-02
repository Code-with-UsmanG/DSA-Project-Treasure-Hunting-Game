# DSA Project - Maze Game

A **Graphical User Interface (GUI)**-based application developed as part of the **Data Structures and Algorithms (DSA)** course project.

---

## Description

The **Maze Game** is an interactive, graphical C++ application that demonstrates the practical application of various data structures and algorithms. Players navigate through a maze with increasing difficulty, utilizing pathfinding algorithms to find a solution while avoiding obstacles.

This project uses a **GUI-based interface** built in C++ using Visual Studio 2022, providing an engaging user experience. 

The game now features **randomly generated mazes** instead of hardcoded ones, DFS and BFS are implemented to generate valid paths from the start to the finish of each level.

---

## Features

- **Customizable maze environment** for dynamic gameplay.
- **Randomly generated mazes** with each game session for varied challenges.
- **Pathfinding algorithm** DFS and BFS are used in this version to find a valid path from start to end.
- **Multiple difficulty levels** with increasing maze complexity. Total Levels are set to 5 but can be increased by just changing the value of Total Mazes globally.
- **Interactive gameplay** using intuitive commands to navigate the maze.
- **Bonus points** for completing levels efficiently.
- **Teleportation functionality** to transition between maze levels.
- **Smooth GUI** for enhanced visual appeal and user interaction.

---

## Technologies Used

- **Programming Language:** C++
- **Data Structures:** 3D vectors for maze representation.
- **Algorithms:** DFS and BFS are used in this version. Future versions will contain Pathfinding (Dijkstra’s Algorithm, etc.), maze generation algorithms.
- **Platform:** Graphical User Interface (GUI) using Visual Studio 2022.

---

## How to Run

1. Clone the repository to your local machine:
    ```sh
    git clone https://github.com/Code-with-UsmanG/DSA-Project-Maze-Game.git
    ```

2. Navigate to the project directory:
    ```sh
    cd DSA-Project-Maze-Game
    ```

3. Compile the source code using a C++ compiler:
    ```sh
    g++ -o maze_game main.cpp
    ```

4. Run the application:
    ```sh
    ./maze_game
    ```

---

## Folder Structure

```plaintext
📦DSA-Project-Maze-Game
 ┣ 📂src
 ┃ ┣ 📜main.cpp
 ┃ ┣ 📜game.cpp
 ┃ ┣ 📜game.h
 ┣ 📂assets
 ┃ ┗ 📜map.txt
 ┣ 📜README.md
 ┗ 📜LICENSE
