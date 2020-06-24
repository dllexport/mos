#include "buddy_system.h"
#include "lib/debug.h"
#include "lib/printk.h"
uint64_t round_up_pow_of_2(uint64_t x) { return x == 1 ? 1 : 1 << (64 - __builtin_clzl(x - 1)); }
#define LEFT_LEAF(index) ((index)*2 + 1)
#define RIGHT_LEAF(index) ((index)*2 + 2)
#define PARENT(index) (((index) + 1) / 2 - 1)

#define IS_POWER_OF_2(x) (!((x) & ((x)-1)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include "memory.h"

uint64_t BuddySystem::PageSize()
{
    return this->total_pages_count * sizeof(Page);
}

void BuddySystem::Construct(uint64_t pages_count)
{
    assert(pages_count >= 1, "pages_count < 1");

    unsigned node_size;
    int i;

    if (!IS_POWER_OF_2(pages_count))
    {
        pages_count = round_up_pow_of_2(pages_count);
        printk("buddy page round %d\n", pages_count);
    }
    this->total_pages_count = pages_count;

    // node number is size * 2 - 1, we alloc size * 2
    node_size = pages_count * 2;

    this->free_pages_count = pages_count;

    // for every node
    for (i = 0; i < 2 * pages_count - 1; ++i)
    {
        if (IS_POWER_OF_2(i + 1))
            node_size /= 2;
        this->nodes[i] = node_size;
    }
}

int64_t BuddySystem::AllocPages(uint64_t pages_count)
{
    assert(pages_count >= 1, "pages_count < 1");

    unsigned node_size;
    unsigned offset = 0;

    if (!IS_POWER_OF_2(pages_count))
        pages_count = round_up_pow_of_2(pages_count);

    unsigned index = 0;
    // if the first(largest) doesn't fit return error
    if (this->nodes[index] < pages_count)
        return -1;

    for (node_size = this->free_pages_count; node_size != pages_count; node_size /= 2)
    {
        if (this->nodes[LEFT_LEAF(index)] >= pages_count)
            index = LEFT_LEAF(index);
        else
            index = RIGHT_LEAF(index);
    }

    offset = (index + 1) * node_size - this->free_pages_count;
    if (offset > this->total_pages_count)
    {
        return -1;
    }

    // 标记为已用
    this->nodes[index] = 0;

    while (index)
    {
        index = PARENT(index);
        this->nodes[index] =
            MAX(this->nodes[LEFT_LEAF(index)], this->nodes[RIGHT_LEAF(index)]);
    }

    return offset;
}
void BuddySystem::BuddySystem::FreePages(uint64_t page_index)
{
}
