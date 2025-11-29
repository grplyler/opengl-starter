# OpenGL Starter - OpenGL Rendering Engine

## Project Overview
`opengl-starter` is a lightweight OpenGL 3.3 starter engine built for learning and experimentation with 3D graphics. It follows patterns from learnopengl.com and emphasizes immediate-mode rendering with modern OpenGL practices.

## Architecture

### Core Components
- **Header-only classes** in `src/`: `shader.h`, `camera.h`, `mesh.h`, `light.h` contain both declarations and implementations
- **Shader system**: Convention-based loader - pass base name (e.g., `"multiple_lights"`) to load `assets/shaders/multiple_lights.vs` and `.fs`
- **Mesh system**: `RenderMesh` struct with procedural generators (`cube()`, `uvsphere()`, `plane()`, `cylinder()`) and GPU upload methods
- **Camera**: First-person fly camera with WASD + mouse look, controlled via `enableFlyCam` global

### Rendering Pipeline
1. GLFW window creation with OpenGL 3.3 core profile
2. GLAD loader initialization
3. ImGui setup for debug UI
4. Shader loading from `assets/shaders/` (name-based convention)
5. Mesh generation → `upload()` to GPU → `draw()` in render loop

### Key Patterns
- **Matrix transforms**: Model-View-Projection set via `Shader::setMat4()` - shader automatically activates before setting uniforms
- **Lighting**: Uses Blinn-Phong with multiple point lights array - see `multiple_lights.fs` for uniform structure
- **Debug visualization**: Meshes support `draw_normals()` and `draw_wireframe()` with separate VAO/VBOs created via `create_debug_*()` methods

## Build & Run Workflow

### Initial Setup
```bash
./bootstrap.sh  # Initialize git submodules (glfw, glm, imgui, ImGuizmo, stb)
```

### Development Scripts
- **`./b`** - Configure CMake + build (equivalent to `cmake -B build && cmake --build build`)
- **`./r`** - Run main executable (`./build/opengl-starter`)
- **`./t`** - Run test executable (`./build/test`)
- **`./br`** - Build and run in one command
- **`./bn`** - Configure CMake + build with Ninja (`cmake -B build -G Ninja && cmake --build build`)
- **`./rn`** - Run main executable after Ninja build

### Build System
- CMake-based with static library compilation for vendor deps
- Two executables: `opengl-starter` (main.cpp) and `test` (test.cpp)
- Platform-specific OpenGL linking (macOS uses frameworks, Linux uses X11)
- Shared include directories defined in `SHARED_INCLUDE_DIRS` CMake variable

## Code Conventions

### File Organization
- New headers go in `src/` with `.h` extension (header-only pattern)
- Shaders use paired `.vs`/`.fs` files in `assets/shaders/`
- No separate `.cpp` implementations for core classes (inline in headers)

### Shader Usage
```cpp
Shader shader("shader_name");  // Loads shader_name.vs and shader_name.fs
shader.use();                  // Automatically called by setter methods
shader.setMat4("model", modelMatrix);
shader.setVec3("lightPos", lightPosition);
```

### Mesh Creation & Rendering
```cpp
RenderMesh mesh = RenderMesh::uvsphere(10, 10);  // rings, sectors
mesh.compute_vertex_normals();  // Optional: calculate smooth normals
mesh.upload();                  // Upload to GPU once
// In render loop:
mesh.draw();                    // Binds VAO and draws elements
```

### Camera Integration
- Global `Camera camera` object with position, front, up vectors
- Mouse callback connected via `glfwSetCursorPosCallback(window, mouse_callback)`
- Update view matrix: `glm::mat4 view = camera.GetViewMatrix()`
- Toggle fly cam with `enableFlyCam` global boolean

## Vendor Dependencies
All included as git submodules in `vendor/`:
- **GLFW 3.x**: Windowing and input
- **GLAD**: OpenGL function loader (3.3 core)
- **GLM**: Math library (vec3, mat4, transformations)
- **Dear ImGui**: Immediate-mode GUI with GLFW+OpenGL3 backends
- **ImGuizmo**: 3D gizmo manipulation (translate/rotate/scale)
- **stb_image.h**: Image loading (single header)

## Common Tasks

### Adding a New Shader
1. Create `assets/shaders/myshader.vs` and `myshader.fs`
2. Use `Shader myShader("myshader");` in code
3. Set uniforms with `setMat4()`, `setVec3()`, `setFloat()`, etc.

### Adding a New Mesh Type
1. Add static method to `RenderMesh` in `mesh.h` (see `cube()` as example)
2. Use `add_vertex()` and `add_face()` to build geometry
3. Return constructed `RenderMesh` by value

### Debug Visualization
```cpp
mesh.create_debug_normals(0.1f);   // Create normal lines with length
mesh.create_debug_wireframe();     // Create wireframe lines
// In render loop with debug shader active:
mesh.draw_normals();
mesh.draw_wireframe();
```

## Known Issues & Gotchas
- Cursor capture enabled by default (`GLFW_CURSOR_DISABLED`) - may need to toggle for ImGui interaction
- Shader paths relative to executable location - run from project root
- Mac requires `GLFW_OPENGL_FORWARD_COMPAT` hint for OpenGL 3.3+
- ImGui and ImGuizmo require backend initialization before use
- Test executable (`test`) exports meshes to .obj files in working directory
