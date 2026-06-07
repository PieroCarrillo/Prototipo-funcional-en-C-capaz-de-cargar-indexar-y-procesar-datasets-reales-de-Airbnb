#include "RangeAnalytics.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <numeric>

namespace {
using Clock = std::chrono::steady_clock;

double elapsedMs(const Clock::time_point& start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

class SegmentTree {
public:
    explicit SegmentTree(const std::vector<double>& values) {
        size_ = 1;
        while (size_ < values.size()) {
            size_ *= 2;
        }
        tree_.assign(size_ * 2, 0.0);
        for (std::size_t i = 0; i < values.size(); ++i) {
            tree_[size_ + i] = values[i];
        }
        for (std::size_t i = size_ - 1; i > 0; --i) {
            tree_[i] = tree_[i * 2] + tree_[i * 2 + 1];
        }
    }

    double query(std::size_t left, std::size_t right) const {
        double result = 0.0;
        left += size_;
        right += size_;
        while (left <= right) {
            if (left % 2 == 1) {
                result += tree_[left++];
            }
            if (right % 2 == 0) {
                result += tree_[right--];
            }
            left /= 2;
            right /= 2;
        }
        return result;
    }

    std::size_t memoryBytes() const {
        return tree_.capacity() * sizeof(double);
    }

private:
    std::size_t size_ = 1;
    std::vector<double> tree_;
};

class LazySegmentTree {
public:
    explicit LazySegmentTree(const std::vector<double>& values) {
        size_ = 1;
        while (size_ < values.size()) {
            size_ *= 2;
        }
        tree_.assign(size_ * 2, 0.0);
        lazy_.assign(size_ * 2, 0.0);
        for (std::size_t i = 0; i < values.size(); ++i) {
            tree_[size_ + i] = values[i];
        }
        for (std::size_t i = size_ - 1; i > 0; --i) {
            tree_[i] = tree_[i * 2] + tree_[i * 2 + 1];
        }
    }

    void rangeAdd(std::size_t left, std::size_t right, double value) {
        rangeAdd(1, 0, size_ - 1, left, right, value);
    }

    double rangeSum(std::size_t left, std::size_t right) {
        return rangeSum(1, 0, size_ - 1, left, right);
    }

    std::size_t memoryBytes() const {
        return (tree_.capacity() + lazy_.capacity()) * sizeof(double);
    }

private:
    std::size_t size_ = 1;
    std::vector<double> tree_;
    std::vector<double> lazy_;

    void apply(std::size_t node, std::size_t left, std::size_t right, double value) {
        tree_[node] += value * static_cast<double>(right - left + 1);
        lazy_[node] += value;
    }

    void push(std::size_t node, std::size_t left, std::size_t right) {
        if (lazy_[node] == 0.0 || left == right) {
            return;
        }
        const std::size_t mid = (left + right) / 2;
        apply(node * 2, left, mid, lazy_[node]);
        apply(node * 2 + 1, mid + 1, right, lazy_[node]);
        lazy_[node] = 0.0;
    }

    void rangeAdd(std::size_t node, std::size_t left, std::size_t right, std::size_t ql, std::size_t qr, double value) {
        if (qr < left || right < ql) {
            return;
        }
        if (ql <= left && right <= qr) {
            apply(node, left, right, value);
            return;
        }
        push(node, left, right);
        const std::size_t mid = (left + right) / 2;
        rangeAdd(node * 2, left, mid, ql, qr, value);
        rangeAdd(node * 2 + 1, mid + 1, right, ql, qr, value);
        tree_[node] = tree_[node * 2] + tree_[node * 2 + 1];
    }

    double rangeSum(std::size_t node, std::size_t left, std::size_t right, std::size_t ql, std::size_t qr) {
        if (qr < left || right < ql) {
            return 0.0;
        }
        if (ql <= left && right <= qr) {
            return tree_[node];
        }
        push(node, left, right);
        const std::size_t mid = (left + right) / 2;
        return rangeSum(node * 2, left, mid, ql, qr) + rangeSum(node * 2 + 1, mid + 1, right, ql, qr);
    }
};

class FenwickTree {
public:
    explicit FenwickTree(std::size_t size) : tree_(size + 1, 0.0) {}

    void add(std::size_t index, double value) {
        for (++index; index < tree_.size(); index += index & -index) {
            tree_[index] += value;
        }
    }

    double sumPrefix(std::size_t index) const {
        double result = 0.0;
        for (++index; index > 0; index -= index & -index) {
            result += tree_[index];
        }
        return result;
    }

    double rangeSum(std::size_t left, std::size_t right) const {
        if (left > right) {
            return 0.0;
        }
        return sumPrefix(right) - (left == 0 ? 0.0 : sumPrefix(left - 1));
    }

    std::size_t memoryBytes() const {
        return tree_.capacity() * sizeof(double);
    }

private:
    std::vector<double> tree_;
};

struct AvlNode {
    int key = 0;
    double value = 0.0;
    double subtreeSum = 0.0;
    int height = 1;
    std::unique_ptr<AvlNode> left;
    std::unique_ptr<AvlNode> right;

    AvlNode(int keyValue, double storedValue)
        : key(keyValue), value(storedValue), subtreeSum(storedValue) {}
};

int height(const std::unique_ptr<AvlNode>& node) {
    return node ? node->height : 0;
}

double nodeSum(const std::unique_ptr<AvlNode>& node) {
    return node ? node->subtreeSum : 0.0;
}

void update(AvlNode& node) {
    node.height = 1 + std::max(height(node.left), height(node.right));
    node.subtreeSum = node.value + nodeSum(node.left) + nodeSum(node.right);
}

std::unique_ptr<AvlNode> rotateRight(std::unique_ptr<AvlNode> node) {
    std::unique_ptr<AvlNode> newRoot = std::move(node->left);
    node->left = std::move(newRoot->right);
    update(*node);
    newRoot->right = std::move(node);
    update(*newRoot);
    return newRoot;
}

std::unique_ptr<AvlNode> rotateLeft(std::unique_ptr<AvlNode> node) {
    std::unique_ptr<AvlNode> newRoot = std::move(node->right);
    node->right = std::move(newRoot->left);
    update(*node);
    newRoot->left = std::move(node);
    update(*newRoot);
    return newRoot;
}

int balanceFactor(const std::unique_ptr<AvlNode>& node) {
    return node ? height(node->left) - height(node->right) : 0;
}

std::unique_ptr<AvlNode> balance(std::unique_ptr<AvlNode> node) {
    update(*node);
    if (balanceFactor(node) > 1) {
        if (balanceFactor(node->left) < 0) {
            node->left = rotateLeft(std::move(node->left));
        }
        return rotateRight(std::move(node));
    }
    if (balanceFactor(node) < -1) {
        if (balanceFactor(node->right) > 0) {
            node->right = rotateRight(std::move(node->right));
        }
        return rotateLeft(std::move(node));
    }
    return node;
}

class AvlTree {
public:
    void insertOrAssign(int key, double value) {
        root_ = insertOrAssign(std::move(root_), key, value);
    }

    double rangeSum(int left, int right) const {
        return rangeSum(root_, left, right);
    }

    std::size_t memoryBytes() const {
        return nodeCount_ * sizeof(AvlNode);
    }

private:
    std::unique_ptr<AvlNode> root_;
    std::size_t nodeCount_ = 0;

    std::unique_ptr<AvlNode> insertOrAssign(std::unique_ptr<AvlNode> node, int key, double value) {
        if (!node) {
            ++nodeCount_;
            return std::make_unique<AvlNode>(key, value);
        }
        if (key < node->key) {
            node->left = insertOrAssign(std::move(node->left), key, value);
        } else if (key > node->key) {
            node->right = insertOrAssign(std::move(node->right), key, value);
        } else {
            node->value = value;
        }
        return balance(std::move(node));
    }

    double rangeSum(const std::unique_ptr<AvlNode>& node, int left, int right) const {
        if (!node) {
            return 0.0;
        }
        if (node->key < left) {
            return rangeSum(node->right, left, right);
        }
        if (node->key > right) {
            return rangeSum(node->left, left, right);
        }
        return node->value + rangeSum(node->left, left, right) + rangeSum(node->right, left, right);
    }
};

std::vector<double> listingPrices(const std::vector<Listing>& listings) {
    std::vector<double> prices;
    prices.reserve(listings.size());
    for (const Listing& listing : listings) {
        prices.push_back(listing.price);
    }
    std::sort(prices.begin(), prices.end());
    return prices;
}

std::pair<std::size_t, std::size_t> priceBounds(const std::vector<double>& prices, double minPrice, double maxPrice) {
    auto left = std::lower_bound(prices.begin(), prices.end(), minPrice);
    auto right = std::upper_bound(prices.begin(), prices.end(), maxPrice);
    if (left == prices.end() || left >= right) {
        return {1, 0};
    }
    return {
        static_cast<std::size_t>(left - prices.begin()),
        static_cast<std::size_t>((right - prices.begin()) - 1)
    };
}
}

std::vector<RangeMetric> RangeAnalytics::comparePriceRange(
    const std::vector<Listing>& listings,
    double minPrice,
    double maxPrice
) {
    std::vector<RangeMetric> metrics;
    const std::vector<double> prices = listingPrices(listings);
    if (prices.empty()) {
        return metrics;
    }
    const auto bounds = priceBounds(prices, minPrice, maxPrice);

    std::vector<double> counts(prices.size(), 1.0);
    const auto segmentStart = Clock::now();
    SegmentTree segmentTree(counts);
    std::size_t segmentMatches = bounds.first > bounds.second
        ? 0
        : static_cast<std::size_t>(segmentTree.query(bounds.first, bounds.second));
    double segmentMs = elapsedMs(segmentStart);
    metrics.push_back({"Segment Tree", segmentMatches, segmentMs, segmentMs > 0 ? 1000.0 / segmentMs : 0.0, segmentTree.memoryBytes()});

    const auto fenwickStart = Clock::now();
    FenwickTree fenwick(prices.size());
    for (std::size_t i = 0; i < prices.size(); ++i) {
        fenwick.add(i, 1.0);
    }
    std::size_t fenwickMatches = bounds.first > bounds.second
        ? 0
        : static_cast<std::size_t>(fenwick.rangeSum(bounds.first, bounds.second));
    double fenwickMs = elapsedMs(fenwickStart);
    metrics.push_back({"Fenwick Tree", fenwickMatches, fenwickMs, fenwickMs > 0 ? 1000.0 / fenwickMs : 0.0, fenwick.memoryBytes()});

    const auto binaryStart = Clock::now();
    auto left = std::lower_bound(prices.begin(), prices.end(), minPrice);
    auto right = std::upper_bound(prices.begin(), prices.end(), maxPrice);
    std::size_t binaryMatches = static_cast<std::size_t>(right - left);
    double binaryMs = elapsedMs(binaryStart);
    metrics.push_back({"Busqueda binaria", binaryMatches, binaryMs, binaryMs > 0 ? 1000.0 / binaryMs : 0.0, prices.capacity() * sizeof(double)});

    return metrics;
}

CalendarSimulationResult RangeAnalytics::simulateCalendarUpdates(const std::vector<CalendarEntry>& entries) {
    CalendarSimulationResult result;
    result.entries = entries.size();
    if (entries.empty()) {
        return result;
    }

    std::vector<double> prices;
    prices.reserve(entries.size());
    for (const CalendarEntry& entry : entries) {
        prices.push_back(entry.price);
    }

    const std::size_t left = 0;
    const std::size_t right = prices.size() - 1;

    const auto segmentStart = Clock::now();
    LazySegmentTree segmentTree(prices);
    segmentTree.rangeAdd(left, left, 2.0);
    result.segmentTreeSum = segmentTree.rangeSum(left, right);
    result.segmentTreeLazyMs = elapsedMs(segmentStart);

    const auto fenwickStart = Clock::now();
    FenwickTree fenwick(prices.size());
    for (std::size_t i = 0; i < prices.size(); ++i) {
        fenwick.add(i, prices[i]);
    }
    fenwick.add(0, 2.0);
    result.fenwickSum = fenwick.rangeSum(left, right);
    result.fenwickPointUpdateMs = elapsedMs(fenwickStart);

    const auto avlStart = Clock::now();
    AvlTree avl;
    for (std::size_t i = 0; i < prices.size(); ++i) {
        avl.insertOrAssign(static_cast<int>(i), prices[i]);
    }
    avl.insertOrAssign(0, prices[0] + 2.0);
    result.avlSum = avl.rangeSum(static_cast<int>(left), static_cast<int>(right));
    result.avlUpdateQueryMs = elapsedMs(avlStart);

    result.memoryBytes = segmentTree.memoryBytes() + fenwick.memoryBytes() + avl.memoryBytes();
    return result;
}
