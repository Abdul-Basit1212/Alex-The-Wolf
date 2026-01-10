#ifndef INVENTORY_H
#define INVENTORY_H

#include <string>
#include <vector>
#include <iostream>

// ==========================================
// 1. MOVE ITEM DEFINITIONS HERE
// ==========================================
enum ItemType { FOOD, TOOL, WEAPON, HERB, QUEST };

struct Item {
    std::string name;
    ItemType type;
    int effectValue;
    int quantity;

    Item(std::string n = "", ItemType t = TOOL, int v = 0, int q = 1)
        : name(n), type(t), effectValue(v), quantity(q) {}
};

// ==========================================
// 2. INVENTORY LIST DEFINITIONS
// ==========================================

struct InventoryNode {
    Item data;
    InventoryNode* next;
    InventoryNode(Item item) : data(item), next(nullptr) {}
};

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