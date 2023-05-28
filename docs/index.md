# Documentation 
The individual topics are sorted by occurrence in the development process. Read the docs carefully to be able to use the persistence framework to its full potential.

### 1. [Game mode setup](setup.md)
The basic setup needed to start integrating the persistence framework into a game mode.

### 2. [Persistence Manager](persistence-manager.md)
What does the central persistence manager do and what useful methods does it provide?

### 3. [Persistence component setup](persistence-component.md)
Component setup instructions to make entities persistent and connect them to the manager.

### 4. [World entity save-data](entity-savedata.md)
Setup instructions how to persistst entity information.

### 5. [Custom component save-data](custom-component.md)
Adding persistence support for custom components.

### 6. [Baked map entities](baked-entities.md)
What to know about saving states for baked map objects.

### 7. [Scripted states](scripted-states.md)
Saving scripted states (aka custom class instances unrelated to IEntity).

### 8. [Utilities](utilities.md)
How to use the loader utilities to manually spawn back entities and scripted states selectively and access the persistence db repositories.

### 9. [Versioning](versioning.md)
How to utilize versioning for save-data layout changes.

### 10. [Persistence ID](id-generation.md)
Details how ids are generated and what guarantees they provide.

### 11. [Respawn system integration](respawn-system-integration.md)
Tips and considerations for the integration into a custom respawn system.

### 12. [Debugging and metrics](debugging.md)
Optional debug steps and peformance metrics.
