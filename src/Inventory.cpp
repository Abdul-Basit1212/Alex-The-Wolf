#include "Inventory.h"

InventoryList::InventoryList() {
    head = nullptr;
    itemCount = 0;
}

InventoryList::~InventoryList() {
    clear();
}

void InventoryList::clear() {
    InventoryNode* current = head;
    while (current != nullptr) {
        InventoryNode* next = current->next;
        delete current;
        current = next;
    }
    head = nullptr;
    itemCount = 0;
}

bool InventoryList::addItem(Item newItem) {
    // 1. Stack existing items
    InventoryNode* current = head;
    while (current != nullptr) {
        if (current->data.name == newItem.name) {
            current->data.quantity++;
            return true;
        }
        current = current->next;
    }

    // 2. Add new node
    if (itemCount >= MAX_ITEMS) return false;

    InventoryNode* newNode = new InventoryNode(newItem);
    if (head == nullptr) {
        head = newNode;
    } else {
        current = head;
        while (current->next != nullptr) {
            current = current->next;
        }
        current->next = newNode;
    }
    itemCount++;
    return true;
}

bool InventoryList::removeOne(std::string itemName) {
    if (head == nullptr) return false;

    InventoryNode* current = head;
    InventoryNode* prev = nullptr;

    while (current != nullptr) {
        if (current->data.name == itemName) {
            current->data.quantity--;
            if (current->data.quantity <= 0) {
                if (prev == nullptr) head = current->next;
                else prev->next = current->next;
                delete current;
                itemCount--;
            }
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

bool InventoryList::hasItem(std::string itemName) const {
    InventoryNode* current = head;
    while (current != nullptr) {
        if (current->data.name == itemName) return true;
        current = current->next;
    }
    return false;
}

bool InventoryList::isFull() const { return itemCount >= MAX_ITEMS; }

std::vector<Item> InventoryList::toVector() const {
    std::vector<Item> vec;
    InventoryNode* current = head;
    while (current != nullptr) {
        vec.push_back(current->data);
        current = current->next;
    }
    return vec;
}