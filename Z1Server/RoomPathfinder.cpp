#include "pch.h"
#include "RoomPathfinder.h"
#include <queue>

namespace
{
    constexpr std::array<TileCoordinate, 4> Directions
    {
        TileCoordinate{0, -1},
        TileCoordinate{1, 0},
        TileCoordinate{0, 1},
        TileCoordinate{-1, 0}
    };

    struct Node
    {
        std::int32_t index = 0;
        std::int32_t g = 0;
        std::int32_t h = 0;
        std::int32_t f = g + h;

        Node(std::int32_t index, std::int32_t g, std::int32_t h) 
            : index(index), g(g), h(h), f(g + h)
        {

        }

        bool operator<(const Node& other) const noexcept
        {
            if (f != other.f)
            {
                return f < other.f;
            }

            if (h != other.h)
            {
                return h < other.h;
            }

            return index < other.index;
        }

        bool operator>(const Node& other) const noexcept
        {
            if (f != other.f)
            {
                return f > other.f;
            }

            if (h != other.h)
            {
                return h > other.h;
            }

            return index > other.index;
        }

        bool operator==(const Node& other) const noexcept
        {
            return !(*this < other && *this > other);
        }
    };

    std::int32_t ManhattanDistance(TileCoordinate from, TileCoordinate to)
    {
        return std::abs(from.x - to.x) + std::abs(from.y - to.y);
    }
}

std::vector<TileCoordinate> RoomPathfinder::FindPath(const RoomNavigationGrid& grid, TileCoordinate start, TileCoordinate goal) const
{
    if (!grid.IsWalkable(start) || !grid.IsWalkable(goal))
    {
        return{};
    }

    const std::int32_t startIndex = ToIndex(start);
    const std::int32_t goalIndex = ToIndex(goal);

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq; // MinHeap
    std::vector<std::int32_t> gCosts(16 * 11, INT32_MAX);
    std::vector<std::int32_t> parents(16 * 11, -1);

    pq.emplace(startIndex, 0, ManhattanDistance(start, goal));
    gCosts[startIndex] = 0;
    parents[startIndex] = startIndex;   // 자기 자신

    while (!pq.empty())
    {
        Node current = pq.top();
        pq.pop();
        
        // 더 비싼 노드는 탐색하지 말자
        if (current.g > gCosts[current.index])
        {
            continue;
        }

        // 목표 지점
        if (current.index == goalIndex)
        {
            std::vector<TileCoordinate> path;
            std::int32_t index = goalIndex;

            while (index != startIndex)
            {
                path.emplace_back(ToTile(index));
                index = parents[index];
            }

            std::reverse(path.begin(), path.end());
            return path;
        }

        const TileCoordinate currentTile = ToTile(current.index);
        for (const TileCoordinate& direction : Directions)
        {
            const TileCoordinate nextTile = currentTile + direction;
            if (!grid.IsWalkable(nextTile))
            {
                continue;
            }

            std::int32_t nextIndex = ToIndex(nextTile);
            std::int32_t newG = current.g + 1; // 1칸 이동이라 
            if (newG >= gCosts[nextIndex])  // 새로운 비용이 기존 비용보다 비싸면 skip
            {
                continue;
            }

            gCosts[nextIndex] = newG;
            parents[nextIndex] = current.index;
            pq.emplace(nextIndex, newG, ManhattanDistance(nextTile, goal));
        }
    }

    return {};
}

std::int32_t RoomPathfinder::ToIndex(TileCoordinate tile) noexcept
{
    return tile.y * RoomNavigationGrid::Width + tile.x;
}

TileCoordinate RoomPathfinder::ToTile(std::int32_t index) noexcept
{
    // (7, 3) -> 3 x 16 + 7 = 55 -> (55 % 16 = 7, 55 / 16 = 3)
    return { index % RoomNavigationGrid::Width, index / RoomNavigationGrid::Width };
}
