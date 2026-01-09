#include "EventQueue.h"

void EventQueue::pushEvent(int nodeID, int priority, std::string name) {
    pq.push({nodeID, priority, name});
}

int EventQueue::popEvent() {
    if (pq.empty()) return -1;
    GameEvent e = pq.top();
    pq.pop();
    return e.eventNodeID;
}

bool EventQueue::isEmpty() const {
    return pq.empty();
}

void EventQueue::clear() {
    while (!pq.empty()) pq.pop();
}