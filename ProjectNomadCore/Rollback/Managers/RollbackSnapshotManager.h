#pragma once

#include "Rollback/Model/RollbackSettings.h"
#include "Utilities/FrameType.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    // Note that SnapshotType should NOT have any pointers as snapshot restoration will be ineffective.
    // This includes std::array and callbacks.

    /// <summary>
    /// Encapsulates snapshot data and related behavior specific to rollbacks.
    /// ie this does not define what a snapshot is nor when to take one, but rather is responsible for:
    /// - Storing snapshots for rollback usage (eg, one per frame and just enough data stored for all rollback needs)
    /// - Retrieving relevant snapshot for any possible rollback usage
    /// </summary>
    template <typename SnapshotType>
    class RollbackSnapshotManager {
      public:
        bool OnSessionStart() {
            mLatestStoredFrame = std::numeric_limits<FrameType>::max(); // Next frame to store is 0 (max + 1 = 0 with overflow)

            // No need to reset RingBuffer as we just expect existing data in the buffer to be "noise"

            return true;
        }
        
        /// <summary>Inserts provided snapshot into buffer</summary>
        /// <param name="targetFrame">Frame that snapshot is intended for</param>
        /// <param name="snapshot">
        /// Snapshot to insert passed by reference.
        /// Assume that this value will be unusable after the call (due to swap-insert).
        /// </param>
        void StoreSnapshot(FrameType targetFrame, SnapshotType& snapshot) {
            // Trying to add snapshot for next expected frame?
            if (targetFrame == mLatestStoredFrame + 1) {
                mSnapshotBuffer.SwapInsert(snapshot); 
                mLatestStoredFrame = targetFrame;
            }
            // Trying to replace a previously stored frame?
            else if (targetFrame <= mLatestStoredFrame && mLatestStoredFrame != std::numeric_limits<FrameType>::max()) {
                int offset = CalculateOffset(targetFrame);
                mSnapshotBuffer.SwapReplace(offset, snapshot);
            }
            else { // Invalid input!
                Singleton<LoggerSingleton>::get().LogErrorMessage(
                    "Unexpected currentFrame value! Latest stored frame: " + std::to_string(mLatestStoredFrame)
                        + ", input frame: " + std::to_string(targetFrame)
                );
            }
        }

        const SnapshotType& GetSnapshot(FrameType frameToRetrieveSnapshotFor) const {
            if (frameToRetrieveSnapshotFor > mLatestStoredFrame) {
                Singleton<LoggerSingleton>::get().LogErrorMessage(
                    "Provided retrieval frame greater than latest frame, input frame: " +
                    std::to_string(frameToRetrieveSnapshotFor)
                );
                return mSnapshotBuffer.Get(0);
            }

            int offset = CalculateOffset(frameToRetrieveSnapshotFor);
            return mSnapshotBuffer.Get(offset);
        }

        const SnapshotType& GetLatestFrameSnapshot() const {
            // Sanity check
            if (mLatestStoredFrame == std::numeric_limits<FrameType>::max()) {
                Singleton<LoggerSingleton>::get().LogWarnMessage(
                    "No stored frame data yet! Beware of undefined results"
                );
            }

            return mSnapshotBuffer.Get(0); // Element at head is always the latest frame stored
        }

      private:
        bool IsInsertingInitialFrame(FrameType frameToInsert) {
            return mLatestStoredFrame == std::numeric_limits<FrameType>::max() && frameToInsert +  1;
        }
        
        int CalculateOffset(FrameType frameForStoredSnapshot) const {
            FrameType frameOffset = mLatestStoredFrame - frameForStoredSnapshot;

            // Sanity check to help catch bugs
            if (frameOffset > RollbackStaticSettings::kOneMoreThanMaxRollbackFrames) {
                Singleton<LoggerSingleton>::get().LogErrorMessage(
                    "Provided retrieval frame beyond buffer size (rollback window + 1 older frames), input frame: " +
                    std::to_string(frameForStoredSnapshot)
                );
                return 0;
            }

            // Negative values represent the past in RingBuffer so flip the sign
            return static_cast<int>(frameOffset) * -1;
        }
        
        FrameType mLatestStoredFrame = std::numeric_limits<FrameType>::max(); // Next frame to store is 0 (max + 1 = 0 with overflow)
        RingBuffer<SnapshotType, RollbackStaticSettings::kTwoMoreThanMaxRollbackFrames> mSnapshotBuffer = {}; // Store current frame, 10 frames in past, and 1 extra frame for verified frame processing
    };
}
