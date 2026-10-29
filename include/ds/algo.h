#ifndef DS_ALGO_H
#define DS_ALGO_H

#include "array.h"

template <typename T, typename Less>
int partition(Array<T>& items, int low, int high, Less less) {
    T pivot = items[high];
    int store = low;
    for (int i = low; i < high; ++i) {
        if (less(items[i], pivot)) {
            T temp = items[store];
            items[store] = items[i];
            items[i] = temp;
            ++store;
        }
    }
    T temp = items[store];
    items[store] = items[high];
    items[high] = temp;
    return store;
}

template <typename T, typename Less>
void quickSortRange(Array<T>& items, int low, int high, Less less) {
    if (low >= high) {
        return;
    }
    const int pivot = partition(items, low, high, less);
    quickSortRange(items, low, pivot - 1, less);
    quickSortRange(items, pivot + 1, high, less);
}

template <typename T, typename Less>
void quickSort(Array<T>& items, Less less) {
    if (items.size() > 1) {
        quickSortRange(items, 0, items.size() - 1, less);
    }
}

template <typename T, typename Key, typename GetKey>
int binarySearch(const Array<T>& items, const Key& key, GetKey getKey) {
    int low = 0;
    int high = items.size() - 1;
    while (low <= high) {
        const int mid = low + (high - low) / 2;
        const auto midKey = getKey(items[mid]);
        if (midKey == key) {
            return mid;
        }
        if (midKey < key) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

#endif
