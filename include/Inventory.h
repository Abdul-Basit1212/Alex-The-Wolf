#ifndef INVENTORY_H
#define INVENTORY_H

#include "GameData.h"
#include <iostream>
#include <vector>

// Linked List Node
struct InventoryNode {
    Item data;
    InventoryNode* next;
    InventoryNode(Item item) : data(item), next(nullptr) {}
};

// Linked List Class
class InventoryList {
private:
    InventoryNode* head;
    int itemCount;
    const int MAX_ITEMS = 20;

public:
    InventoryList();
    ~InventoryList();

    // Actions
    bool addItem(Item newItem);
    bool removeOne(std::string itemName); 
    void clear(); 

    // Checkers
    bool hasItem(std::string itemName) const;
    bool isFull() const;
    bool isEmpty() const { return head == nullptr; }

    // Helper: Converts List to Vector so GUI can read it
    std::vector<Item> toVector() const;
};

#endif