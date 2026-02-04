# Project Goals

## Vision

Digital Workshop is a cross-platform desktop application for managing 3D models with integrated G-code analysis and 2D cut optimization.

## Primary Objectives

1. **Cross-Platform First** - Windows, Linux, macOS from a single codebase
2. **Performance** - Handle 600MB+ models with streaming and efficient memory use
3. **Simplicity** - Clean code, minimal dependencies, easy to maintain
4. **Focused Features** - Do fewer things well rather than many things poorly

## Core Features

### 3D Model Management
- Import STL, OBJ, 3MF files
- Content-based deduplication
- Automatic thumbnail generation
- Metadata extraction and editing
- Library organization

### G-Code Analysis
- Parse G-code files
- Calculate timing estimates
- Visual path preview
- Tool change tracking

### 2D Cut Optimization
- Separate feature from 3D viewing
- Panel/sheet layout optimization
- Material waste minimization
- Cut list generation

### Project Organization
- Project-based workflows
- Cost estimation
- Basic customer/quote tracking

## Non-Goals

- STEP/IGES CAD format support
- AI/ML features
- Cloud-based operation
- CAD editing (view/import only)
- Scientific precision analysis
- Mobile platforms

## UI Philosophy

**Object-Centric Workspace**

Everything revolves around the current object or focus:
- Central viewport shows the active model/preview
- Surrounding panels provide context and actions
- Tools act on the focused object
- Panels can be docked, floated, hidden as needed
