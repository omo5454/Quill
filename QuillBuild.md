# Quill — Build Process

This document explains the full pipeline from TypeScript source to a distributable Windows executable.

---

## Table of Contents
- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Project Structure](#project-structure)
- [Step 1 — TypeScript Compilation](#step-1--typescript-compilation)
- [Step 2 — Bundling](#step-2--bundling)
- [Step 3 — Minification & Compilation](#step-3--minification--compilation)
- [Step 4 — Building the Installer](#step-4--building-the-installer)
- [Full Release Script](#full-release-script)
- [Build Output](#build-output)
- [Benchmarks](#benchmarks)

---

## Overview

The build pipeline has 5 stages:

```
TypeScript (.ts)
      ↓  tsc
JavaScript (.js)
      ↓  esbuild
Single bundled file (quill.bundled.js)
      ↓  terser + bun (combined in minify script)
Minified Windows executable (quill.exe)
      ↓  Inno Setup
Installer (QuillInstaller.exe)
```

Each stage has a specific job. Skipping stages is possible for development but the full pipeline is required for a release build.

---

## Prerequisites

The following tools are required:

| Tool | Purpose | Install |
|------|---------|---------|
| Node.js | Runtime | [nodejs.org](https://nodejs.org) |
| Bun | EXE compiler | [bun.sh](https://bun.sh) |
| TypeScript | TS compiler | `npm install --save-dev typescript` |
| esbuild | Bundler | `npm install --save-dev esbuild` |
| Terser | Minifier | `npm install --save-dev terser` |
| Inno Setup | Installer builder | [jrsoftware.org](https://jrsoftware.org/isinfo.php) |

All Node.js tools are listed in `package.json` and can be installed in one go:

```bash
npm install
```

---

## Project Structure

```
interperter/
├── core/
│   ├── lexer/
│   │   └── lexer.ts          # Tokenizer
│   ├── parser/
│   │   └── parser.ts         # AST builder
│   ├── interpreter/
│   │   └── interpreter.ts    # Tree-walk evaluator
│   ├── ast/
│   │   └── ast.ts            # AST node type definitions
│   └── types/
│       └── types.ts          # TokenType and Token interface
├── dest/                     # tsc output (generated)
├── quill.ts                  # CLI entry point
├── quill.exe                 # Compiled minified executable
├── installer.iss             # Inno Setup script
├── tsconfig.json
└── package.json
```

---

## Step 1 — TypeScript Compilation

**Tool:** `tsc` (TypeScript compiler)

**What it does:** Strips all TypeScript types and emits plain JavaScript into the `dest/` folder, preserving the folder structure of `core/`.

**Command:**
```bash
npm run build
# which runs: tsc
```

**tsconfig.json key settings:**
```json
{
  "compilerOptions": {
    "target": "ES2015",
    "module": "ESNext",
    "rootDir": "./",
    "outDir": "./dest"
  },
  "include": ["core/**/*"],
  "exclude": ["node_modules", "dest", "**/*.test.ts"]
}
```

- `target: ES2015` — compiles class fields into constructor assignments so downstream tools don't choke on modern syntax
- `outDir: ./dest` — all compiled JS lands here
- `include: core/**/*` — only compiles source files, not tests or output

**Output:** `dest/core/lexer/lexer.js`, `dest/core/parser/parser.js`, etc.

---

## Step 2 — Bundling

**Tool:** `esbuild`

**What it does:** Takes all the separate JS modules and merges them into a single self-contained file. This resolves all the `import` statements and handles Node.js built-ins like `fs` and `path` that other tools (like Closure Compiler) can't deal with.

**Command:**
```bash
npm run bundle
# which runs: esbuild dest/quill.js --bundle --platform=node --format=esm --external:fs --external:path --outfile=dest/quill.bundled.js
```

**Flags explained:**
- `--bundle` — inline all imports into one file
- `--platform=node` — target Node.js, not the browser
- `--format=esm` — output as ES modules
- `--external:fs --external:path` — leave Node built-ins as external imports rather than trying to bundle them
- `--outfile` — single output file

**Output:** `dest/quill.bundled.js` — one file containing the entire Quill runtime.

---

## Step 3 — Minification & Compilation

**Tools:** `terser` + `bun`

**What it does:** Two things happen in the minify script. First Terser compresses the bundled JS, then Bun compiles the minified JS directly into a standalone Windows executable in one step. The user does not need Node.js or Bun installed to run the output.

Terser compression includes:
- Removing all whitespace and newlines
- Stripping all comments
- Shortening variable names (`interpreter` → `a`)
- Replacing boolean literals (`true` → `!0`, `false` → `!1`)
- Removing dead code

**Command:**
```bash
npm run minify
# which runs: terser dest/quill.bundled.js --compress --mangle --output dest/quill.min.js && bun build dest/quill.min.js --compile --outfile quill.exe
```

**Terser flags:**
- `--compress` — apply all compression transforms
- `--mangle` — rename variables to shortest possible names

**Bun flags:**
- `--compile` — produce a standalone executable with the Bun runtime embedded
- `--outfile` — name of the resulting executable

**Output:** `quill.exe` — the final minified standalone executable.

> **Why not use Google Closure Compiler?** Closure Compiler was trialled but does not support Node.js built-ins (`fs`, `path`) and its ADVANCED compilation mode requires full program visibility that conflicts with Node's module system. Terser produces equivalent output with no such limitations.

> **Why Bun over pkg?** Bun's `--compile` flag produces smaller, faster executables than pkg and doesn't require a separate Node.js installation to build. It also compiles in a single command with no extra configuration.

---

## Step 4 — Building the Installer

**Tool:** Inno Setup 6

**What it does:** Packages `quill.exe` into a user-friendly Windows installer (`QuillInstaller.exe`) that:
- Installs Quill to `Program Files\Quill`
- Optionally adds Quill to the system PATH so `quill myfile.quill` works from any terminal
- Prevents duplicate PATH entries on reinstall

**Script:** `installer.iss`

```iss
[Setup]
AppName={your app name}
AppVersion={your version}
DefaultDirName={pf}\{your app name}
DefaultGroupName={your app name}
OutputBaseFilename={your app name}Installer
OutputDir=.

[Files]
Source: "{your exe name}.exe"; DestDir: "{app}"; DestName: "{your exe name}.exe"

[Icons]
Name: "{group}\{your app name}"; Filename: "{app}\{your exe name}.exe"

[Tasks]
Name: "addtopath"; Description: "Add {your app name} to PATH (use {your exe name} from anywhere)"; GroupDescription: "System integration"

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + Param + ';', ';' + OrigPath + ';') = 0;
end;

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Check: NeedsAddPath('{app}'); Tasks: addtopath
```

**To compile the installer:**
1. Open `installer.iss` in Inno Setup
2. Press `Ctrl+F9` or go to **Build → Compile**
3. `QuillInstaller.exe` appears in the project root

---

## Full Release Script

All steps except the Inno Setup compile are wired into `package.json`:

```json
"scripts": {
  "build": "tsc",
  "bundle": "esbuild dest/quill.js --bundle --platform=node --format=esm --external:fs --external:path --outfile=dest/quill.bundled.js",
  "minify": "terser dest/quill.bundled.js --compress --mangle --output dest/quill.min.js && bun build dest/quill.min.js --compile --outfile quill.exe",
  "release": "npm run build && npm run bundle && npm run minify"
}
```

Run the full pipeline with:

```bash
npm run release
```

Then open Inno Setup and compile `installer.iss` manually for the final installer.

---

## Build Output

After `npm run release` the following files are produced:

| File | Description |
|------|-------------|
| `dest/` | Compiled TypeScript output |
| `dest/quill.bundled.js` | Single bundled JS file |
| `dest/quill.min.js` | Minified bundle |
| `quill.exe` | Final distributable executable (minified)

After Inno Setup compile:

| File | Description |
|------|-------------|
| `QuillInstaller.exe` | Windows installer for end users |

---

## Benchmarks

Measured on Windows using PowerShell `Measure-Command` averaged over 10 runs:

| Build | Time |
|-------|------|
| `node quill.min.js` | ~1379ms |
| `quill.exe` (minified) | ~327ms |

The minified executable is roughly **4x faster** than running via Node directly. The primary gains come from Terser's compression reducing parse time and the embedded Bun runtime skipping startup overhead.