#pragma once

namespace ProjectNomad {
    /// <summary>
    /// Queue in place with fixed size. Intended to be alternative to std::queue for snapshot/memcpy behavior,
    ///     as std::queue's storage is effectively a pointer to elsewhere in memory.
    /// </summary>
    // TODO: This is a stack (first-in-last-out), NOT a queue (FIFO)! Fix this!
    template <typename ContentType, uint32_t MaxSize>
    class InPlaceQueue {
        static_assert(MaxSize > 0, "MaxSize must be greater than 0");

        ContentType array[MaxSize];
        uint32_t headIndex = 0; // Points to where next element should be added

    public:
        InPlaceQueue() {}
        
        static constexpr uint32_t getMaxSize() {
            return MaxSize;
        }

        uint32_t getSize() {
            return headIndex;
        }
        
        /// <returns>Returns true if succeeds, false otherwise</returns>
        bool push(const ContentType& element) {
            if (headIndex >= MaxSize) {
                return false;
            }
            
            array[headIndex] = element;
            headIndex++;
            return true;
        }

        const ContentType& front() {
            return array[headIndex - 1]; // May be out of range if no elements but leaving that as a concern for user
        }

        /// <returns>Returns true if succeeds, false otherwise</returns>
        bool pop() {
            if (headIndex == 0) {
                return false;
            }

            headIndex--;
            return true;
        }

        void clear() {
            headIndex = 0;
        }

        bool isEmpty() {
            return headIndex == 0;
        }
    };
}
