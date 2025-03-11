# DSA Project - Maze Game

A **Graphical User Interface (GUI)**-based application developed as part of the **Data Structures and Algorithms (DSA)** course project.

---

## Description

The **Maze Game** is an interactive, graphical C++ application that demonstrates the practical application of various data structures and algorithms. Players navigate through a maze with increasing difficulty, utilizing **pathfinding algorithms** to find a solution while avoiding obstacles.

This project features a **GUI-based interface** built in **C++ using Visual Studio 2022**, ensuring an engaging user experience. Instead of predefined mazes, the game now **generates random mazes** dynamically using **Depth-First Search (DFS) and Breadth-First Search (BFS)** to create valid paths from the start to the finish of each level.

---

## Features

- **Dynamic maze generation** using DFS and BFS for unique challenges in every session.
- **Customizable maze environment** with varied obstacles and difficulty settings.
- **Pathfinding Algorithms** implemented to ensure solvable mazes.
- **Multiple difficulty levels** with increasing maze complexity. The default is **5 levels**, but this can be adjusted globally.
- **Interactive gameplay** with intuitive controls for smooth navigation.
- **Bonus points system** for faster level completion.
- **Teleportation functionality** to transition between maze levels.
- **Smooth GUI** for enhanced visual appeal and user interaction.

---

## Technologies Used

- **Programming Language:** C++
- **Data Structures:** 3D vectors for maze representation.
- **Algorithms:**  
  - **Maze Generation:** DFS and BFS (Implemented)  
  - **Pathfinding (Upcoming):** Dijkstra’s Algorithm, A* Search  
- **Platform:** GUI-based application built in **Visual Studio 2022**.

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
📦 DSA-Project-Maze-Game
 ┣ 📂 src
 ┃ ┣ 📜 main.cpp      # Main entry point
 ┃ ┣ 📜 game.cpp      # Core game logic
 ┃ ┣ 📜 game.h        # Header file
 ┣ 📂 assets
 ┃ ┗ 📜 map.txt       # Sample map data
 ┣ 📜 README.md       # Project documentation
 ┗ 📜 LICENSE         # License file
```
---

## Contributions

Feel free to fork the repo, suggest improvements, or report issues! 🚀  

📌 **GitHub Repository:** [DSA-Project-Maze-Game](https://github.com/Code-with-UsmanG/DSA-Project-Maze-Game)
```
