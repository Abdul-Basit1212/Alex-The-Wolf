#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include <queue>
#include <vector>
#include <string>

// Structure for a dynamic event
struct GameEvent {
    int eventNodeID;    // The ID of the story node to trigger (e.g., 901, 902)
    int priority;       // Higher number = happens first
    std::string name;

    // Operator for priority_queue sorting (Highest priority first)
    bool operator<(const GameEvent& other) const {
        return priority < other.priority;
    }
};

class EventQueue {
private:
    std::priority_queue<GameEvent> pq;

public:
    void pushEvent(int nodeID, int priority, std::string name);
    int popEvent(); // Returns the Node ID of the event
    bool isEmpty() const;
    void clear();
};

#endif