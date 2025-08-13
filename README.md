# Inventory Systems Plugin

A comprehensive spatial inventory system plugin for Unreal Engine 5 that provides grid-based inventory management, equipment systems, and item interactions.

**Developed with Unreal Engine 5**

## Overview

This plugin implements a robust inventory system with spatial grid-based item placement, equipment management, and a rich UI framework. It supports stackable items, cross-grid operations, equipment visualization, and comprehensive item management functionality.

## Core Features

### 🎒 Spatial Inventory System
- **Grid-based Layout**: Items occupy specific grid spaces with configurable dimensions
- **Multiple Inventory Grids**: Backpack, Satchel, Quiver, and Locked sections
- **Smart Item Placement**: Automatic placement validation and highlighting
- **Cross-Grid Operations**: Move items between different inventory sections
- **Stack Management**: Intelligent stacking system for stackable items

### ⚔️ Equipment Management
- **Equipment Slots**: Dedicated slots for different equipment types (weapons, armor, accessories)
- **Visual Equipment Display**: Real-time 3D visualization of equipped items
- **Equipment Categories**: Weapons, chest armor, helmets, gloves, boots, and more
- **Equip Actor System**: Spawns 3D representations of equipped items on characters

### 📦 Item System
- **Modular Item Framework**: Fragment-based item system for flexible item definitions
- **Item Categories**: Consumables, Equipment, Craftables with distinct behaviors
- **Item Highlighting**: Interactive highlighting system for world items
- **Rich Item Descriptions**: Detailed tooltips with stats, descriptions, and requirements

### 🎮 User Interface
- **Responsive Grid UI**: Dynamic grid slots with hover effects and visual feedback
- **Item Descriptions**: Rich tooltips showing item details, stats, and lore
- **Equipment Visualization**: Character display showing equipped items
- **Popup Interactions**: Context menus for item actions (consume, drop, equip)

## Plugin Structure

```
Plugins/Inventory/
├── Content/                          # Blueprint assets and UI widgets
│   ├── EquipmentManagement/         # Equipment-related blueprints
│   ├── InventoryManagement/         # Core inventory components
│   ├── Items/                       # Item blueprints and components
│   ├── Player/                      # Player controller extensions
│   └── Widgets/                     # UI widget blueprints
│       ├── HUD/                     # Main HUD elements
│       ├── Inventory/               # Inventory grid widgets
│       └── ItemDescription/         # Item tooltip widgets
├── Source/Inventory/                # C++ source code
│   ├── Public/                      # Header files
│   │   ├── EquipmentManagement/     # Equipment system headers
│   │   ├── InventoryManagement/     # Core inventory headers
│   │   ├── Items/                   # Item system headers
│   │   ├── Player/                  # Player integration headers
│   │   ├── Types/                   # Data structures and enums
│   │   └── Widgets/                 # UI framework headers
│   └── Private/                     # Implementation files
└── Inventory.uplugin                # Plugin descriptor
```

## Key Components

### Core Inventory
- **`UInv_InventoryComponent`**: Main inventory management component
- **`UInv_InventoryItem`**: Represents items in the inventory system
- **`UInv_ItemComponent`**: World item interaction component
- **`UInv_SpatialInventory`**: Grid-based spatial inventory widget

### Equipment System
- **`UInv_EquipmentComponent`**: Manages character equipment
- **`AInv_EquipActor`**: 3D representations of equipped items
- **`UInv_EquippedGridSlot`**: UI slots for equipment

### Item Framework
- **`FInv_ItemManifest`**: Core item data structure
- **`UInv_ItemFragment`**: Modular item behavior system
- **`UInv_FastArray`**: Optimized networked item arrays

## How to Use

### Setting Up the Plugin

1. **Enable the Plugin**: 
   - In your project, go to Edit → Plugins
   - Search for "Inventory" and enable it
   - Restart the editor when prompted

2. **Add Components to Your Character**:
   ```cpp
   // In your character's constructor
   InventoryComponent = CreateDefaultSubobject<UInv_InventoryComponent>(TEXT("InventoryComponent"));
   EquipmentComponent = CreateDefaultSubobject<UInv_EquipmentComponent>(TEXT("EquipmentComponent"));
   ```

3. **Configure Player Controller**:
   - Use `AInv_PlayerController` as your base player controller class
   - Or add inventory input bindings to your existing controller

### Creating Items

1. **Create Item Blueprint**:
   - Inherit from `BP_Inv_Base_Item`
   - Configure item properties (size, category, stackable, etc.)
   - Set up item fragments for specific behaviors

2. **Item Categories**:
   - **Consumables**: Items that can be used/consumed
   - **Equipment**: Items that can be equipped
   - **Craftables**: Materials for crafting systems

### Inventory Operations

```cpp
// Add item to inventory
InventoryComponent->TryAddItem(ItemComponent);

// Drop item from inventory
InventoryComponent->Server_DropItem(Item, StackCount);

// Equip item
InventoryComponent->Server_EquipSlotClicked(ItemToEquip, ItemToUnequip);
```

### Widget Setup

1. **Create Inventory UI**:
   - Inherit from `UInv_SpatialInventory` for your main inventory widget
   - Add inventory grids: Backpack, Satchel, Quiver, Locked
   - Configure equipment slots for different item types

2. **Item Interaction**:
   - Items automatically handle hover effects
   - Right-click for context menus
   - Drag and drop between grids and equipment slots

## How It Works

### Spatial Grid System
The inventory uses a coordinate-based grid system where items occupy rectangular spaces. Each item has dimensions (width × height) and the system validates placement to prevent overlapping.

### Item Stacking
Stackable items are automatically combined when added to inventory. The system tracks stack counts and handles splitting when partial amounts are moved or dropped.

### Equipment Visualization
When items are equipped, the `UInv_EquipmentComponent` spawns `AInv_EquipActor` instances that attach to the character's skeletal mesh at predefined sockets.

### Network Replication
The system is fully networked using Unreal's FastArray replication for efficient inventory state synchronization across clients.

### Fragment System
Items use a modular fragment system where different behaviors (equipment stats, consumable effects, etc.) are implemented as separate fragments that can be mixed and matched.

## Configuration

### Default Settings
- **Grid Tile Size**: Configurable tile dimensions
- **Drop Mechanics**: Angle and distance parameters for dropped items
- **UI Timers**: Hover delays for tooltips and descriptions
- **Equipment Sockets**: Character attachment points for equipment

### Blueprint Integration
Most functionality is exposed to Blueprints, allowing designers to:
- Create custom item types
- Configure inventory layouts
- Customize UI appearance
- Define equipment behaviors

## Dependencies

- **Unreal Engine 5**: Core framework
- **Enhanced Input**: For input handling
- **Gameplay Tags**: For item categorization and equipment types
- **UMG**: For user interface widgets

---

*This plugin provides a solid foundation for inventory systems in action RPGs, survival games, and other genres requiring complex item management.*
