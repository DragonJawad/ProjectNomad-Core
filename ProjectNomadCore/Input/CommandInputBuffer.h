#pragma once

#include <array>
#include <CRCpp/CRC.h>

#include "BufferedInputData.h"
#include "InputCommand.h"
#include "CharacterInput.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    /**
    * Represents gameplay input buffer for a single entity
    **/
    class CommandInputBuffer {
      public:
        void UpdateCommands(FrameType curFrame, const CommandSetList& curFrameCommands) {
            ClearConsumedOrExpiredInputs(curFrame - 1); // Note that this is safe to do at frame 0 with current implementation
            
            AddNewCommandsToInputBuffer(curFrame, mRawCommandInputs, curFrameCommands);
            mRawCommandInputs = curFrameCommands;
        }

        bool IsCommandInitiallyPressedWithConsumption(InputCommand command) {
            return mBufferedInputs[static_cast<size_t>(command)].GetAndConsumeInput();
        }

        bool IsCommandInitiallyPressedWithoutConsumption(InputCommand command) const {
            return mBufferedInputs[static_cast<size_t>(command)].GetWithoutConsumingInput();
        }

        void ConsumeInputInitialPress(InputCommand command) {
            mBufferedInputs[static_cast<size_t>(command)].MarkConsumed();
        }

        void ImmediatelyResetInputPress(InputCommand command) {
            mBufferedInputs[static_cast<size_t>(command)].ImmediatelyResetInputPress();
        }

        bool IsCommandHeld(InputCommand command) const {
            // "Held" originates from "Is button held?", albeit doesn't exactly make sense with commands.
            // However, logic may want to check if command is still being actively given (eg, hold jump to go higher).
            // Thus, just return raw input value rather than going through input buffer.
            return mRawCommandInputs.IsCommandSet(command);
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            mRawCommandInputs.CalculateCRC32(resultThusFar);
            
            for (auto& bufferedInput : mBufferedInputs) {
                bufferedInput.CalculateCRC32(resultThusFar);
            }
        }

      private:
        void AddNewCommandsToInputBuffer(FrameType curFrame,
                                          const CommandSetList& prevFrameCommands,
                                          const CommandSetList& newFrameCommands) {
            // For each command...
            for (size_t i = 0; i < mBufferedInputs.size(); i++) {
                // If command was *just* pressed, then add to input buffer
                // ...bit hesitant on this design choice atm, but eh it feels "right" for now based on - say - Smash's input buffer
                if (newFrameCommands.commandInputs.GetIndex(i) && !prevFrameCommands.commandInputs.GetIndex(i)) {
                    mBufferedInputs[i].RememberInputSet(curFrame);
                }
            }
        }

        bool GetAndConsumeInput(InputCommand command) {
            return mBufferedInputs[static_cast<size_t>(command)].GetAndConsumeInput();
        }

        void ClearConsumedOrExpiredInputs(FrameType latestCompletedFrame) {
            for (auto& bufferedInput : mBufferedInputs) {
                bufferedInput.ClearIfConsumedOrExpired(latestCompletedFrame);
            }
        }
        
        CommandSetList mRawCommandInputs = {};
        std::array<BufferedInputData, static_cast<size_t>(InputCommand::ENUM_COUNT)> mBufferedInputs = {};
    };
}
