# ProjectUmeowmi

A cooking simulation game developed with Unreal Engine 5.5, featuring an isometric perspective, interactive dialogue system, and comprehensive dish customization mechanics.

## 🎮 Game Overview

ProjectUmeowmi is a restaurant simulation game where players can interact with characters, customize dishes with various ingredients and preparations, and experience a rich dialogue-driven narrative. The game features an isometric camera system that provides strategic gameplay perspectives.

## ✨ Key Features

### 🎥 Camera System
- **Isometric Perspective**: 4-position rotatable camera system
- **Smooth Transitions**: Configurable camera movement and rotation speeds
- **Zoom Functionality**: Mouse wheel and controller stick zoom controls
- **Orthographic Rendering**: Optimized for isometric gameplay

### 🚶 Movement System
- **Grid-based Movement**: Optional precise grid movement system
- **Smooth Transitions**: Configurable movement speeds and grid sizes
- **Character Control**: Enhanced input system for responsive controls

### 💬 Dialogue & Interaction
- **DlgSystem Integration**: Advanced dialogue system with branching conversations
- **NPC Interactions**: Multiple character types (NPC, Prop, System)
- **Interactive Objects**: Sphere-based interaction detection
- **Dialogue Widgets**: Custom UI for conversation management

### 🍳 Dish Customization
- **Ingredient System**: Comprehensive ingredient management with meshes and properties
- **Preparation Methods**: Multiple cooking techniques and preparations
- **Order Management**: Order tracking and fulfillment system
- **Customization UI**: Dedicated interface for dish creation

### 👥 Characters
- **Multiple Characters**: Lola, Yeoh, Bao, and test characters
- **Character Interactions**: Dialogue participation and relationship systems
- **Customizable Avatars**: Character customization and progression

### 🎨 UI/UX
- **Radar Charts**: Visual representation of dish attributes
- **Dialogue Boxes**: Custom conversation interface
- **Common UI Framework**: Consistent user experience across platforms
- **Interaction Widgets**: Visual feedback for interactive elements

## 🛠️ Technical Stack

### Engine & Version
- **Unreal Engine**: 5.5
- **Platforms**: Windows, macOS, iOS, tvOS, VisionOS

### Key Plugins
- **DlgSystem**: Advanced dialogue system
- **CommonUI**: Cross-platform UI framework
- **RadarChart**: Data visualization component
- **ModelingToolsEditorMode**: Enhanced editor tools

### Core Modules
- **Enhanced Input**: Modern input handling
- **UMG**: User interface framework
- **GameplayTags**: Tag-based gameplay systems
- **Slate**: Low-level UI framework

## 📁 Project Structure

```
ProjectUmeowmi/
├── Source/
│   └── ProjectUmeowmi/
│       ├── UI/                    # User interface components
│       ├── Dialogue/              # Dialogue and interaction systems
│       ├── DishCustomization/     # Cooking and dish management
│       ├── Interactables/         # Interactive object system
│       └── Interfaces/            # System interfaces
├── Content/
│   ├── Characters/               # Character assets and blueprints
│   ├── Maps/                     # Game levels and environments
│   ├── Systems/                  # Game system blueprints
│   ├── UI/                       # User interface assets
│   └── Dialogue/                 # Dialogue content and scripts
└── Config/                       # Engine configuration files
```

## 🚀 Getting Started

### Prerequisites
- Unreal Engine 5.5
- Visual Studio 2022 (for C++ development)
- Xcode (for iOS/macOS development)

### Installation
1. Clone the repository
2. Open `ProjectUmeowmi.uproject` in Unreal Engine
3. Allow the engine to rebuild the project
4. Install required plugins from the Epic Marketplace:
   - DlgSystem
   - RadarChart

### Building
- **Windows**: Open `ProjectUmeowmi.sln` in Visual Studio
- **macOS**: Open `ProjectUmeowmi (Mac).xcworkspace` in Xcode
- **iOS**: Open `ProjectUmeowmi (IOS).xcworkspace` in Xcode
- **VisionOS**: Open `ProjectUmeowmi (VisionOS).xcworkspace` in Xcode

## 🎯 Gameplay Mechanics

### Character Movement
- Use WASD or arrow keys for movement
- Toggle grid movement for precise positioning
- Rotate camera with Q/E or right stick
- Zoom with mouse wheel or controller triggers

### Interactions
- Approach interactive objects to see interaction prompts
- Press the interact key to start conversations or interactions
- Navigate dialogue options using arrow keys or D-pad

### Dish Customization
- Access cooking stations to begin dish creation
- Select ingredients from available options
- Choose preparation methods for each ingredient
- Complete orders to progress through the game

## 🔧 Development

### Code Architecture
The project follows Unreal Engine best practices with:
- **Component-based design** for modular functionality
- **Interface-driven interactions** for flexible object communication
- **Blueprint/C++ hybrid approach** for rapid prototyping and performance
- **Event-driven systems** for loose coupling between components

### Key Classes
- `AProjectUmeowmiCharacter`: Main player character with camera and movement
- `ATalkingObject`: Base class for interactive dialogue objects
- `UPUDishCustomizationComponent`: Core dish creation system
- `UPUDialogueBox`: Custom dialogue interface widget

## 📝 License

This project is developed with Unreal Engine 5.5. Please refer to Epic Games' licensing terms for Unreal Engine usage.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For questions or issues related to this project, please open an issue in the repository or contact the development team.

---

*Developed with ❤️ using Unreal Engine 5.5*
