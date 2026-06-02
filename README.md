# OBS Setup Wizard Plugin

Plugin para OBS Studio que guía al usuario en la configuración inicial mediante un asistente paso a paso con botones interactivos.

## ¿Qué hace?

Al abrir OBS por primera vez (o desde **Herramientas → Setup Wizard**), muestra un diálogo con preguntas:

1. **¿Para qué usas OBS?** — Streaming, Gameplay, Podcast, Tutoriales, Eventos, Clips
2. **¿Qué tan potente es tu PC?** — Básica / Media / Potente
3. **¿Qué resolución y FPS quieres?** — 720p30 / 1080p60 / 1440p60 / 4K30
4. **Resumen + botón "Aplicar"** — aplica encoder, bitrate y resolución automáticamente

---

## Requisitos

| Herramienta | Versión mínima |
|-------------|---------------|
| Visual Studio | 2022 (con Desktop C++) |
| CMake | 3.16+ |
| Qt | 6.x (o 5.15) |
| OBS Studio source | 30.x |
| Git | cualquiera |

---

## Compilar en Windows con Visual Studio 2022

### 1. Clonar OBS Studio

```bash
git clone --recursive https://github.com/obsproject/obs-studio.git
cd obs-studio
```

### 2. Compilar OBS como dependencia

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DENABLE_BROWSER=OFF -DENABLE_VST=OFF

cmake --build build --config Release --target obs-frontend-api libobs
```

### 3. Clonar este plugin DENTRO del árbol de OBS

```bash
# Desde la raíz de obs-studio:
git clone https://github.com/TU_USUARIO/obs-setup-wizard.git plugins/obs-setup-wizard
```

Luego agrega al final de `plugins/CMakeLists.txt` de OBS:

```cmake
add_subdirectory(obs-setup-wizard)
```

### 4. Reconfigurar y compilar

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target obs-setup-wizard
```

El `.dll` aparecerá en:
```
build/rundir/Release/obs-plugins/64bit/obs-setup-wizard.dll
```

### 5. Instalación manual (sin recompilar todo OBS)

Copia el `.dll` a:
```
C:\Program Files\obs-studio\obs-plugins\64bit\
```

---

## Compilar como proyecto independiente (recomendado para desarrollo)

```bash
git clone https://github.com/TU_USUARIO/obs-setup-wizard.git
cd obs-setup-wizard

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DOBS_SOURCE_DIR="C:/ruta/a/obs-studio" ^
  -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2022_64"

cmake --build build --config Release
```

---

## Publicar en GitHub desde Visual Studio

### Opción A — desde Visual Studio (GUI)

1. Abre la carpeta `obs-setup-wizard` en VS: `Archivo → Abrir → Carpeta`
2. Ve a `Git → Crear repositorio Git`
3. Marca **"Push to GitHub"**, inicia sesión con tu cuenta
4. Dale nombre al repo: `obs-setup-wizard`
5. Haz commit inicial y **"Push"**

### Opción B — desde terminal (más rápido)

```bash
cd obs-setup-wizard

git init
git add .
git commit -m "feat: initial OBS Setup Wizard plugin"

# Crear el repo en GitHub (necesitas GitHub CLI)
gh repo create obs-setup-wizard --public --source=. --remote=origin --push

# O manualmente:
git remote add origin https://github.com/TU_USUARIO/obs-setup-wizard.git
git branch -M main
git push -u origin main
```

---

## Estructura del proyecto

```
obs-setup-wizard/
├── src/
│   ├── obs-setup-wizard.h      ← declaraciones del plugin y diálogo Qt
│   └── obs-setup-wizard.cpp    ← implementación completa
├── data/
│   └── locale/
│       └── en-US.ini           ← textos localizables
├── cmake/                      ← módulos CMake adicionales (opcional)
├── CMakeLists.txt              ← build script
├── .gitignore
└── README.md
```

---

## Personalizar las opciones

Para agregar o modificar las opciones del wizard, edita en `obs-setup-wizard.cpp`:

```cpp
// En createUseCasePage() — agrega tu opción al final:
QList<UseCaseOption> options = {
    ...
    {"🎵", "Música en vivo", "Conciertos y sesiones de DJ"},  // ← nueva opción
};
```

Para modificar la configuración que se aplica:

```cpp
// En buildConfigMap() — clave = índice de la opción
map[6] = {"x264", "AAC", "8000 kbps", "192 kbps",
          1920, 1080, 60, "obs-ndi", ""};
```

---

## Licencia

GPL-2.0 — igual que OBS Studio.  
Puedes distribuir y modificar libremente citando el repositorio original.
