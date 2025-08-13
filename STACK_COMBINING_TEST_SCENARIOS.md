# Stack Combining Test Scenarios

## Overview
This document outlines test scenarios for the enhanced stack combining functionality in your inventory system.

## Test Scenarios

### Scenario 1: Full Stack Consumption (Same Grid)
**Setup**: Have a stack of 10 items in slot A and a stack of 5 identical items being dragged (hover item)
**Action**: Click on the stack in slot A to combine
**Expected Result**: Slot A should now have 15 items, and the hover item should disappear completely

### Scenario 2: Partial Stack Transfer (Same Grid)
**Setup**: Have a stack of 8 items in slot A (max stack size 10) and a stack of 5 identical items being dragged
**Action**: Click on the stack in slot A
**Expected Result**: Slot A should have 10 items (max), and the hover item should have 3 remaining items

### Scenario 3: Cross-Grid Full Transfer
**Setup**: Have a stack of 5 items in Grid A and drag it to Grid B where there's a stack of 3 identical items
**Action**: Click on the stack in Grid B
**Expected Result**: Grid B should have 8 items, Grid A should be empty (original stack removed), hover item disappears

### Scenario 4: Cross-Grid Partial Transfer
**Setup**: Have a stack of 10 items in Grid A and drag it to Grid B where there's a stack of 7 identical items (max stack 10)
**Action**: Click on the stack in Grid B
**Expected Result**: Grid B should have 10 items (max), hover item should have 7 remaining items, Grid A still has original stack

### Scenario 5: Split Operation Stacking
**Setup**: Split a stack of 10 items (taking 4 items), then try to combine with another stack
**Action**: Use the split operation, then combine the split items with another stack
**Expected Result**: Original stack should remain with 6 items, target stack should increase by the split amount

### Scenario 6: Stack Swapping
**Setup**: Have a partially full stack (3 items, max 10) and a smaller hover stack (2 items)
**Action**: Click to combine when target has no room and hover stack is smaller than max
**Expected Result**: Stacks should swap - target gets 2 items, hover gets 3 items

## Key Features Implemented

1. **Automatic Stack Removal**: When a stack is completely transferred, the original stack is automatically removed
2. **Cross-Grid Support**: Stacking works between different inventory grids
3. **Split Operation Handling**: Properly handles items created from split operations
4. **Safety Checks**: Validates all parameters before performing operations
5. **Visual Feedback**: Stack counts update immediately and hover items disappear when depleted

## Testing Notes

- Test with different item types to ensure only identical stackable items combine
- Verify that non-stackable items don't interfere with the stacking logic
- Check that UI updates correctly show new stack counts
- Ensure hover item cursor changes appropriately when stacks are depleted
- Test edge cases like maximum stack sizes and empty slots

## Code Changes Summary

The implementation enhances these key functions:
- `ConsumeHoverItemStacks()`: Now removes original stacks when fully consumed
- `FillInStack()`: Handles removal of depleted hover items
- `SwapStackCounts()`: Manages stack cleanup when swapped stacks become empty
- `HandleStackableItemInteraction()`: Added comprehensive safety checks
- All functions now properly handle cross-grid transfers and split operations