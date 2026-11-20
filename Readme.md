# JUIT Club Hub

A small college-evaluation project for the clubs and events of Jaypee University of Information Technology (JUIT), Waknaghat, Solan, H.P. 

The backend is built entirely from scratch using C++ and raw sockets, without any external libraries or frameworks.

## Tech Stack
- **Frontend:** HTML, CSS, Vanilla JavaScript
- **Backend:** C++ (Standard Library + OS socket headers)
- **Design:** Editorial campus-poster look, responsive, CSS animations.

## Key Features & DSA Implementation
This project heavily focuses on core Data Structures and Algorithms (DSA) using the C++ Standard Template Library (STL):
- **`vector`**: Used for main storage (`vector<Club>`, `vector<Event>`, etc.).
- **`map`**: 
  - `map<string, vector<int>> skillIndex`: Maps a skill to the list of club indexes that need it.
  - `map<int, vector<int>> stallsByEvent`: Maps an event ID to its stall indexes.
- **`priority_queue`**: Used for the audition filter (`priority_queue<ClubMatch>`). Clubs are ranked by the highest number of matching skills (ties broken alphabetically).
- **`queue`**: Used for `interestQueue` to process student interests in a strict FIFO order.
- **`deque`**: Used for `recentSearches` to keep track of the last 5 skill searches.
- **`sort()`**: Used to sort upcoming events chronologically by start date.

## How to Run

### Prerequisites
- A C++ compiler (`g++` / MinGW).
- Any modern web browser.

### 1. Compile the Server
Open your terminal/command prompt inside the project folder.

**For Windows:**
```bash
g++ server.cpp -o server -std=c++11 -lws2_32